-- most simple ruleset: for each mistake, the opponent gets a point
function OnMistake(player) 
	if( opponent(player) == servingplayer() ) then
		score(opponent(player))
	end
end 