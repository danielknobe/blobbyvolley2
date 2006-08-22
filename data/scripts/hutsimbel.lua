function OnOpponentServe()
moveto(120)
end

wait = 0


function OnServe(ballready)
	serveTarget = ballx()- math.random(10, 49)
	wait = wait + 1
	if (posx() < serveTarget + 2) and (posx() > serveTarget - 2) then
		if ballready then
			if wait > 90 then
				jump()
				wait = 0
			end
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

	lasty = 0

function OnGame()

target = math.abs(estimx(450 / math.pow(bspeedy() * bspeedy(), 2.9)))

	if bspeedx() > 0 and target > 400 then
		target = 800 - target + 100
		else
			if target > 400 then
			toomuch = target - 400
			target = target - toomuch
		end
	end
	
	
	targetDiff = 0
	
	
--Netzattacke
	if ballx() > 1000 * math.abs(math.atan(bspeedy() / bspeedx())) and ballx() < 400 and ballx() > posx() then
    jump()
    targetDiff = targetDiff - 25
    if math.abs(posy() - bally()) < 90 then
      right()
    end
  end

    if touches() > 0 then
    targetDiff = targetDiff - 40 + (400 - ballx()) / 15
    if math.abs(posx() - ballx()) < 50 and bally() - posy() < 40 then
      if math.abs(bspeedx()) * 1.3 < math.abs(bspeedy()) then
        jump()
      end
    end
  end

  
  if 0 then if ballx() > 380 and ballx() < 420 and bally() < 480 then
  	jump()
  	targetDiff = targetDiff - 25
  	if math.abs(posy() - bally()) < 90 then
      right()
   end
	end
	end
	
	target = target + targetDiff
	

--Ball von Hinten

	if ballx() + 20 < posx() and ballx() < 220 then
    left()
    if bally() - posy() > 0 then
    	if math.abs(bspeedy() / bspeedx()) > 1 then
      	if bspeedx() > 0 then
      	  jump()
      	end
    	end
    end
  else
  		moveto(target)
  end
  
  
  

	lasty = posy()
	
end
