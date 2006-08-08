ballposx=0
ballposy=0

function step()
	stop()
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
	else
		stopjump()
	end
end
