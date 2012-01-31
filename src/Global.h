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

#pragma once

#include <string>
#include <exception>
#include <SDL/SDL_stdinc.h>

// I hope the GP2X is the only combination of these systems
#if defined(__linux__) && defined(__arm__)
#define GP2X GP2X
#endif

/*!	\def DEBUG
	\brief Enable debugging support
	\details when this marko is present, Blobby generates some additional debugging code
			usefull for tracking down bugs.
*/

const int BLOBBY_PORT = 1234;

const int BLOBBY_VERSION_MAJOR = 0;
const int BLOBBY_VERSION_MINOR = 98;

// will be set to 1.0 when we have our new replay format!
const unsigned char REPLAY_FILE_VERSION_MAJOR = 1;
const unsigned char REPLAY_FILE_VERSION_MINOR = 0;

const char AppTitle[] = "Blobby Volley 2 Version 1.0 RC1";

const float ROUND_START_SOUND_VOLUME = 0.2;
const float BALL_HIT_PLAYER_SOUND_VOLUME = 0.4;

// max. 1 ms additional latency, but much improved performance
const int RAKNET_THREAD_SLEEP_TIME = 1;

enum PlayerSide
{
	NO_PLAYER = -1,
	LEFT_PLAYER = 0,
	RIGHT_PLAYER = 1,
	//LEFT_PLAYER_2 = 2,
	//RIGHT_PLAYER_2 = 3,
	MAX_PLAYERS // This is always one more than the highest player enum
			// and can be used to declare arrays
};

enum InputDeviceName
{
	KEYBOARD = 1,
	MOUSE = 2,
	JOYSTICK = 3
};

/*! \class Color
	\brief represents RGB Colours
	\details This class represents colors as RGB with one byte for each channel.
*/
struct Color
{
	Color(int red, int green, int blue)
		: r(red), g(green), b(blue) {}
	
	/// \sa toInt()
	Color(unsigned int col)
		:r(col&0xff), g((col>>8)&0xff), b((col>>16)&0xff) {}
	Color() {}

	union
	{
		struct
		{
			Uint8 r;
			Uint8 g;
			Uint8 b;
		};
		Uint8 val[3];
	};
	
	bool operator == (Color rval) const
	{
		return !memcmp(val, rval.val, 3);
	}
	
	bool operator != (Color rval) const
	{
		return !(*this == rval);
	}

	unsigned int toInt() const
	{
		int i = 0;
		i |= r;
		i |= g << 8;
		i |= b << 16;
		return i;
	}

};


struct ExtensionUnsupportedException : public std::exception
{
	std::string extension;
	ExtensionUnsupportedException(std::string name) : extension(name) {}
	~ExtensionUnsupportedException() throw() {}
};

struct ScriptException : public std::exception
{
	std::string luaerror;
	~ScriptException() throw() {}
};

/// we need to define this constant to make it compile with strict c++98 mode
#undef M_PI
const double M_PI = 3.141592653589793238462643383279;
