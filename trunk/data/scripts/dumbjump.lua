ballposx=0
ballposy=0

function OnOpponentServe()
  moveto(130)
end

function OnServe(ballready)
  if ballready then
    moveto(ballx() - 40)
    if (posx() < ballx() - 37 and posx() > ballx() - 43) then
     jump()
    end
  else
    moveto(200)
  end
end

function OnGame()
	if 400 > ballx() then
		moveto((ballx()-20))
		if ballx() == ballposx and bally() == ballposy then
			if ballx() - 20 > posx() then
				jump()
			end
		else
			if ballx() < posx()+50 then
				jump()
			end
		end
		ballposx=ballx()
		ballposy=bally()
	end
end
