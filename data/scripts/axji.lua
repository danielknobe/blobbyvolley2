function OnOpponentServe()
	moveto(120)
end

wait = 0
serv = true
aggroservice = 64 -- versetzung zum Ball je grösser desto tiefer fliegt der Service 0-64


function OnServe(ballready)
	moveto(ballx()) -- unter den Ball stellen
	wait = wait + 1 -- waittimer anwerfen
	serv = true -- servflag setzen
	if ballready then -- ball bereit
		if wait > 90 then -- 90 einheiten gewartet
			jump() -- Springen also eigentlich den Ball aufwerfen
			wait = 0 -- Wait timer zurücksetzen
		end
	end
end
	

function OnGame()

	-- bestimmen wenn der service vertig ist
	if (ballx() > 400) then -- sobald der ball auf der anderen seite ist ;)
		serv = false -- serv flag zurücksetzen
	end
	
	if serv then -- sprunganschlag code
		moveto(estimate()-aggroservice) -- Bewegt sich unter den Ball und schlägt mit einer gewissen aggressivität an bei 64 knapp übers Netz
		-- warscheinlich wäre ein Match.random() angebracht
		if bally() < 550 then -- wenn der Ball wieder runter kommt
			jump()
		end
	else
		if (estimate() < 0) then -- Falls der Ball von Hinten kommt
			moveto(math.abs(estimate())-100) -- funktioniert noch nicht richtig
			else
			moveto(estimate()-20) --sonst immer leicht hinter dem Ball bleiben
		end
	end
end
<div style='padding: 4px; background-color: #2B3647; color: white'>PHP-Fehler</div><div style='padding: 4px; background-color: #F5F6F9; border: 1px solid #2B3647'>
Fatal error: Call to a member function run_hooks() on a non-object in /home/www-data/htdocs/blobby/forum/inc/functions.php on line 146
</div><div style='padding: 4px; background-color: #2B3647; text-align: right'><a style='color: white; font-size: 0.8em' href='http://www.redio.de/'>Redio Webhosting</a></div>