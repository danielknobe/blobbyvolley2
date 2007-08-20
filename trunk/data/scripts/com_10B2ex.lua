-- Com 1.0 extrem Beta 2 Schmetter-Mod
-- by Oreon, Axji & Enormator
-- Name: com_10eB2_Schmettermod

-- Flags und runners
wait = 0
naechsterBallSchmettern = true -- evtl Variablennamen wechseln


-- Weltkonstanten

CONST_FELD_LAENGE = 800
CONST_BALL_RADIUS = 31.5
CONST_GROUND_PLANE = 100

CONST_BALL_GRAVITY = 0.28

CONST_MITTE = CONST_FELD_LAENGE/2
CONST_RECHTER_RAND = CONST_FELD_LAENGE - CONST_BALL_RADIUS

CONST_BLOBBY_HOEHE = 89
CONST_BLOBBY_KOPF_RADIUS = 25
CONST_BLOBBY_BAUCH_RADIUS = 33
CONST_BLOBBY_KOPF_BERUEHRUNG = CONST_GROUND_PLANE + CONST_BLOBBY_HOEHE + CONST_BALL_RADIUS
CONST_BLOBBY_MAXJUMP = 393.625

CONST_NETZ_RADIUS = 7
CONST_NETZ_HOEHE = 323

-- Ber�hrungsebene des Balls falls er ans Netz kommt
CONST_NETZ_LINKS = CONST_MITTE - CONST_NETZ_RADIUS - CONST_BALL_RADIUS 
CONST_NETZ_RECHTS = CONST_MITTE + CONST_NETZ_RADIUS + CONST_BALL_RADIUS 

-- Charakter
CONST_ANGRIFFSGRUNDWERT_MIN = 30
CONST_ANGRIFFSGRUNDWERT_MAX = 55
MIN_ANGRIFFSSTAERKE = CONST_ANGRIFFSGRUNDWERT_MIN
MAX_ANGRIFFSSTAERKE = CONST_ANGRIFFSGRUNDWERT_MAX
ANGRIFFSEINSCHRAENKUNG_HINTEN = 10

-- sonstige Einstellungen
servexVersetzung=-7 --Wert ist so gewaehlt, dass der Ball nah ans Netz fliegt, der Gegner ihn aber grade nicht erreichen kann

-- ***ANFANG***

function OnOpponentServe()
	moveto(130)
end

function OnServe(ballready)
	servex=ballx()+servexVersetzung
	naechsterBallSchmettern = true
	generatenaechsterBallSchmettern()
	moveto(servex)
	if ballready and (servex-2 < posx()) and (posx() < servex+2) then
		jump()
	end
end	

function OnGame()
	target = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_KOPF_BERUEHRUNG,1) --X Ziel in Blobbyhoehe
	targets = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_KOPF_BERUEHRUNG,2) --X Richtung (-1 oder 1) bei Einschlag
	targetNetz = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_NETZ_HOEHE,1) --X Ziel in Netzhoehe (Netzrollerberechnung)
	targetJump = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_MAXJUMP,1) --X Ziel in Schmetterhoehe
	naechsterBallSchmetternFlagTesten() -- schaut ob der bot angreifen soll oder nicht

	if (ballx() > CONST_NETZ_RECHTS) then --Wenn Ball auf rechter Spielfeldseite dann
		generatenaechsterBallSchmettern() --Angriffsstaerke neu berechnen
	end
	
	if (target > CONST_MITTE) and (ballx() > CONST_NETZ_RECHTS) then --Wenn der Ball mich nix angeht
		moveto(135) --Dann auf Standartposition warten
	else
		if (targetNetz > CONST_NETZ_LINKS - 10) then --Bei Netzroller einfach schmettern
			naechsterBallSchmettern = true
		end

		if naechsterBallSchmettern then
			if ((math.abs(bspeedx()) < 4) or (estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_MAXJUMP,2) < 0)) then
				sprungattacke(angriffsstaerke)
			else
				if (targetJump < CONST_MITTE / 2) then
					sprungattacke(-35) --an Rueckwand spielen
				else
					sprungattacke(0) --weiterleiten
				end
			end
			return
		end

		moveto(target)
	end
end


function sprungattacke(p_angriffsstaerke)
	p_angriffsstaerke=math.max(p_angriffsstaerke, MIN_ANGRIFFSSTAERKE + ANGRIFFSEINSCHRAENKUNG_HINTEN * (targetJump / CONST_NETZ_LINKS)) --Weiter hinten nicht ganz so hoch spielen (kommt nicht auf die andere Seite)
	p_angriffsstaerke=math.min(p_angriffsstaerke, MAX_ANGRIFFSSTAERKE - ANGRIFFSEINSCHRAENKUNG_HINTEN * (targetJump / CONST_NETZ_LINKS)) --Weiter hinten nicht ganz so tief spielen (kommt ans Netz)

	moveto(targetJump-p_angriffsstaerke) -- Bei der Sprungatacke wird die St�rke des gew�nschten schlages angegeben

	if (bally() < 580) and (bspeedy() < 0) then
		jump()
	end
end

function naechsterBallSchmetternFlagTesten()
	naechsterBallSchmettern = true
end

function generatenaechsterBallSchmettern()
	angriffsstaerke = math.random(MIN_ANGRIFFSSTAERKE,MAX_ANGRIFFSSTAERKE)
end

function estimImpact(bx,by,vbx,vby,destY,Frage) -- erlaubt ein besseres Estimate mit ein paar unbeding n�tigen Angaben
	bgrav = 0.28	

    time1 =(-vby-math.sqrt((vby^2)-(-2*bgrav*(by-destY))))/(-bgrav)
    resultX = (vbx * time1) + bx
	estimbspeedx=bspeedx()/math.abs(bspeedx())

	if(resultX > CONST_RECHTER_RAND) then -- Korrigieren der Appraller an der Rechten Ebene
		resultX = 2 * CONST_FELD_LAENGE - resultX
		estimbspeedx=-estimbspeedx
	end

	if(resultX < CONST_BALL_RADIUS) then -- korrigieren der Appraller an der linken Ebene
		resultX = math.abs(resultX - CONST_BALL_RADIUS) + CONST_BALL_RADIUS
		estimbspeedx=-estimbspeedx
		KollisionLinks = true
	else
		KollisionLinks = false
	end

	if (resultX > CONST_NETZ_RECHTS) and (estimbspeedx > 0) and ((KollisionLinks == true) or (ballx() < CONST_NETZ_LINKS)) then -- Abpraller am Netz unterhalb der Kugel erst wenn Netzroller ausgeschlossen sind
		resultX = 2 * CONST_NETZ_LINKS - resultX
		estimbspeedx=-estimbspeedx
	end

	if (Frage == 1) then
		return resultX
	end
	if (Frage == 2) then
		return estimbspeedx
	end
end