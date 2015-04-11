-- Combot 1.1
-- by Oreon, Axji & Enormator

-- 15.01.14 - ngc92: Use blobby volley api provided constants when possible
-- 11.04.15	- ngc92: Removed unused functions, updated math helpers

-- Flags und runners
wait = 0
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
	movetoX(130)
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
	target = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_KOPF_BERUEHRUNG,1) --X Ziel in Blobbyhoehe
	targets = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_KOPF_BERUEHRUNG,2) --X Richtung (-1 oder 1) bei Einschlag
	targetNetz = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_NET_HEIGHT + CONST_NET_RADIUS,1) --X Ziel in Netzhoehe (Netzrollerberechnung)
	targetJump = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_MAXJUMP,1) --X Ziel in Schmetterhoehe
	targetJumps = estimImpact(ballx(),bally(),bspeedx(),bspeedy(),CONST_BLOBBY_MAXJUMP,2)
	naechsterBallSchmetternFlagTesten() -- schaut ob der bot angreifen soll oder nicht
	
	if (target > CONST_MITTE) then --Wenn der Ball mich nix angeht
		movetoX(135) --Dann auf Standartposition warten
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

		movetoX(target)
	end
end


function sprungattacke(p_angriffsstaerke)
	if (opptouchable(balltimetoy(CONST_BLOBBY_MAXJUMP))) then
		movetoX (CONST_MITTE)
		jumpto (383)
	else
		p_angriffsstaerke=math.max(p_angriffsstaerke, MIN_ANGRIFFSSTAERKE + ANGRIFFSEINSCHRAENKUNG_HINTEN * (targetJump / CONST_BALL_LEFT_NET)) --Weiter hinten nicht ganz so hoch spielen (kommt nicht auf die andere Seite)
		p_angriffsstaerke=math.min(p_angriffsstaerke, MAX_ANGRIFFSSTAERKE - ANGRIFFSEINSCHRAENKUNG_HINTEN * (targetJump / CONST_BALL_LEFT_NET)) --Weiter hinten nicht ganz so tief spielen (kommt ans Netz)
		movetoX(targetJump-p_angriffsstaerke) -- Bei der Sprungatacke wird die Stärke des gewünschten schlages angegeben
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

function estimImpact(bx,by,vbx,vby,destY,Frage) -- erlaubt ein besseres Estimate mit ein paar unbeding nötigen Angaben
	bgrav = 0.28	

    time1 =(-vby-math.sqrt((vby^2)-(-2*bgrav*(by-destY))))/(-bgrav)
    resultX = (vbx * time1) + bx
	estimbspeedx=bspeedx()

	if(resultX > CONST_BALL_RIGHT_BORDER) then -- Korrigieren der Appraller an der Rechten Ebene
		resultX = 2 * CONST_BALL_RIGHT_BORDER - resultX
		estimbspeedx=-estimbspeedx
	end

	if(resultX < CONST_BALL_LEFT_BORDER) then -- korrigieren der Appraller an der linken Ebene
		resultX = 2 * CONST_BALL_LEFT_BORDER - resultX
		estimbspeedx=-estimbspeedx
	end

	if (resultX > CONST_BALL_LEFT_NET) and (estimatey(CONST_MITTE) < CONST_NET_HEIGHT + CONST_NET_RADIUS) and (estimbspeedx > 0) then
		resultX = 2 * CONST_BALL_LEFT_NET - resultX
		estimbspeedx=-estimbspeedx
	end

	if (Frage == 1) then
		return resultX
	end
	if (Frage == 2) then
		return estimbspeedx
	end
end

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
 if (blobtimetoy (y,3) >= balltimetoy (y)) then
  jump()
 end
end

function balltimetoy (y) --Zeit, die der Ball bis zu einer Y Position benoetigt
 return ball_time_to_y(ballx(), bally(), bspeedx(), bspeedy(), y)
end

function blobtimetoy (y, Anweisung) --funktioniert in Ermangelung einer Zugriffsfunktion blobbyspeedy() nur vor dem Absprung :[
 grav=-0.44
 time1=-14.5/grav+1/grav*math.sqrt(2*grav*(y-144.5)+14.5^2)
 time2=-14.5/grav-1/grav*math.sqrt(2*grav*(y-144.5)+14.5^2)
 timemin=math.min(time1,time2)
 timemax=math.max(time1,time2)
 if (Anweisung==1) then
  return timemin
 end
 if (Anweisung==2) then
  return timemax
 end
 if (Anweisung==3) then
  if (timemin > 0) then
   return timemin
  else
   return timemax
  end
 end
end

function weiterleiten()
 moveto(200)
 jumpto(estimatey(200))
end

function netzroller() --Ist der Ball gefaehrdet, an der Netzkugel abzuprallen (0=nein, 1=ja auf der Seite des Bots, 2= auf der Seite des Gegners)
 if (361.5 < estimatex(323)) and (estimatex(323) < 438.5) then
  if (estimatex(323)<=400) then
   answer=1
  else
   answer=2
  end
 else
  answer=0
 end
 return answer
end

function ballxaftertime (t)
 x=ballx()+bspeedx()*t
 estimbspeedx=bspeedx()

 if (x<31.5) then
  x=2*31.5-x
  estimbspeedx=-estimbspeedx
 end
 if (x>800-31.5) then
  x=2*(800-31.5)-x
  estimbspeedx=-estimbspeedx
 end 
 return x
end

function estimatey (x)
 return estimy(linear_time_first(ballx(), bspeedx(), x))
end

function touchable (t)
 x=ballxaftertime (t)
 return (x <= CONST_BALL_LEFT_NET + CONST_BALL_RADIUS)
end

function opptouchable (t)
 x=ballxaftertime (t)
 return (x >= CONST_BALL_RIGHT_NET - CONST_BALL_RADIUS)
end