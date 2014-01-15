-- 15.01.14 - ngc92: Use blobby volley api provided constants when possible

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
CONST_MITTE = CONST_FIELD_WIDTH/2

-- Linke Ber�hrungsebene des Balls falls er ans Netz kommt
CONST_NETZ_LINKS = CONST_MITTE - CONST_NET_RADIUS - CONST_BALL_RADIUS 


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
	target = estimImpact()
	
	if aggro then
		sprungattacke(aggroservice)
		return
	end

	-- Zielbestimmung --> optimierung weniger CPU power n�tig ;)
	if (target < CONST_BALL_RADIUS) then -- ball wird hinten apprallen
		vonhinten(target)
		return
	end
	
	-- nun k�mmern wir uns um die B�lle die ans netz prallen
	if ((target > CONST_NETZ_LINKS) and (ballx() < CONST_NETZ_LINKS)) then
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
		moveto(CONST_NETZ_LINKS - (p_target - CONST_NETZ_LINKS) + math.abs(bspeedx()*bspeedx()*1.4))
end

function vonhinten(p_target)
		p_target = CONST_BALL_RADIUS + math.abs(CONST_BALL_RADIUS-p_target)
		--das ist der fall wenn  der Ball hinten abprallt
		moveto(p_target-math.abs(bspeedx()*bspeedx()*1.1))
end

function sprungattacke(p_aggroservice)
		moveto(ballx()-p_aggroservice)
		if bally() < 580 then
			jump()
		end
end

function aggroflagtesten()
	if (ballx() > CONST_MITTE) then
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

function estimTime()
	bgrav = 0.248	
	-- standart Realfloor ist 144 
	-- warscheinilcher Realflfoor f�r Ball + blobby ist 144 (der Ball muss schon berechnet worden sein)
	realFloor = 144

    by=bally()
    vby=bspeedy()-600

	time1 =(-vby+math.sqrt((vby^2)-(4*-0.5*bgrav*(by-realFloor))))/(-2*0.5*bgrav)
    time2 =(-vby-math.sqrt((vby^2)-(4*-0.5*bgrav*(by-realFloor))))/(-2*0.5*bgrav)
    
    if time1>time2 then
        time3 = time1
    else
        time3 = time2
    end
	
	return time3

	--ist die Funktion nicht auf folgende aussage minimierbar ?
	-- time1 =(-vby-math.sqrt((vby^2)-(-2*bgrav*(by-realFloor))))/-bgrav
	-- return time1
end

function estimImpact()
	--diese Funktion sollte die einschlagkoordinaten auf K�pfh�he Rechnen
    bx=ballx()
    vbx=bspeedx()
    
    time3 = estimTime()
    resultX = (vbx * time3) + bx    
    
    return resultX
end