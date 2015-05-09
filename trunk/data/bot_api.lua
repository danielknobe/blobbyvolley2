-- this file provides an intermediary layer between the very simplistic c++ lua api and the 
-- lua bot api that can be used to program blobby volley 2 bots.

-- legacy functions
-- these function definitions make lua functions for the old api functions, which are sometimes more conveniente to use 
-- than their c api equivalent.

function posx()
	local x, y = get_blob_pos( LEFT_PLAYER )
	return x
end

function posy()
	local x, y = get_blob_pos( LEFT_PLAYER )
	return y
end

function touches()
	return get_touches( LEFT_PLAYER )
end

-- redefine launched to refer to the LEFT_PLAYER
_launched = launched
function launched()
	return _launched( LEFT_PLAYER )
end

function left()
	__WANT_LEFT = true
	__WANT_RIGHT = false
end

function right()
	__WANT_RIGHT = true
	__WANT_LEFT = false
end

function jump()
	__WANT_JUMP = true
end

function moveto(target)
	if target == nil then
		error("invalid target for moveto", 2)
	end
	local x = get_blob_pos( LEFT_PLAYER )
	if x < target - CONST_BLOBBY_SPEED/2 then
		right()
		return false
	elseif x > target + CONST_BLOBBY_SPEED/2 then
		left()
		return false
	else
		return true
	end
end

function oppx()
	local x, y = get_blob_pos( RIGHT_PLAYER )
	return x
end

function oppy()
	local x, y = get_blob_pos( RIGHT_PLAYER )
	return y
end

function getScore()
	return get_score( LEFT_PLAYER )
end

function getOppScore()
	return get_score( RIGHT_PLAYER )
end

-----------------------------------------------------------------------------------------------
-- helper functions

-- this function returns the first positive time that pos + vel*t + grav/2 * t² == destination. 
function parabola_time_first(pos, vel, grav, destination)
	local sq = vel^2 + 2*grav*(destination - pos);

	-- if unreachable, return inf
	if ( sq < 0 ) then
		return math.huge, math.huge
	end
	
	sq = math.sqrt(sq);
	
	local tmin = (-vel - sq) / grav;
	local tmax = (-vel + sq) / grav;
	
	if ( grav < 0 ) then
		tmin, tmax = tmax, tmin
	end

	if ( tmin > 0 ) then
		return tmin, tmax
	elseif ( tmax > 0 ) then
		return tmax, tmin
	else
		return math.huge, math.huge
	end
end

-- this function returns the first positive time that pos + vel*t  == destination. 
function linear_time_first(pos, vel, destination)
	assert(pos, "linear time first called with nil as position")
	if vel == 0 then
		return math.huge
	end
	return (destination - pos) / vel
end

-----------------------------------------------------------------------------------------
-- 						enhances ball prediction functions							   --
-----------------------------------------------------------------------------------------

-- calculates the time the ball needs to reach the specified x position
function ball_time_to_x( destination, posx, posy, velx, vely )
	return linear_time_first(posx, velx, destination)
end

-- calculates the time the blobby needs from its current position to destination
function blob_time_to_x (destination) 
	return math.abs(posx() - destination) / CONST_BLOBBY_SPEED
end

-- calculates the time the ball needs from pos to destination
function ball_time_to_y( destination, posx, posy, velx, vely )
	-- TODO this ignores net bounces
	return parabola_time_first( posy, vely, CONST_BALL_GRAVITY, destination )
end

-- old style estimate functions
function estimx(time)
	local straight = ballx() + time * bspeedx()
	-- correct wall impacts
	if(straight > CONST_BALL_RIGHT_BORDER) then
		return mirror(straight, CONST_BALL_RIGHT_BORDER), CONST_BALL_RIGHT_BORDER, -bspeedx()
	elseif(straight < CONST_BALL_LEFT_BORDER) then
		return mirror(straight, CONST_BALL_LEFT_BORDER), CONST_BALL_LEFT_BORDER, -bspeedx()
	else
		-- find net impacts
		if ballx() < CONST_BALL_LEFT_NET and straight > CONST_BALL_LEFT_NET then
			-- estimate where ball is when it reaches the net
			local timenet = ball_time_to_x( CONST_BALL_LEFT_NET, balldata())
			local y_at_net = estimy( timenet )
			if y_at_net < CONST_BALL_TOP_NET then
				return mirror(straight, CONST_BALL_LEFT_NET), CONST_BALL_LEFT_NET, -bspeedx()
			end
			-- otherwise, the ball might fall onto the net top
			timenet = ball_time_to_x( CONST_BALL_RIGHT_NET, balldata())
			y_at_net = estimy( timenet )
			-- TODO add net sphere handling! for now just assume net spere acts like net side
			if y_at_net < CONST_BALL_TOP_NET then
				return mirror(straight, CONST_BALL_LEFT_NET), CONST_BALL_LEFT_NET, -bspeedx()
			end
		elseif ballx() > CONST_BALL_RIGHT_NET and straight < CONST_BALL_RIGHT_NET then
			-- estimate where ball is when it reaches the net
			local timenet = ball_time_to_x( CONST_BALL_RIGHT_NET, balldata())
			local y_at_net = estimy( timenet )
			if y_at_net < CONST_BALL_TOP_NET then
				return mirror(straight, CONST_BALL_RIGHT_NET), CONST_BALL_RIGHT_NET, -bspeedx()
			end
			-- otherwise, the ball might fall onto the net top
			timenet = ball_time_to_x( CONST_BALL_LEFT_NET, balldata())
			y_at_net = estimy( timenet )
			-- TODO add net sphere handling! for now just assume net spere acts like net side
			if y_at_net < CONST_BALL_TOP_NET then
				return mirror(straight, CONST_BALL_RIGHT_NET), CONST_BALL_RIGHT_NET, -bspeedx()
			end
		end
		return straight, nil, bspeedx()
	end
end

function estimy(time)
	return bally() + time * bspeedy() + 0.5 * time^2 * CONST_BALL_GRAVITY
end