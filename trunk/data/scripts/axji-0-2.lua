-- 15.01.14 - ngc92: Use blobby volley api provided constants when possible
-- 11.04.15 - ngc92: updated math helpers

function OnOpponentServe()
	moveto(120)
end

-- Flags und runners
wait = 0
aggro = true -- evtl Variablennamen wechseln
aggroservice = 50 -- versetzung zum Ball je gr�sser desto tiefer fliegt der Service 0-64(  64 ist ca CONST_BALLRADIUS + CONST_BLOBBY_BAUCH_RADIUS) 

-- Korrekturfaktor f�r die Flugberechnung
korrekturfaktor = 7

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
	aggroflagtesten() -- schaut ob der bot aggro werden soll oder nicht
	local target, hit = estimImpact()
	
	if aggro then
		sprungattacke(aggroservice)
		return
	end

	-- Zielbestimmung --> optimierung weniger CPU power n�tig ;)
	if (hit == CONST_BALL_LEFT_BORDER) then -- ball wird hinten apprallen
		vonhinten(target)
		return
	end
	
	-- nun k�mmern wir uns um die B�lle die ans netz prallen
	if ((target > CONST_BALL_LEFT_NET) and (ballx() < CONST_BALL_LEFT_NET)) then
		netzappraller(target)
		return
	end
	
	if (bspeedx() < 10 ) then
		moveto(target - 20)
	else
		moveto(target)
	end

end


function netzappraller(p_target)
		--moveto(netzlinks - (netzlinks - estimate()))
		moveto(mirror(p_target, CONST_BALL_LEFT_NET) + bspeedx()*bspeedx()*1.4)
end

function vonhinten(p_target)
		--das ist der fall wenn  der Ball hinten abprallt
		moveto(p_target - bspeedx()*bspeedx()*1.1)
end

function sprungattacke(p_aggroservice)
		moveto(ballx()-p_aggroservice)
		if bally() < 580 then
			jump()
		end
end

function aggroflagtesten()
	if (ballx() > CONST_FIELD_MIDDLE) then
		aggro = false 
		return
	end
	
	if (touches() == 2) then
		aggro = true
		return
	end

	if ((ballx() < (400-CONST_BALL_RADIUS)) and (bally() < 200) and (math.abs(bspeedx()) < 40)) then -- der ball k�nnte nohc dr�ber fliegen ** noch optimieren
		aggro = true
		return
	end
end

function estimImpact()
	--diese Funktion sollte die einschlagkoordinaten auf K�pfh�he Rechnen
	-- standart Realfloor ist 144 
	-- warscheinilcher Realflfoor f�r Ball + blobby ist 144 (der Ball muss schon berechnet worden sein)
	local realFloor = 144
	
	local time3 = ball_time_to_y(realFloor, balldata())
	return estimx(time3)
end