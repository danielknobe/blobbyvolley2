-- most simple ruleset: for each mistake, the opponent gets a point
-- includes comments for documentation purposes

-- rules.lua doc
-- function OnMistake
--		IMPLEMENTED BY RULES.lua
--		called when a player makes a mistake
--		when this function is called, servinglayer() returns which player has
--		served (so it is not neccesarily the enemy of the player who made the mistake)
--		param: player - player who made the mistake
--		return: none

-- when a player makes a mistake, the other one gets a point if he was the serving player
function OnMistake(player) 
	--	function opponent
	--		PREDEFINED
	--		param:	player - player of whom you want to get the opponent
	--		return:	opponent of the player, so, for LEFT_PLAYER, RIGHT_PLAYER is returned and vice-versa
	
	-- function servingplayer
	--		PREDEFINED
	--		param: none
	--		return: which player has served
	if( opponent(player) == servingplayer() ) then
		--	function score
		--		PREDEFINED
		--		param:  player - player who gets a point
		--		return: none
		
		score(opponent(player))
	end
end 

--	function IsWinning
--		IMPLEMENTED BY RULES.lua
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




