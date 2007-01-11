-- thx @ Combotteam, viele Sachen wurden von euch übernommen, und der Bot wurde einfach auf dumm herumspringen getrimmt (wie der der Name schon sagt ;) )
-- Flags und runners
wait = 0
naechsterBallSchmettern = true -- evtl Variablennamen wechseln
angriffsstaerke = 50 -- versetzung zum Ball je grösser desto tiefer fliegt der Service 0-64(  64 ist ca CONST_BALLRADIUS + CONST_BLOBBY_BAUCH_RADIUS) 
angriffsstaerkeNeuBerechnen =false -- neuberechnung von angriffsstaerke => Variable Angriffe

-- Konstanten

CONST_FELD_LAENGE = 800
CONST_BALL_RADIUS = 31.5
CONST_GROUND_PLANE = 100

CONST_MITTE = CONST_FELD_LAENGE/2
CONST_RECHTER_RAND = CONST_FELD_LAENGE - CONST_BALL_RADIUS


CONST_BLOBBY_HOEHE = 89
CONST_BLOBBY_KOPF_RADIUS = 25
CONST_BLOBBY_BAUCH_RADIUS = 33
CONST_BLOBBY_KOPF_BERUEHRUNG = CONST_GROUND_PLANE + CONST_BLOBBY_HOEHE + CONST_BALL_RADIUS

CONST_NETZ_RADIUS = 7
CONST_NETZ_HOEHE = 157

-- Linke Berührungsebene des Balls falls er ans Netz kommt
CONST_NETZ_LINKS = CONST_MITTE - CONST_NETZ_RADIUS - CONST_BALL_RADIUS 



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

	target = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),posy()+CONST_BLOBBY_HOEHE)	
	jump()	
	moveto(target-16)
end


-- von Combot übernommen ! (thx für die gute Estimatefunktion =) )
function estimImpact(bx,by,vbx,vby,destY) -- erlaubt ein besseres Estimate mit ein paar umbeding nötigen Angaben
	bgrav = 0.28	

    time1 =(-vby-math.sqrt((vby^2)-(-2*bgrav*(by-destY))))/(-bgrav)
    resultX = (vbx * time1) + bx

	if(resultX < CONST_BALL_RADIUS) then -- korrigieren der Appraller an der Hinteren Ebene
		resultX = math.abs(CONST_BALL_RADIUS - resultX) + CONST_BALL_RADIUS
	end
	
	if(resultX > CONST_RECHTER_RAND) then -- Korrigieren der Appraller an der Rechten Ebene
		resultX = CONST_FELD_LAENGE - (resultX - CONST_FELD_LAENGE)
	end

    return resultX
end
