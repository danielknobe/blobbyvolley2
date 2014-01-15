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
CONST_MITTE = CONST_FIELD_WIDTH/2
CONST_RECHTER_RAND = CONST_FIELD_WIDTH - CONST_BALL_RADIUS

-- Berührungsebene des Balls falls er ans Netz kommt
CONST_NETZ_LINKS = CONST_MITTE - CONST_NET_RADIUS - CONST_BALL_RADIUS 
CONST_NETZ_RECHTS = CONST_MITTE + CONST_NET_RADIUS + CONST_BALL_RADIUS 

CONST_BLOBBY_KOPF_BERUEHRUNG = CONST_GROUND_HEIGHT + CONST_BLOBBY_HEIGHT + CONST_BALL_RADIUS
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
   estimImpact2(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_MAXJUMP - 25,1)
end

function estimImpactLow()
   estimImpact2(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_KOPF_BERUEHRUNG,1)
end

function estimImpact2(bx,by,vbx,vby,destY,Frage) -- erlaubt ein besseres Estimate mit ein paar unbeding nötigen Angaben
   bgrav = -CONST_BALL_GRAVITY

   if ((vby^2)-(-2*bgrav*(by-destY))) < 0 then
      target = nil
      return
   end
   time1 =(-vby-math.sqrt((vby^2)-(-2*bgrav*(by-destY))))/(-bgrav)
   --time2 =(-vby+math.sqrt((vby^2)-(-2*bgrav*(by-destY))))/(-bgrav)
   timeto = time1

   if (timeto < 0) then
      target = nil
      return
   end
   naivetarget = (vbx * time1) + bx
   resultX = naivetarget
   estimbspeedx=bspeedx()

   direct = true

   if(resultX > CONST_RECHTER_RAND) then
      resultX = 2 * CONST_RECHTER_RAND - resultX
      estimbspeedx=-estimbspeedx
      direct = true
   end

   if(resultX < CONST_BALL_RADIUS) then -- korrigieren der Appraller an der linken Ebene
      resultX = 2 * CONST_BALL_RADIUS - resultX
      estimbspeedx=-estimbspeedx
      direct = false
   end

   if (estimbspeedx > 0) and (resultX > CONST_NETZ_LINKS) then
      direct = false
      resultX = 2 * CONST_NETZ_LINKS - resultX
      estimbspeedx=-estimbspeedx
   end

   target = resultX
end
