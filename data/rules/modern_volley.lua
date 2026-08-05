__AUTHOR__ = "Blobby Volley 2 Developers"
__TITLE__  = "Modern Volley Rules"

function IsWinning(lscore, rscore)
	if lscore >= SCORE_TO_WIN and lscore >= rscore + 2 then
		return true
	end
	if rscore >= SCORE_TO_WIN and rscore >= lscore + 2 then
		return true
	end
	return false
end

function OnBallHitsPlayer(player)
	if touches(player) > 3 then
		mistake(player, opponent(player), 1)
	end
end

function OnBallHitsWall(player)
end

function OnBallHitsNet(player)
end

function OnBallHitsGround(player)
	mistake(player, opponent(player), 1)
end

function OnGame()
end

function HandleInput(player, left, right, up)
	return left, right, up
end
