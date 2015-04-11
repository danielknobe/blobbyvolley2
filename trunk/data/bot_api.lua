-- this file provides an intermediary layer between the very simplistic c++ lua api and the 
-- lua bot api that can be used to program blobby volley 2 bots.

-- derived constants that can be useful 
CONST_FIELD_MIDDLE		= CONST_FIELD_WIDTH / 2 						-- centre position

CONST_BALL_LEFT_BORDER	= CONST_BALL_RADIUS								-- minimum position of ball
CONST_BALL_RIGHT_BORDER	= CONST_FIELD_WIDTH - CONST_BALL_RADIUS			-- maximum position of ball

CONST_BALL_LEFT_NET		= FIELD_MIDDLE - CONST_BALL_RADIUS - CONST_NET_RADIUS
CONST_BALL_RIGHT_NET	= FIELD_MIDDLE + CONST_BALL_RADIUS + CONST_NET_RADIUS

-- legacy functions
-- these function definitions make lua functions for the old api functions, which are sometimes more conveniente to use 
-- than their c api equivalent.

function ballx()
	local x, y = get_ball_pos()
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

-- old style estimate functions

function estimx(time)
	return ballx() + time * bspeedx()
end

function estimy(time)
	return bally() + time * bspeedy() + 0.5 * time^2 * CONST_BALL_GRAVITY
end