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

#include <iostream>

// This struct exists for easy exchange of a single player input frame

struct PlayerInput
{
	PlayerInput()
	{
		left = false;
		right = false;
		up = false;
	}
	
	PlayerInput(const std::string& string);
	
	PlayerInput(bool l, bool r, bool u)
	{
		left = l;
		right = r;
		up = u;
	}
	
	void swap()
	{
		bool tmp = left;
		left = right;
		right = tmp;	
	}

	bool left;
	bool right;
	bool up;
};

// This class abstracts several possible input sources, like local input 
// from InputManager, input from a scripted player or input over network
// which in turn can all be recorded over a decorator
// It should be only called once per frame because some implementations
// may use this to activate a refresh routine on their actual source

class InputSource
{
public:
	virtual PlayerInput getInput() = 0;
	virtual ~InputSource()
	{
	}
};

// This class serves as a dummy input source.
// It can optionally be set from outside if low level input access
// is required at a higher level

class DummyInputSource : public InputSource
{
public:
	virtual PlayerInput getInput()
	{
		return mInput;
	}
	virtual ~DummyInputSource()
	{
	}
	
	void setInput(PlayerInput input)
	{
		mInput = input;
	}
	
private:
	PlayerInput mInput;
};

// This operator converts a PlayerInput structure in a packed string
// suitable for saving

std::string& operator<< (std::string& out, const PlayerInput& input);
