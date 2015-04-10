-- this file provides an intermediary layer between the very simplistic c++ lua api and the 
-- lua bot api that can be used to program blobby volley 2 bots.

-- legacy functions
-- these function definitions make lua functions for the old api functions, which are sometimes more conveniente to use 
-- than their c api equivalent.

function ballx()
	local x, y = get_ball_pos()
	debug(x)
	return x
end

function bally()
	local x, y = get_ball_pos()
	return y
end

function bspeedx()
	local x, y = get_ball_vel()
	return x
end

function bspeedy()
	local x, y = get_ball_vel()
	return y
end

function posx()
	local x, y = get_blob_pos( LEFT_PLAYER )
	return x
end

function posy()
	local x, y = get_blob_pos( LEFT_PLAYER )
	return y
end

function oppx()
	local x, y = get_blob_pos( RIGHT_PLAYER )
	return x
end

function oppy()
	local x, y = get_blob_pos( RIGHT_PLAYER )
	return y
end
