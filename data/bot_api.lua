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
-- flags, that determine what estimate takes into account
__e_wall_bounce = true
__e_net_bounce  = true
__e_net_error   = true -- this variable controls net errors

-- debug data
__LastBounces = {}
__OppServe = false

__OPPSIDE = opponent(__SIDE)

-- legacy functions
-- these function definitions make lua functions for the old api functions, which are sometimes more conveniente to use 
-- than their c api equivalent.

function posx()
	local x, y = get_blob_pos( __SIDE )
	if __SIDE == RIGHT_PLAYER then
		return CONST_FIELD_WIDTH - x
	else
		return x
	end
end

function posy()
	local x, y = get_blob_pos( __SIDE )
	return y
end

function touches()
	return get_touches( __SIDE )
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
__balldata__ = balldata
-- this is the internal function that does the side correction
function __balldata()
	local x, y, vx, vy = __balldata__()
	if __SIDE == RIGHT_PLAYER then
		x = CONST_FIELD_WIDTH - x
		vx = -vx
	end
	return x, y, vx, vy
end

-- this is the function to be used by bots, which includes the difficulty changes
function balldata()
	return __bx, __by, __bvx, __bvy
end

-- redefine launched to refer to the __SIDE player
__launched = launched
function launched()
	return __launched( __SIDE )
end

function left()
	__WANT_LEFT  = __SIDE == LEFT_PLAYER
	__WANT_RIGHT = __SIDE ~= LEFT_PLAYER
end

function right()
	__WANT_LEFT  = __SIDE ~= LEFT_PLAYER
	__WANT_RIGHT = __SIDE == LEFT_PLAYER
end

function jump()
	__WANT_JUMP = true
end

function moveto(target)
	if target == nil then
		error("invalid target for moveto", 2)
	end
	local x = posx()
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
	local x, y = get_blob_pos( __OPPSIDE )
	if __SIDE == RIGHT_PLAYER then
		return CONST_FIELD_WIDTH - x
	else
		return x
	end
end

function oppy()
	local x, y = get_blob_pos( __OPPSIDE )
	return y
end

function getScore()
	return get_score( __SIDE )
end

function getOppScore()
	return get_score( __OPPSIDE )
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

-- advances the ball without calculating interactions
function advance_ball( time, posx, posy, velx, vely )
	return posx + time * velx, posy + time * vely + 0.5 * time^2 * CONST_BALL_GRAVITY, velx, vely + time * CONST_BALL_GRAVITY 
end

-- generic estimate function that advances the whole ball state
function estimate(time, posx, posy, velx, vely)
	__PERF_ESTIMATE_COUNTER = __PERF_ESTIMATE_COUNTER + 1
	if __PERF_ESTIMATE_COUNTER > 100 then
		print("OVERLOAD", posx, posy, velx, vely, time)
		return
	end
--	print("estim", time, posx, posy, velx, vely)
	-- read parameters and set correct defaults
	-- TODO actually, it makes only sense to set them all, or none. should we check that somewhere?
	-- TODO re-introduce errors
	posx = posx or ballx()
	velx = velx or bspeedx()
	posy = posy or bally()
	vely = vely or bspeedy()
	
	if time <= 0 then
		return posx, posy, velx, vely
	end
	
	-- first, check where the ball would be if nothing happened
	local straight = posx + time * velx
	
	-- estimate important events
	local time_wall_right  = linear_time_first( posx, velx, CONST_BALL_RIGHT_BORDER )
	local time_wall_left   = linear_time_first( posx, velx, CONST_BALL_LEFT_BORDER )
	local time_left_net    = linear_time_first( posx, velx, CONST_BALL_LEFT_NET )
	local time_right_net   = linear_time_first( posx, velx, CONST_BALL_RIGHT_NET )
	local time_net         = math.min(time_left_net, time_right_net)
	
	-- detect and handle wall collisions
	if time_wall_right < time_net and time_wall_right < time and velx > 0 then
		posx, posy, velx, vely = advance_ball( time_wall_right, posx, posy, velx, vely )
--		print("hit wall")
		return estimate( time - time_wall_right, posx - 0.0001 * velx, posy, -velx, vely )
	elseif time_wall_left < time_net and time_wall_left < time and velx < 0 then
		posx, posy, velx, vely = advance_ball( time_wall_left, posx, posy, velx, vely )
--		print("hit wall")
		return estimate( time - time_wall_left, posx - 0.0001 * velx, posy, -velx, vely )
	end
	
	-- detect net collisions
	-- the easy case: the ball crosses the net boundary
	if time_net < time then
		local netside = 0
		local otherside = 0
		if velx > 0 then
			netside = CONST_BALL_LEFT_NET
			otherside = CONST_BALL_RIGHT_NET
		else
			netside = CONST_BALL_RIGHT_NET
			otherside = CONST_BALL_LEFT_NET
		end
		
		posx, posy, velx, vely = advance_ball( time_net, posx, posy, velx, vely )
		
		-- where does the ball hit the net (or does it pass over it)
		if posy < CONST_NET_HEIGHT then
