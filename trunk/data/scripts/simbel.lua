function OnOpponentServe()
  moveto(120)
end

function OnServe(ballready)
  serveTarget = ballx() - 25
  if (posx() < serveTarget + 3) and (posx() > serveTarget - 3) then
    if ballready then
      jump()
    end
    readyToServe = true
  else 
    readyToServe = false
    if posx() < serveTarget then
      right()
    else
      left()
    end
  end
end
    
function OnGame()
  target = estimx(450 / math.pow(bspeedy() * bspeedy(), 2.9))
  if target < 0 then
    target = math.abs(target)
  end
  if target > 400 then
    target = 800 - target + 100
  end
  
  
  targetDiff = 0
   
-- Attack if near by net
  if ballx() > 280 and ballx() < 400 and ballx() > posx() then
    jump()
    targetDiff = targetDiff - 25
    if math.abs(posy() - bally()) < 90 then
      right()
    end
  end
    
-- Try to get rid of the ball with third strike
  if touches() > 0 then
    targetDiff = targetDiff - 40 + (400 - ballx()) / 15
    if math.abs(posx() - ballx()) < 50 and bally() - posy() < 40 then
      if math.abs(bspeedx()) * 1.3 < math.abs(bspeedy()) then
        jump()
      end
    end
  end
   
  if touches() > 2 then
    targetDiff = -200
  end
    
  if ballx() > 500.0 then
    moveto (100.0)
  else
    moveto(target + targetDiff)
  end
    
-- Protection against balls from back
  if ballx() + 20 < posx() and ballx() < 200 then
    left()
    if bally() - posy() > 0 then
    if math.abs(bspeedy() / bspeedx()) > 1 then
      if bspeedx() > 0 then
        jump()
      end
    end
    end
  end
    
end
