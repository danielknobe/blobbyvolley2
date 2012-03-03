/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

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


// Difficulty Settings
const float BALL_SPEED_FACTOR = 1.00;

// Border Settings
const float LEFT_PLANE = 0;
const float RIGHT_PLANE = 800.0;
// These numbers should include the blobbys width, but in the original game
// the blobbys can go a bit into the walls too.


// Blobby Settings
const float BLOBBY_HEIGHT = 89;
//const float BLOBBY_WIDTH = 75;		// what is the meaning of this value ???????
const float BLOBBY_UPPER_SPHERE = 19;
const float BLOBBY_UPPER_RADIUS = 25;
const float BLOBBY_LOWER_SPHERE = 13;
const float BLOBBY_LOWER_RADIUS = 33;

// Ground Settings
const float GROUND_PLANE_HEIGHT_MAX = 500;
const float GROUND_PLANE_HEIGHT = GROUND_PLANE_HEIGHT_MAX - BLOBBY_HEIGHT / 2.0;


// This is exactly the half of the gravitation, i checked it in
// the original code
const float BLOBBY_MAX_JUMP_HEIGHT = GROUND_PLANE_HEIGHT - 206.375;	// GROUND_Y - MAX_Y
const float BLOBBY_JUMP_ACCELERATION = 15.1;

// these values are calculated from the other two
const float GRAVITATION = BLOBBY_JUMP_ACCELERATION * BLOBBY_JUMP_ACCELERATION / BLOBBY_MAX_JUMP_HEIGHT;
const float BLOBBY_JUMP_BUFFER = GRAVITATION / 2;


// Ball Settings
const float BALL_RADIUS = 31.5;
const float BALL_GRAVITATION = 0.287 * BALL_SPEED_FACTOR * BALL_SPEED_FACTOR;
const float BALL_COLLISION_VELOCITY = std::sqrt(0.75 * RIGHT_PLANE * BALL_GRAVITATION); /// \todo work on a full-fledged physics spec


// Volley Ball Net
const float NET_POSITION_X = RIGHT_PLANE / 2;
const float NET_POSITION_Y = 438;
const float NET_RADIUS = 7;
//const float NET_SPHERE = 154;		// what is the meaning of this value ???????
const float NET_SPHERE_POSITION = 284;