--			print("hit side", time, time_net, posx, posy, velx, vely)
			-- definitely hit the side of the net. easy
			return estimate( time - time_net, posx - 0.0001 * velx, posy, -velx, vely )
		end
		
		local tcross = linear_time_first(posx, velx, otherside)
		tcross = math.min(tcross, time - time_net)
		local ycross = posy + tcross * vely + 0.5 * tcross^2 * CONST_BALL_GRAVITY
		-- the ugly! need to simulate here. we get some errors, as we have worked with non discrete times before!
		if posy < CONST_BALL_TOP_NET or ycross < CONST_BALL_TOP_NET then
			local dt
			posx, posy, velx, vely, dt = simulate_ball_vs_nettop(posx, posy, velx, vely, tcross )
--			print("netsphere", time - time_net - dt)
			posx, posy, velx, vely = estimate( time - time_net - dt, posx, posy, velx, vely )
			return posx, posy, velx, vely
		end
		
		-- we need to use the corrected time here since we have changed posx etc
		-- advance a tiny bit, to ensure we do net get multi intersections
--		print("escaped net", time_net, tcross, posx, velx, otherside)
		-- simulate till tcross without estimte to prevent single stepping
		posx, posy, velx, vely = advance_ball( tcross + 0.0001, posx, posy, velx, vely )
		-- then do the real simulation
		return estimate( time - time_net - tcross, posx, posy, velx, vely )
	
	-- the other case: the ball is above the net the whole time
	elseif math.abs(posx - CONST_FIELD_MIDDLE) < CONST_BALL_RADIUS + CONST_NET_RADIUS then
		local maxt = ball_time_to_y(CONST_NET_HEIGHT - CONST_BALL_RADIUS, posx, posy, velx, vely)
		maxt = math.min(time, maxt)
		posx, posy, velx, vely, maxt = simulate_ball_vs_nettop(posx, posy, velx, vely, maxt)
--		print("within", posx, posy, velx, vely, time, maxt, CONST_NET_HEIGHT - CONST_BALL_RADIUS)
		-- if we started inside the net, perform one step to slip out!
		if maxt == 0 then
			posx = posx + velx
			posy = posy + vely + 0.5 * CONST_BALL_GRAVITY
			vely = CONST_BALL_GRAVITY
			maxt = 1
		end
		return estimate( time - maxt, posx, posy, velx, vely )
	end
	
	-- nothing happens, nice.
	return advance_ball( time, posx, posy, velx, vely )
end

-- this simulates the ball for at most maxtime timesteps and checks if it hits the stationary sphere
function ball_hit_sphere(ballx, bally, velx, vely, posx, posy, radius, maxtime)
	local rsq = radius + CONST_BALL_RADIUS
	rsq = rsq * rsq
	for t = 0,maxtime do
		local dx = ballx - posx
		local dy = bally - posy
		if dx^2 + dy^2 < rsq then
			return t, ballx, bally, velx, vely
		end
		ballx = ballx + velx
		bally = bally + vely + 0.5 * CONST_BALL_GRAVITY
		vely = vely + CONST_BALL_GRAVITY
	end
	
	return math.huge, ballx, bally, velx, vely
end

function simulate_ball_vs_nettop(posx, posy, velx, vely, maxtime)
	local tsp
	tsp, posx, posy, velx, vely = ball_hit_sphere(posx, posy, velx, vely, CONST_FIELD_MIDDLE, CONST_NET_HEIGHT, CONST_NET_RADIUS, maxtime)
	if tsp ~= math.huge then
		-- this calculation is really sensitive to small positional changes! it will only give a crude estimate about the long term behaviour of
		-- the ball
		local nx = posx - CONST_FIELD_MIDDLE
		local ny = posy - CONST_NET_HEIGHT
		local n = math.sqrt(nx*nx+ny*ny)
		nx, ny = nx / n, ny / n
		local dot =  nx * velx + ny * vely
		local ep = dot^2
		local el = velx * velx + vely * vely - ep
		local ns = math.sqrt( 0.7 * ep + 0.9 * el )
		
		velx = velx - 2 * dot * nx
		vely = vely - 2 * dot * ny
		n = math.sqrt((0.7*ep + 0.9*el) / (velx^2 + vely^2))
		
		velx = velx * n
		vely = vely * n
		
		-- shift the ball out of the net
		posx = CONST_FIELD_MIDDLE - nx * (CONST_BALL_RADIUS + CONST_NET_RADIUS + 0.001)
		posy = CONST_NET_HEIGHT - ny * (CONST_BALL_RADIUS + CONST_NET_RADIUS + 0.001)
		
		return posx, posy, velx, vely, tsp
	end
	
	return posx, posy, velx, vely, maxtime
