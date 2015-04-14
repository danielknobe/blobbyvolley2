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

-----------------------------------------------------------------------------------------------
-- helper functions

-- this function returns the first positive time that pos + vel*t + grav/2 * t² == destination. 
function parabola_time_first(pos, vel, grav, destination)
	local sq = vel^2 + 2*grav*(destination - pos);

	-- if unreachable, return inf
	if ( sq < 0 ) then
		return math.huge
	end
	
	sq = math.sqrt(sq);
	
	local tmin = (-vel - sq) / grav;
	local tmax = (-vel + sq) / grav;
	
	if ( grav < 0 ) then
		tmin, tmax = tmax, tmin
	end

	if ( tmin > 0 ) then
		return tmin
	elseif ( tmax > 0 ) then
		return tmax
	else
		return math.huge
	end
end

-- this function returns the first positive time that pos + vel*t  == destination. 
function linear_time_first(pos, vel, destination)
	if vel == 0 then
		return math.huge
	end
	return (destination - pos) / vel
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

-----------------------------------------------------------------------------------------
-- 						enhances ball prediction functions							   --
-----------------------------------------------------------------------------------------

-- calculates the time the ball needs to reach the specified x position
function ball_time_to_x( posx, posy, velx, vely, destination)
	return linear_time_first(posx, velx, destination)
end

-- calculates the time the ball needs from pos to destination
function ball_time_to_y( posx, posy, velx, vely, destination )
	-- TODO this ignores net bounces
	return parabola_first_time( posy, vely, CONST_BALL_GRAVITY, destination )
end

-- old style estimate functions

function estimx(time)
	local straight = ballx() + time * bspeedx()
	-- correct wall impacts
	if(straight > CONST_BALL_RIGHT_BORDER) then
		return mirror(straight, CONST_BALL_RIGHT_BORDER), CONST_BALL_RIGHT_BORDER
	elseif(straight < CONST_BALL_LEFT_BORDER) then
		return mirror(resultX, CONST_BALL_LEFT_BORDER), CONST_BALL_LEFT_BORDER
	else
		return mirror, nil
	end
end

function estimy(time)
	return bally() + time * bspeedy() + 0.5 * time^2 * CONST_BALL_GRAVITY
end