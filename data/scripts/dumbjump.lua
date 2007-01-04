function OnOpponentServe()
	if (oppx() < 680) and (oppx() >= 540) then
		moveto(-2*(oppx()-600)+oppx()-400)
	end
end

function OnServe(ballready)
	if ballready then
		moveto(ballx() - 40)
		if (posx() < ballx() - 37 and posx() > ballx() - 43) then
			jump()
		end
	else
		moveto(100)
	end
end

function OnGame()

	if touches() == 0 and estimate() < 375 and estimate() > 0 then

		if bspeedx() < 0 then
			moveto(estimate()+5)
		end

		if bspeedx() > 0 then
			moveto(estimate()-5)
		end
	return
	end


		if estimate() < 375 and estimate() > 0 then
			moveto(estimate())
		else
			if 520 > ballx() and bally() > 220 then
				jump()
			end

			if 400 > ballx() then
				moveto((ballx()-20))
			end
		end

		if 400 > ballx() and bspeedx() < 3 and bspeedx() > -3 then
			moveto((ballx()-30))
			jump()
		end

end
