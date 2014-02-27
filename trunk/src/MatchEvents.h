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

enum
{
	// physic events
	EVENT_LEFT_BLOBBY_HIT = 1,
	EVENT_RIGHT_BLOBBY_HIT = 2,
	EVENT_BALL_HIT_LEFT_GROUND = 4,
	EVENT_BALL_HIT_RIGHT_GROUND = 8,
	EVENT_BALL_HIT_LEFT_WALL = 16,
	EVENT_BALL_HIT_RIGHT_WALL = 32,
	EVENT_BALL_HIT_NET_LEFT = 64,
	EVENT_BALL_HIT_NET_RIGHT = 128,
	EVENT_BALL_HIT_NET_TOP = 256,
	EVENT_BALL_HIT_GROUND = EVENT_BALL_HIT_LEFT_GROUND | EVENT_BALL_HIT_RIGHT_GROUND,

	// game events
	EVENT_ERROR_LEFT = 512,
	EVENT_ERROR_RIGHT = 1024,
 	EVENT_ERROR = EVENT_ERROR_LEFT | EVENT_ERROR_RIGHT,
	EVENT_RESET = 2048
};
