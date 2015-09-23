-- this file provides an intermediary layer between the very simplistic c++ lua api and the 
-- lua bot api that can be used to program blobby volley 2 bots.

-- in these global variables, we save the ball info for each step
__bx = 0
__by = 0
__bvx = 0
__bvy = 0
-- these save the artificial errors used to reduce the bot strength
__ebx = 0
__eby = 0
__ebvx = 0
__ebvy = 0

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

-- redefine the ball coordinate functions to use the cached values
function ballx()
	return __bx
end

function bally()
	return __by
end

function bspeedx()
	return __bvx
end

function bspeedy()
	return __bvy
end

-- we need to save balldata somewhere to use it in here
__balldata = balldata
function balldata()
	return __bx, __by, __bvx, __bvy
end

-- redefine launched to refer to the LEFT_PLAYER
__launched = launched
function launched()
	return __launched( LEFT_PLAYER )
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

-----------------------------------------------------------------------------------------
-- 						enhances ball prediction functions							   --
-----------------------------------------------------------------------------------------

-- calculates the time the ball needs to reach the specified x position.
-- the parameters posy and vely are currently not used, but are present to
-- ensure that the same group of parameters can be used like for any other 
-- estimate like function
function ball_time_to_x( destination, posx, posy, velx, vely )
	if destination == nil then error("invalid destination specified for ball_time_to_x") end
	
	posx = posx or ballx()
	velx = velx or bspeedx()
	return linear_time_first(posx, velx, destination)
end

-- calculates the time the blobby needs from its current position to destination
function blob_time_to_x (destination) 
	return math.abs(posx() - destination) / CONST_BLOBBY_SPEED
end

-- calculates the time the ball needs from pos to destination
function ball_time_to_y( destination, posx, posy, velx, vely )
	posy = posy or bally()
	vely = vely or bspeedy()
	-- TODO this ignores net bounces
	return parabola_time_first( posy, vely, CONST_BALL_GRAVITY, destination )
end

-- old style estimate functions
function estimx(time, posx, posy, velx, vely)
	-- read parameters and set correct defaults
	-- TODO actually, it makes only sense to set them all, or none. should we check that somewhere?
	posx = posx or ballx()
	velx = velx or bspeedx()
	posy = posy or bally()
	vely = vely or bspeedy()
	
	local straight = posx + time * velx
	-- correct wall impacts
	if(straight > CONST_BALL_RIGHT_BORDER) then
		return mirror(straight, CONST_BALL_RIGHT_BORDER), CONST_BALL_RIGHT_BORDER, -velx
	elseif(straight < CONST_BALL_LEFT_BORDER) then
		return mirror(straight, CONST_BALL_LEFT_BORDER), CONST_BALL_LEFT_BORDER, -velx
	else
		-- find net impacts
		if posx < CONST_BALL_LEFT_NET and straight > CONST_BALL_LEFT_NET then
			-- estimate where ball is when it reaches the net
			local timenet = ball_time_to_x( CONST_BALL_LEFT_NET, posx, posy, velx, vely)
			local y_at_net = estimy( timenet )
			if y_at_net < CONST_BALL_TOP_NET then
				return mirror(straight, CONST_BALL_LEFT_NET), CONST_BALL_LEFT_NET, -velx
			end
			-- otherwise, the ball might fall onto the net top
			timenet = ball_time_to_x( CONST_BALL_RIGHT_NET, posx, posy, velx, vely)
			y_at_net = estimy( timenet )
			-- TODO add net sphere handling! for now just assume net spere acts like net side
			if y_at_net < CONST_BALL_TOP_NET then
				return mirror(straight, CONST_BALL_LEFT_NET), CONST_BALL_LEFT_NET, -velx
			end
		elseif posx > CONST_BALL_RIGHT_NET and straight < CONST_BALL_RIGHT_NET then
			-- estimate where ball is when it reaches the net
			local timenet = ball_time_to_x( CONST_BALL_RIGHT_NET, posx, posy, velx, vely)
			local y_at_net = estimy( timenet )
			if y_at_net < CONST_BALL_TOP_NET then
				return mirror(straight, CONST_BALL_RIGHT_NET), CONST_BALL_RIGHT_NET, -velx
			end
			-- otherwise, the ball might fall onto the net top
			timenet = ball_time_to_x( CONST_BALL_LEFT_NET, posx, posy, velx, vely)
			y_at_net = estimy( timenet )
			-- TODO add net sphere handling! for now just assume net spere acts like net side
			if y_at_net < CONST_BALL_TOP_NET then
				return mirror(straight, CONST_BALL_RIGHT_NET), CONST_BALL_RIGHT_NET, -velx
			end
		end
		return straight, nil, velx
	end
end

function estimy(time)
	return bally() + time * bspeedy() + 0.5 * time^2 * CONST_BALL_GRAVITY
end

---------------------------------------------------------------------------------------------

-- this function is called every game step from the C++ api
__lastBallSpeed = nil
function __OnStep()
	__bx, __by, __bvx, __bvy = __balldata()
	-- add some random noise to the ball info, if we have difficulty enabled
	if __DIFFICULTY > 0 then
		__bx  = __bx + __ebx * __DIFFICULTY
		__by  = __by + __eby * __DIFFICULTY
		__bvx = __bvx + __ebvx * __DIFFICULTY
		__bvy = __bvy + __ebvy * __DIFFICULTY
	end
	
	if __lastBallSpeed == nil then __lastBallSpeed = __bvx end
	
	-- if the x velocity of the ball changed, it bounced and we call the listener function
	if __lastBallSpeed ~= __bvx then
		__lastBallSpeed = __bvx
		-- update the error variables
		local er = math.random() * CONST_BALL_RADIUS
		local phi = 2*math.pi * math.random()
		__ebx = math.sin(phi) * er
		__eby = math.cos(phi) * er
		er = math.random() * 1.5 -- this would be about 10% error
		phi = 2*math.pi * math.random()
		__ebvx = math.sin(phi) * er
		__ebvy = math.cos(phi) * er
		if OnBounce then
			OnBounce()
		end
	end
end

-- OnOpponentServe, called script function if it exists
function __OnOpponentServe()
	if OnOpponentServe then
		OnOpponentServe()
	end
end

function __OnServe( ballready )
	OnServe(ballready)
end

function __OnGame()
	OnGame()
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