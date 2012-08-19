/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/* header include */
#include "BotAPICalculations.h"

/* includes */
#include <cmath>
#include <limits>

#include "GameConstants.h"

/* implementation */

bool FLAG_BOUNCE = false;

// helpers
float time_to_x_direct(float pos, float vel, float destination);
float time_to_y_direct(float pos, float vel, float destination);
float parabel_time_first(float pos, float vel, float gravity, float destination);

float make_unsigned(float f) 
{
	return (f > 0 ? f : std::numeric_limits<float>::infinity());
}


void reset_flags() 
{
	FLAG_BOUNCE = false;
}

float time_to_x(const Vector2& pos, const Vector2& vel, float destination) 
{
	// check whether velocity is valid
	if( vel.x == 0 )
		return std::numeric_limits<float>::max();
	
	// direct?
	float timedirect = time_to_x_direct(pos.x, vel.x, destination);
	
	// needs wall bounce
	if( timedirect < 0 ) 
	{
		FLAG_BOUNCE = true;
		
		float wall = vel.x > 0 ? (RIGHT_PLANE - BALL_RADIUS) : BALL_RADIUS;
		float twall = time_to_x_direct(pos.x, vel.x, wall);
		float net = vel.x > 0 ? (NET_POSITION_X - BALL_RADIUS - NET_RADIUS) : (NET_POSITION_X + BALL_RADIUS + NET_RADIUS);
		float tnet = make_unsigned(time_to_x_direct(pos.x, vel.x, net));
		
		if ( tnet < twall ) 
		{
			Vector2 nhitpos = Vector2(net, predict_y(pos, vel, tnet));
			if ( nhitpos.y > NET_SPHERE_POSITION - NET_RADIUS - BALL_RADIUS )
				return tnet + time_to_x(nhitpos, vel.reflectX(), destination);
			
		}
		Vector2 whitpos = Vector2(wall, predict_y(pos, vel, twall));
		return twall + time_to_x(whitpos, vel.reflectX(), destination);
	}
	
	float net = vel.x > 0 ? (NET_POSITION_X - BALL_RADIUS - NET_RADIUS) : (NET_POSITION_X + BALL_RADIUS + NET_RADIUS);
	float tnet = make_unsigned(time_to_x_direct(pos.x, vel.x, net));
	Vector2 nhitpos = Vector2(net, predict_y(pos, vel, tnet));
	
	if ( tnet > timedirect || nhitpos.y < NET_SPHERE_POSITION - NET_RADIUS - BALL_RADIUS)
	{	
		// if ball is too high or destination is reached before net, no collision can occur
	 	return timedirect;
	}
	
	FLAG_BOUNCE = true;
	
	// if ball hits net on false side, it is impossible to reach its destination
	if( nhitpos.y > pos.y ) 
	{
		return std::numeric_limits<float>::max();
	}
	
	return tnet + time_to_x(nhitpos, vel.reflectX(), destination);
	
}

float time_to_y(const Vector2& pos, const Vector2& vel, float destination) 
{
	return time_to_y_direct(pos.y, vel.y, destination);
}

float predict_x(const Vector2& pos, const Vector2& vel, float time) 
{
	// can net collision occur
	float net = vel.x > 0 ? (NET_POSITION_X - BALL_RADIUS - NET_RADIUS) : (NET_POSITION_X + BALL_RADIUS + NET_RADIUS);
	float tnet = make_unsigned( time_to_x_direct(pos.x, vel.x, net) );
	
	// calculate estimated hitpos
	Vector2 nhitpos = Vector2(net, predict_y(pos, vel, tnet));
	
	// can ignore net bounce
	if ( tnet > time || nhitpos.y < NET_SPHERE_POSITION - NET_RADIUS - BALL_RADIUS) {
		float spos = pos.x + vel.x*time;
		if ( spos < BALL_RADIUS)
			return 2 * BALL_RADIUS - spos;
		else if ( spos > RIGHT_PLANE - BALL_RADIUS ) 
			return 2*(RIGHT_PLANE - BALL_RADIUS) - spos;
		
		return spos;
	}
	
	// collision with net
	return predict_x(nhitpos, vel.reflectX(), time - tnet);
	
	
}
float predict_y(const Vector2& pos, const Vector2& vel, float time) 
{
	return pos.y + (vel.y + BALL_GRAVITATION/2.0 * time) * time; 
}

float y_at_x(const Vector2& pos, const Vector2& vel, float destination) 
{
	float time = time_to_x(pos, vel, destination);
	return predict_y(pos, vel, time);
}
float x_at_y(const Vector2& pos, const Vector2& vel, float destination) 
{
	float time = time_to_y(pos, vel, destination);
	return predict_x(pos, vel, time);
}

float next_event(const Vector2& pos, const Vector2& vel) 
{
	// walls and net
	float time_wall;
	float time_net;
	if( vel.x > 0 ) 
	{
		time_wall = time_to_x_direct(pos.x, vel.x, RIGHT_PLANE - BALL_RADIUS);
		time_net = time_to_x_direct(pos.x, vel.x, NET_POSITION_X - NET_RADIUS - BALL_RADIUS);
	}
	 else 
	{
		time_wall = time_to_x_direct(pos.x, vel.x, LEFT_PLANE + BALL_RADIUS);
		time_net = time_to_x_direct(pos.x, vel.x, NET_POSITION_X + NET_RADIUS + BALL_RADIUS);
	}
	
	// ground
	float time_ground = time_to_y_direct(pos.y, vel.y, GROUND_PLANE_HEIGHT_MAX - BALL_RADIUS);

	time_net = make_unsigned(time_net);
	time_wall = make_unsigned(time_wall);
	time_ground = make_unsigned(time_ground);
	
	if ( time_net < time_wall && time_net < time_ground ) 
	{
		FLAG_BOUNCE = true;
		return time_net;
	} 
	 else if ( time_wall < time_net && time_wall < time_ground ) 
	{
		FLAG_BOUNCE = true;
		return time_wall;
	} 
	 else 
	{
		return time_ground;
	}
}

float time_to_x_direct(float pos, float vel, float destination) 
{
	return (destination - pos) / vel;
}

float time_to_y_direct(float pos, float vel, float destination) 
{
	return parabel_time_first(pos, vel, BALL_GRAVITATION, destination);
}

float parabel_time_first(float pos, float vel, float grav, float destination) 
{
	float sq = vel*vel + 2*grav*(destination - pos);

	// if unreachable, return -1
	if ( sq < 0 ) 
	{
		return -1;
	}
	sq = std::sqrt(sq);
	
	float tmin = (-vel - sq) / grav;
	float tmax = (-vel + sq) / grav;
	
	if ( grav < 0 ) 
	{
		float temp = tmin;
		tmin = tmax; tmax = temp;
	}

	if ( tmin > 0 ) 
		return tmin;
	else if ( tmax > 0 )
		return tmax;
		
	return -1;
}
