function OnOpponentServe()
	moveto(120)
end

-- Flags und runners
wait = 0
aggro = true -- evtl Variablennamen wechseln
aggroservice = 50 -- versetzung zum Ball je grösser desto tiefer fliegt der Service 0-64(  64 ist ca CONST_BALLRADIUS + CONST_BLOBBY_BAUCH_RADIUS) 

-- Korrekturfaktor für die Flugberechnung
korrekturfaktor = 7

-- Konstanten
CONST_BALL_RADIUS = 31.5

CONST_MITTE = 400

CONST_BLOBBY_HOEHE = 89
CONST_BLOBBY_KOPF_RADIUS = 25
CONST_BLOBBY_BAUCH_RADIUS = 33

CONST_NETZ_RADIUS = 7
CONST_NETZ_HOEHE = 157

-- Linke Berührungsebene des Balls falls er ans Netz kommt
CONST_NETZ_LINKS = CONST_MITTE - CONST_NETZ_RADIUS - CONST_BALL_RADIUS 

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

	if (target > 400) then
	
	else
		if aggro then
			sprungattacke(aggroservice)
			return
		end

		-- Zielbestimmung --> optimierung weniger CPU power nötig ;)
		if (target < CONST_BALL_RADIUS) then -- ball wird hinten apprallen
			vonhinten(target)
			return
		end
		
		-- nun kümmern wir uns um die Bälle die ans netz prallen
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

	if ((ballx() < (400-CONST_BALL_RADIUS)) and (bally() < 200) and (math.abs(bspeedx()) < 40)) then -- der ball könnte nohc drüber fliegen ** noch optimieren
		aggro = true
		return
	end
end

function estimTime()
	bgrav = 0.28
	
	-- 100 bezeichnet die Ebene auf denen die Blobbys stehen
	-- 89 ist die Blobbyhöhe
	-- hier wird also die Höhe festgelegt auf der der Ball ankommen soll
	realFloor = 100 + 89

    by=bally()
    vby=bspeedy()
	
    time1 =(-vby-math.sqrt((vby^2)-(4*-0.5*bgrav*(by-realFloor))))/(-2*0.5*bgrav)
    
    return time1
end

function estimImpact()
	--diese Funktion sollte die einschlagkoordinaten auf Köpfhöhe Rechnen
    bx=ballx()
    vbx=bspeedx()
    
    time1 = estimTime()
    resultX = (vbx * time1) + bx  

    return resultX
end
