-- Combot 1.1
-- by Oreon, Axji & Enormator

-- 15.01.14 - ngc92: Use blobby volley api provided constants when possible
-- 11.04.15	- ngc92: Removed unused functions, updated math helpers
-- 18.02.23 - ngc92: fixed encoding, formatting; adapted constants

-- Flags und runners
naechsterBallSchmettern = true -- evtl Variablennamen wechseln

-- Charakter
CONST_ANGRIFFSGRUNDWERT_MIN = 30
CONST_ANGRIFFSGRUNDWERT_MAX = 55
CONST_WAITING_POSITION      = 135
MIN_ANGRIFFSSTAERKE = CONST_ANGRIFFSGRUNDWERT_MIN
MAX_ANGRIFFSSTAERKE = CONST_ANGRIFFSGRUNDWERT_MAX
ANGRIFFSEINSCHRAENKUNG_HINTEN = 10

-- sonstige Einstellungen
servexVersetzung=-6 --Wert ist so gewaehlt, dass der Ball nah ans Netz fliegt, der Gegner ihn aber grade nicht erreichen kann

-- ***ANFANG***

function OnOpponentServe()
	moveto(CONST_WAITING_POSITION)
	generatenaechsterBallSchmettern()
end

function OnServe(ballready)
	naechsterBallSchmettern = true
	generatenaechsterBallSchmettern()
	if ballready and moveto( ballx()+servexVersetzung ) then
		jump()
	end
end

function OnGame()
	local target = estimate_x_at_y(CONST_BALL_BLOBBY_HEAD) --X Ziel in Blobbyhoehe
	local targetNetz = estimate_x_at_y(CONST_NET_HEIGHT + CONST_NET_RADIUS) --X Ziel in Netzhoehe (Netzrollerberechnung)
	naechsterBallSchmetternFlagTesten() -- schaut ob der bot angreifen soll oder nicht

	if (target > CONST_FIELD_MIDDLE) then --Wenn der Ball mich nix angeht
		moveto(CONST_WAITING_POSITION) --Dann auf Standartposition warten
		generatenaechsterBallSchmettern() --Angriffsstaerke neu berechnen
	else
		if (targetNetz > CONST_BALL_LEFT_NET - 10) and targetNetz ~= math.huge then --Bei Netzroller einfach schmettern
			naechsterBallSchmettern = true
		end

		local targetJump, targetspeed = estimate_x_at_y(CONST_BLOBBY_MAX_JUMP) --X Ziel in Schmetterhoehe

		if naechsterBallSchmettern then
			if (targetspeed < 2) then
				sprungattacke(angriffsstaerke, targetJump)
			else
				weiterleiten()
			end
			return
		end

		moveto(target)
	end
end


function sprungattacke(p_angriffsstaerke, targetJump)
	if (opptouchable(ball_time_to_y(CONST_BLOBBY_MAX_JUMP))) then
		moveto (CONST_FIELD_MIDDLE)
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

	if (ballx() > CONST_FIELD_MIDDLE) then -- wenn der ball auf der Anderen Seite ist soll der bot nicht naechsterBallSchmettern sein
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
	angriffsstaerke = math.random(MIN_ANGRIFFSSTAERKE, MAX_ANGRIFFSSTAERKE)
end

function jumpto (y)
	if (blobtimetoy (y) + 4 >= ball_time_to_y (y)) then
		jump()
	end
end

function blobtimetoy (y) --funktioniert in Ermangelung einer Zugriffsfunktion blobbyspeedy() nur vor dem Absprung :[
	local t1, t2 = parabola_time_first(CONST_BLOBBY_GROUND_HEIGHT, -CONST_BLOBBY_JUMP, CONST_BLOBBY_GRAVITY / 2, y)
	return t1
end

function weiterleiten()
	moveto(200)
	jumpto(estimatey(200))
end

function estimatey (x)
	return estimy(ball_time_to_x(x))
end

function opptouchable (t)
	return (estimx(t) >= CONST_BALL_RIGHT_NET - CONST_BALL_RADIUS)
end