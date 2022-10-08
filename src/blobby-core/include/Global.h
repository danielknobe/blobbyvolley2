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

#include <string>

/*!	\def DEBUG
	\brief Enable debugging support
	\details when this marco is present, Blobby generates some additional debugging code
			useful for tracking down bugs.
*/

const int BLOBBY_PORT = 1234;

const int BLOBBY_VERSION_MAJOR = 0;
const int BLOBBY_VERSION_MINOR = 105;

const float ROUND_START_SOUND_VOLUME = 0.2;
const float BALL_HIT_PLAYER_SOUND_VOLUME = 0.4;

// max. 1 ms additional latency, but much improved performance
const int RAKNET_THREAD_SLEEP_TIME = 1;
extern int CURRENT_NETWORK_LAG;

const std::string DEFAULT_RULES_FILE = "default.lua";

/// we need to define this constant to make it compile with strict c++98 mode
#undef M_PI
const double M_PI = 3.141592653589793238462643383279;
