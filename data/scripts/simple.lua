ballPosX = 0
ballPosY = 0

serving = false
readyToServe = false
mySide = side()

function ballDistance()
  local x = abs(posx() - ballx())
  local y = abs(posy() - bally())
  return x * x + y * y
end

function step()
  stop()
  stopjump()
      
  if (ballPosX == ballx()) and (ballPosY == bally()) and ballx() < 400 then
    serving = true
  else
    serving = false
  end
  
  serveTarget = ballx() - 25
  if serving then
    if (posx() < serveTarget + 3) and (posx() > serveTarget - 3) then
      jump()
      readyToServe = true
      if posy() < 400 then
        right()
      end
    else 
      readyToServe = false
      if posx() < serveTarget then
        right()
      else
        left()
      end
    end
  else
    target = estimate(0)
    if target < 0 then
      target = abs(target)
    end
    
    targetDiff = 0
    
--    target = target - bspeedx() * 4
--    targetDiff = targetDiff - abs(bspeedx())
    
-- Attack if near by net
    if ballx() > 300 and ballx() < 400 and ballx() > posx() then
      jump()
      targetDiff = targetDiff - 15
    end
    
-- Try to get rid of the ball with third strike
    if touches() > 1 then
    	targetDiff = targetDiff - 55 + (400.0 - ballx()) / 15
    	if abs(posx() - ballx()) < 64 and bally() > 350 then
    	  jump()
    	end
    end
    
    if touches() > 2 then
        targetDiff = -200
    end
    
    if bspeedx() < 0 then
--      targetDiff = targetDiff + 3 * abs(bspeedx() / bspeedy())
      targetDiff = targetDiff - bspeedx() * 5
    end
    
    if ballx() > 500.0 then
      moveto (100.0)
    else
      moveto(target + targetDiff)
    end
    
-- Protection against balls from back
    if ballx() < posx() then
      left()
    end
    
--    if abs(posx() - ballx()) < 50 and abs(posy() - bally()) < 300 then
--      if abs(estimate(0) - posx()) < 60 then
--        jump()
--      end
--    end
  
  end
  
  ballPosX = ballx()
  ballPosY = bally()
end
