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

#pragma once

// encoding of events that can happen in the physics subsystem
struct MatchEvent
{
	// this characterizes the event type
	enum EventType
	{
		// physic events
		BALL_HIT_BLOB = 1,
		BALL_HIT_WALL,
		BALL_HIT_GROUND,
		BALL_HIT_NET,
		BALL_HIT_NET_TOP,
		// logic events
		PLAYER_ERROR,				// a player made an error
		RESET_BALL					// the ball was reset for next serve
	};

	// type of the event
	EventType event;
	// side on which the event happened
	PlayerSide side;
	// intensity of the event (only set if required by the event type)
	float intensity;

	MatchEvent(EventType e, PlayerSide s, float i = 0) : event(e), side(s), intensity(i)
	{

	}
};