end

-- old style estimate functions, using the new, advanced function for implementation
function estimx(time, posx, posy, velx, vely)
	posx, posy, velx, vely = estimate(time, posx, posy, velx, vely)
	return posx, velx
end

function estimy(time, posx, posy, velx, vely)
	posx, posy, velx, vely = estimate(time, posx, posy, velx, vely)
	return posy, vely
end

-- really useful estimate functions
function estimate_x_at_y(height, posx, posy, velx, vely)
	local time = ball_time_to_y(height, posx, posy, velx, vely)
	if time == math.huge then
		return math.huge, math.huge, math.huge
	end
	posx, posy, velx, vely = estimate(time, posx, posy, velx, vely)
	-- this works, because the only possible change to the estimated time
	-- can happen at the net sphere, and that only increases time!
	if math.abs( posy - height ) > 5  then
--		print("recurse", posy, height)
		posx, velx, t = estimate_x_at_y(height, posx, posy, velx, vely)
		return posx, velx, t + time
	end
	return posx, velx, time
end

---------------------------------------------------------------------------------------------

-- this function is called every game step from the C++ api
__lastBallSpeed = nil
function __OnStep()
	__PERF_ESTIMATE_COUNTER = 0 -- count the calls to estimate!
	__bx, __by, __bvx, __bvy = __balldata()
	local original_bvx = __bvx
	-- add some random noise to the ball info, if we have difficulty enabled
	if __DIFFICULTY > 0 then
		__bx  = __bx + __ebx * __DIFFICULTY
		__by  = __by + __eby * __DIFFICULTY
		__bvx = __bvx + __ebvx * __DIFFICULTY
		__bvy = __bvy + __ebvy * __DIFFICULTY
	end
	
	if __lastBallSpeed == nil then __lastBallSpeed = original_bvx end
	
	-- if the x velocity of the ball changed, it bounced and we call the listener function
	if __lastBallSpeed ~= original_bvx and is_ball_valid() then
		__lastBallSpeed = original_bvx
		-- update the error variables
		local er = math.random() * CONST_BALL_RADIUS
		local phi = 2*math.pi * math.random()
		__ebx = math.sin(phi) * er
		__eby = math.cos(phi) * er
		er = math.random() * 1.5 -- this would be about 10% error
		phi = 2*math.pi * math.random()
		__ebvx = math.sin(phi) * er
		__ebvy = math.cos(phi) * er
		
		-- enable and disable bounce predictions
		__e_wall_bounce = (math.random()*__DIFFICULTY - 0.75) <= 0 				-- diff 1: 25% - diff 0.75: 0%
		__e_net_bounce  = (math.random()*__DIFFICULTY * 0.5/0.66 - 0.50) <= 0 	-- diff 1: 50% -- diff 0.66: 0%
		-- todo review the error values
		
		
		-- debug data.
		if __DEBUG and is_game_running() and ballx() > CONST_FIELD_MIDDLE - CONST_BALL_RADIUS then
			local bounce = capture_situation_data()
			for i = 5, 2, -1 do
				__LastBounces[i] = __LastBounces[i-1]
			end
			__LastBounces[1] = bounce
		end
		
		if OnBounce then
			OnBounce()
		end
	end
	
	
	-- call the action functions
	if not is_game_running() then
		
		-- get the serving player. if NO_PLAYER, the ball is on the left.
		local server = get_serving_player()
		if server == NO_PLAYER then 
			server = LEFT_PLAYER
		end
		
		if __SIDE == server then
			OnServe( is_ball_valid() )
		else
			-- print last ball information when the opponent serves
			if __DEBUG then
				if not __OppServe then
					for k=1, #__LastBounces do
						print("step ", k)
						for i, v in pairs(__LastBounces[k]) do
							print(i, v)
						end
					end
					__OppServe = true
				end
			end
			
			set_ball_data(404.71929931641, 428.68881225586, -11.259716033936, 6.7393469810486)
			--set_blob_data(0, 50, 144.5, 0, 0)
			--print( estimx( ) )
			
			if OnOpponentServe then
				OnOpponentServe()
			end
		end
	else
		__OppServe = false
		OnGame()
	end
	
	--print(__PERF_ESTIMATE_COUNTER)
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
	local result = (destination - pos) / vel
	if result < 0 then
		return math.huge
	else 
		return result
	end
end

-- debug function: capture situational information
function capture_situation_data()
	local x, y, vx, vy = __balldata()
	local data = {}
	data['ballx'] = x
	data['bally'] = y
	data['bspeedx'] = vx
	data['bspeedy'] = vy
	data['posx'] = posx()
	data['posy'] = posy()
	data['touches'] = touches()
	return data
end
