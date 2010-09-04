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

#include "Global.h"
#include "InputSource.h"

#include <iostream>

// ScriptedInputSource provides an implementation of InputSource, which uses
// Lua scripts to get its input. The given script is automatically initialised
// and provided with an interface to the game.

// The API documentation can now be found in doc/ScriptAPI.txt

// The time the bot waits after game start
const int WAITING_TIME = 1500;

class lua_State;

class ScriptedInputSource : public InputSource
{
public:
	// The constructor automatically loads and initializes the script
	// with the given filename. The side parameter tells the script
	// which side is it on.
	ScriptedInputSource(const std::string& filename, PlayerSide side);
	~ScriptedInputSource();
	
	virtual PlayerInput getInput();
	
private:
	static PlayerSide getSide(lua_State* state);

	static int side(lua_State* state);
	static int touches(lua_State* state);
	static int balldown(lua_State* state);
	static int launched(lua_State* state);
	static int debug(lua_State* state);
	static int jump(lua_State* state);
	static int left(lua_State* state);
	static int right(lua_State* state);
	static int moveto(lua_State* state);
	static int ballx(lua_State* state);
	static int bally(lua_State* state);
	static int bspeedx(lua_State* state);
	static int bspeedy(lua_State* state);
	static int posx(lua_State* state);
	static int posy(lua_State* state);
	static int oppx(lua_State* state);
	static int oppy(lua_State* state);
	static int estimate(lua_State* state);
	static int estimx(lua_State* state);
	static int estimy(lua_State* state);
	static int getScore(lua_State* state);
	static int getOppScore(lua_State* state);
	static int getScoreToWin(lua_State* state);
	static int getGameTime(lua_State* state);

	lua_State* mState;
	unsigned int mStartTime;
};
