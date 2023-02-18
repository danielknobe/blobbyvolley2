-- 15.01.14 - ngc92: Use blobby volley api provided constants when possible
-- 11.04.15 - ngc92: updated math helpers

function OnOpponentServe()
	moveto(120)
end

-- Flags und runners
wait = 0
aggro = true -- evtl Variablennamen wechseln
aggroservice = 50 -- versetzung zum Ball je grösser desto tiefer fliegt der Service 0-64(  64 ist ca CONST_BALLRADIUS + CONST_BLOBBY_BAUCH_RADIUS)

-- Konstanten
function OnServe(ballready)
	moveto(ballx())
	wait = wait + 1 
	aggro = true
	if ballready then
		if wait > 90 then
			jump()
			wait = 0
		end
	end
end	

function OnGame()
	aggro = aggroflagtesten() -- schaut ob der bot aggro werden soll oder nicht
	local timetoblobhead = ball_time_to_y(CONST_BALL_BLOBBY_HEAD)
	local target, hit = estimx(timetoblobhead)
	
	if aggro then
		sprungattacke(aggroservice)
		return
	end

	-- Zielbestimmung --> optimierung weniger CPU power nötig ;)
	if (hit == CONST_BALL_LEFT_BORDER) then -- ball wird hinten apprallen
		vonhinten(target)
		return
	end
	
	-- nun kümmern wir uns um die Bälle die ans netz prallen
	if hit == CONST_BALL_LEFT_NET then
		netzappraller(target)
	end
	
	-- it crosses the net - we are save
	if target > CONST_BALL_RIGHT_NET then
		target = 220
	end
	
	if (bspeedx() < 10 ) then
		moveto(target - 20)
	else
		moveto(target)
	end

end


function netzappraller(p_target)
		--moveto(netzlinks - (netzlinks - estimate()))
		moveto(p_target + bspeedx()*bspeedx()*1.4)
end

function vonhinten(p_target)
		--das ist der fall wenn  der Ball hinten abprallt
		moveto(p_target - bspeedx()*2)
end

function sprungattacke(p_aggroservice)
		moveto(ballx()-p_aggroservice)
		if bally() < 580 then
			jump()
		end
end

function aggroflagtesten()
	if (ballx() > CONST_FIELD_MIDDLE) then
		return false 
	end
	
	if (touches() == 2) then
		return true
	end
	
	if (ballx() < (CONST_FIELD_MIDDLE - CONST_BALL_RADIUS)) and (bally() < 210) then -- der ball könnte nohc drüber fliegen ** noch optimieren
		return true
	end
	
	-- leave unchanged
	return aggro
end
