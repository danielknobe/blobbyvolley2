-- this file provides an intermediary layer between the very simplistic c++ lua api and the 
-- lua api used by bots and rules.

-- derived constants that can be useful 
CONST_FIELD_MIDDLE		= CONST_FIELD_WIDTH / 2 						-- centre position

CONST_BALL_LEFT_BORDER	= CONST_BALL_RADIUS								-- minimum position of ball
CONST_BALL_RIGHT_BORDER	= CONST_FIELD_WIDTH - CONST_BALL_RADIUS			-- maximum position of ball

CONST_BALL_LEFT_NET		= CONST_FIELD_MIDDLE - CONST_BALL_RADIUS - CONST_NET_RADIUS
CONST_BALL_RIGHT_NET	= CONST_FIELD_MIDDLE + CONST_BALL_RADIUS + CONST_NET_RADIUS

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

-- all combined
function balldata()
	-- need x,y vars to expand to two results
	local x, y = get_ball_pos()
	return x, y, get_ball_vel()
end

-- this function mirrors pos around mirror
function mirror(pos, mirror)
	return 2*mirror - pos
end

-------------------------------------------------------------------------------------
--   utilities

-- emulate ?: operator
function select(cond, a, b)
	if cond then
		return a
	end
	return b
end

function make_unsigned(num)
	if num >= 0 then
		return num
	else
		return math.huge
	end
end
