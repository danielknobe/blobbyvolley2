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

// This is exactly the half of the gravitation, i checked it in
// the original code
const float BLOBBY_JUMP_BUFFER = 0.44;
const float GRAVITATION = 0.88;
const float BLOBBY_JUMP_ACCELERATION = 15.1;


// Ball Settings
const float BALL_RADIUS = 31.5;
const float BALL_GRAVITATION = 0.28;
const float BALL_COLLISION_VELOCITY = 13.125;


// Volley Ball Net
const float NET_POSITION_X = RIGHT_PLANE / 2;
const float NET_POSITION_Y = 438;
const float NET_RADIUS = 7;
//const float NET_SPHERE = 154;		// what is the meaning of this value ???????
const float NET_SPHERE_POSITION = 284;

// Ground Settings
const float GROUND_PLANE_HEIGHT_MAX = 500;
const float GROUND_PLANE_HEIGHT = GROUND_PLANE_HEIGHT_MAX - BLOBBY_HEIGHT / 2.0;

