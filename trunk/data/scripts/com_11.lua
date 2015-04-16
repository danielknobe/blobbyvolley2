-- Combot 1.1
-- by Oreon, Axji & Enormator

-- 15.01.14 - ngc92: Use blobby volley api provided constants when possible
-- 11.04.15	- ngc92: Removed unused functions, updated math helpers

-- TODO estimatex function is missing

-- Flags und runners
naechsterBallSchmettern = true -- evtl Variablennamen wechseln


-- Weltkonstanten
CONST_BLOBBY_KOPF_BERUEHRUNG = CONST_GROUND_HEIGHT + CONST_BLOBBY_HEIGHT + CONST_BALL_RADIUS
CONST_BLOBBY_MAXJUMP = 393.625

-- Charakter
CONST_ANGRIFFSGRUNDWERT_MIN = 30
CONST_ANGRIFFSGRUNDWERT_MAX = 55
MIN_ANGRIFFSSTAERKE = CONST_ANGRIFFSGRUNDWERT_MIN
MAX_ANGRIFFSSTAERKE = CONST_ANGRIFFSGRUNDWERT_MAX
ANGRIFFSEINSCHRAENKUNG_HINTEN = 10

-- sonstige Einstellungen
servexVersetzung=-6 --Wert ist so gewaehlt, dass der Ball nah ans Netz fliegt, der Gegner ihn aber grade nicht erreichen kann

-- ***ANFANG***

function OnOpponentServe()
	moveto(130)
	generatenaechsterBallSchmettern()
end

function OnServe(ballready)
	servex=ballx()+servexVersetzung
	naechsterBallSchmettern = true
	generatenaechsterBallSchmettern()
	if ballready and movetoX(servex) then
		jump()
	end
end	

function OnGame()
	target, targets = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_KOPF_BERUEHRUNG) --X Ziel in Blobbyhoehe
	targetNetz = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_NET_HEIGHT + CONST_NET_RADIUS) --X Ziel in Netzhoehe (Netzrollerberechnung)
	targetJump, targetJumps = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_MAXJUMP) --X Ziel in Schmetterhoehe
	naechsterBallSchmetternFlagTesten() -- schaut ob der bot angreifen soll oder nicht
	
	if (target > CONST_MITTE) then --Wenn der Ball mich nix angeht
		moveto(135) --Dann auf Standartposition warten
		generatenaechsterBallSchmettern() --Angriffsstaerke neu berechnen
	else
		if (targetNetz > CONST_BALL_LEFT_NET - 10) then --Bei Netzroller einfach schmettern
			naechsterBallSchmettern = true
		end

		if naechsterBallSchmettern then
			if (targetJumps < 2) then
				sprungattacke(angriffsstaerke)
			else
				weiterleiten()
			end
			return
		end

		moveto(target)
	end
end


function sprungattacke(p_angriffsstaerke)
	if (opptouchable(balltimetoy(CONST_BLOBBY_MAXJUMP))) then
		moveto (CONST_MITTE)
		jumpto (383)
	else
		p_angriffsstaerke=math.max(p_angriffsstaerke, MIN_ANGRIFFSSTAERKE + ANGRIFFSEINSCHRAENKUNG_HINTEN * (targetJump / CONST_BALL_LEFT_NET)) --Weiter hinten nicht ganz so hoch spielen (kommt nicht auf die andere Seite)
		p_angriffsstaerke=math.min(p_angriffsstaerke, MAX_ANGRIFFSSTAERKE - ANGRIFFSEINSCHRAENKUNG_HINTEN * (targetJump / CONST_BALL_LEFT_NET)) --Weiter hinten nicht ganz so tief spielen (kommt ans Netz)
		moveto(targetJump-p_angriffsstaerke) -- Bei der Sprungatacke wird die Stärke des gewünschten schlages angegeben
		jumpto (383)
	end
end

function naechsterBallSchmetternFlagTesten()
	if (touches() == 3) then -- falls der Bot einen Anschlag Findet der Direckt punktet so wird der Wer nicht neu berechnet da er dann nciht auf 3 Berührungen kommt
		naechsterBallSchmettern = false
		return
	end
	
	if (ballx() > CONST_MITTE) then -- wenn der ball auf der Anderen Seite ist soll der bot nicht naechsterBallSchmettern sein
		naechsterBallSchmettern = false 
		return
	end
	
	if (touches() == 1) and (math.abs(bspeedx()) < 2) then -- schon nach der 1ten Beruehrung angreifen wenn der Ball gut kommt
		naechsterBallSchmettern = true
		return
	end
	
	if (touches() == 2) then -- nach der 2. Berührung angreifen
		naechsterBallSchmettern = true
		return
	end
	naechsterBallSchmettern = false
end

function generatenaechsterBallSchmettern()
	angriffsstaerke = math.random(MIN_ANGRIFFSSTAERKE,MAX_ANGRIFFSSTAERKE)
end

function estimImpact(bx,by,vbx,vby,destY) -- erlaubt ein besseres Estimate mit ein paar unbeding nötigen Angaben
    local time1 = ball_time_to_y(destY, bx, by, vbx, vby)
    local resultX, hit, estimbspeedx = estimx(time1)

	if (resultX > CONST_BALL_LEFT_NET) and (estimatey(CONST_MITTE) < CONST_NET_HEIGHT + CONST_NET_RADIUS) and (estimbspeedx > 0) then
		resultX = 2 * CONST_BALL_LEFT_NET - resultX
		estimbspeedx=-estimbspeedx
	end
	
	return resultX, estimbspeedx
end

-- TODO make the API function return that bool, so we can remove this one
function movetoX (x)
 if (math.abs(posx()-x)>math.abs(posx()+4.5-x)) then
  right()
  done=false
 else
  if (math.abs(posx()-x)>math.abs(posx()-4.5-x)) then
   left()
   done=false
  else
   done=true
  end
 end
 return done
end

function jumpto (y)
 if (blobtimetoy (y) >= balltimetoy (y)) then
  jump()
 end
end

function balltimetoy (y) --Zeit, die der Ball bis zu einer Y Position benoetigt
 return ball_time_to_y(y, balldata())
end

function blobtimetoy (y) --funktioniert in Ermangelung einer Zugriffsfunktion blobbyspeedy() nur vor dem Absprung :[
 return parabola_time_first(144.5, 14.5, -0.44, y)
end

function weiterleiten()
 moveto(200)
 jumpto(estimatey(200))
end

function netzroller() --Ist der Ball gefaehrdet, an der Netzkugel abzuprallen (0=nein, 1=ja auf der Seite des Bots, 2= auf der Seite des Gegners)
 if (361.5 < estimatex(323)) and (estimatex(323) < 438.5) then
  if (estimatex(323)<=400) then
   return 1
  else
   return 2
  end
 end
 -- otherwise, return 0
 return 0
end

function estimatey (x)
 return estimy(linear_time_first(ballx(), bspeedx(), x))
end

function touchable (t)
 local x=estimx(t)
 return (x <= CONST_BALL_LEFT_NET + CONST_BALL_RADIUS)
end

function opptouchable (t)
 local x=estimx(t)
 return (x >= CONST_BALL_RIGHT_NET - CONST_BALL_RADIUS)
end