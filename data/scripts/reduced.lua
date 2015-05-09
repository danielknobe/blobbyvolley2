-- Reduced but very effective bot.
-- Borrows from axji.lua, com_10.lua and com_11.lua as distributed with
-- Blobby Volley.
-- Copyright (C) 2010 Soeren D. Schulze
-- The terms of the GPLv2 or later apply.
-- See the Blobby Volley distribution for the license text.

-- 15.01.14 - ngc92: Use blobby volley api provided constants when possible

modeLock = 0
timeto = 0
target = 0
naivetarget = 0
estimbspeedx = 0
direct = true
servrand = nil

-- Konstanten
-- TODO calculate this from constants
CONST_BLOBBY_MAXJUMP = 393.625

function OnServe(ballready)
   if servrand == nil then
      servrand = math.random()
   end
   moveto(ballx() + servrand * 5)
   if ballready and math.abs(posx() - (ballx() + servrand * 5)) < 3 then
      jump()
      servrand = nil
   end
end	

function OnOpponentServe()
   moveto(100)
end

function OnGame()
   estimImpactHigh()

   if (not (target == nil))
      and naivetarget < 400
      and (modeLock == 1
	   or timeto > math.abs(posx()-highPlayPos())/4.5 + 26)
      and touches() < 3
   then
      if (timeto < 30) then
	 modeLock = 1
      else
	 modeLock = 0
	 servrand = nil
      end
      highPlay()
   else
      modeLock = 0
      servrand = nil
      estimImpactLow()
      if (not (target == nil))
         and ((estimbspeedx > 0 and timeto > (target-posx()-10)/4.5)
	      or (estimbspeedx < 0 and timeto > (posx()-target-10)/4.5)
	      or naivetarget >= 400) then
	 lowPlay()
      else
	 -- HEELLPPP...
	 if not (target == nil)
	    and naivetarget < 400 then
	    -- This often saves your ass if you're standing inside a
	    -- corner and the ball bounces from the wall or the net.
	    lowPlay()
	    jump()
	 end
      end
   end
end

function highPlayPos()
   if estimbspeedx < 0 then
      -- safety againt fast balls
      return target - 50 - estimbspeedx*5
   else
      return target - 50
   end
end

function highPlay()
   if (target > 400) then
      moveto(100)
   else
      moveto(highPlayPos())
      -- 33 time units for jumping to max height
      -- Regarding naive target here.
      -- If the ball really bounces back, it would be a bad idea to jump...
      if servrand == nil then
	 servrand = math.random()
      end
      if naivetarget < 400 and timeto < 28 + servrand then
	 jump()
      end
   end
end


function lowPlay()
   if (target > 400) then
      moveto(100)
   else
      moveto(target)
   end
end

function estimImpactHigh()
   estimImpact2(CONST_BLOBBY_MAXJUMP - 25, balldata())
end

function estimImpactLow()
   estimImpact2(CONST_BALL_BLOBBY_HEAD, balldata())
end

function estimImpact2(destY, bx,by,vbx,vby) -- erlaubt ein besseres Estimate mit ein paar unbeding nötigen Angaben
   timeto = ball_time_to_y(destY, bx, by, vbx, vby)

   if (timeto == math.huge) then
      target = nil
      return
   end
   -- TODO need estimx function for bx and vbx state
   naivetarget = (vbx * timeto) + bx
   resultX = naivetarget
   estimbspeedx=bspeedx()

   direct = true

   if(resultX > CONST_BALL_RIGHT_BORDER) then
      resultX = mirror(resultX, CONST_BALL_RIGHT_BORDER)
      estimbspeedx=-estimbspeedx
      direct = true
   end

   if(resultX < CONST_BALL_LEFT_BORDER) then -- korrigieren der Appraller an der linken Ebene
      resultX = mirror(resultX, CONST_BALL_LEFT_BORDER)
      estimbspeedx=-estimbspeedx
      direct = false
   end

   if (estimbspeedx > 0) and (resultX > CONST_BALL_LEFT_NET) then
      direct = false
      resultX = mirror(resultX, CONST_BALL_LEFT_NET)
      estimbspeedx=-estimbspeedx
   end

   target = resultX
end
