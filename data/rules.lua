-- most simple ruleset: for each mistake, the opponent gets a point
-- includes comments for documentation purposes


__AUTHOR__ = "Blobby Volley 2 Developers"
__TITLE__  = "BV2 Default Rules"

-- rules.lua doc
-- function OnMistake
--		IMPLEMENTED BY RULES.lua
--		called when a player makes a mistake
--		when this function is called, servingplayer() returns which player has
--		served (so it is not neccesarily the enemy of the player who made the mistake)
--		param: player - player who made the mistake
--		return: none
function OnMistake(player) 
	--	function opponent
	--		PREDEFINED
	--		param:	player - player of whom you want to get the opponent
	--		return:	opponent of the player, so, for LEFT_PLAYER, RIGHT_PLAYER is returned and vice-versa
	
	--	function score
	--		PREDEFINED
	--		params:		player - player who gets points
	--					serve - true to make a new serve, false to continue playing
	--					amount - how many points player gets
	--		return: none
	
	score(opponent(player), true, 1)
end 

--	function IsWinning
--		IMPLEMENTED BY rules.lua
--		called when it is determined whether a player has won
--		params:		lscore: score of left player
--					rscore: score of right player
--		return: whether a player has won
function IsWinning(lscore, rscore) 
	-- constant SCORE_TO_WIN: number of points for a player to win
	if lscore >= SCORE_TO_WIN and lscore >= rscore + 2 then
		return true
	end
	if rscore >= SCORE_TO_WIN and rscore >= lscore + 2 then
		return true
	end
	return false
end

-- function OnBallHitsPlayer
--		IMPLEMENTEDBY rules.lua
--		called when a valid collision between a player and the ball happens.
--		params: player - the player that hit the ball
--		return: none
function OnBallHitsPlayer(player)
	--	function touches
	--		PREDEFINED
	--		param:	player - player whos touches you want to get
	--		return:	how many touches did player
	
	--	function mistake
	--		PREDEFINED
	--		param:  player - player who made a mistake
	--		return: none
	
	if touches(player) > 3 then
		mistake(player)
	end
end

-- function OnBallHitsWall
--		IMPLEMENTEDBY rules.lua
--		called when a valid collision between the ball and a wall happens.
--		params: player - the player on whos side the ball hit a wall
--		return: none
function OnBallHitsWall(player)
end

-- function OnBallHitsNet
--		IMPLEMENTEDBY rules.lua
--		called when a valid collision between the ball and a net happens.
--		params: player - the player on whos side the ball hits a net (NO_PLAYER for net top)
--		return: none
function OnBallHitsNet(player)
end

-- function OnBallHitsGround
--		IMPLEMENTEDBY rules.lua
--		called when the ball hits the ground.
--		params: player - the player on whos side the ball hits the ground
--		return: none
function OnBallHitsGround(player)
	mistake(player)
end

-- function OnGame
--		IMPLEMENTEDBY rules.lua
--		called for every moment in the game.
--		params: none
--		return: none
function OnGame()
end

-- function HandleInput
--		IMPLEMENTEDBY rules.lua
--		called to control player movement
--		params: player - the player to check
--				left, right, up - source user input
--		return: new input values
function HandleInput(player, left, right, up)
	return left, right, up
end

-- uncomment this to change number of points for a player to win
-- SCORE_TO_WIN = 15
