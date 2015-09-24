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
#include "ScriptedInputSource.h"

/* includes */
#include <cmath>
#include <algorithm>
#include <iostream>

#include <SDL2/SDL.h>

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

#include "DuelMatch.h"
#include "DuelMatchState.h"
#include "GameConstants.h"

/* implementation */

ScriptedInputSource::ScriptedInputSource(const std::string& filename, PlayerSide playerside, unsigned int difficulty)
: mDifficulty(difficulty)
, mSide(playerside)
, mDelayDistribution( difficulty/3, difficulty/2 )
{
	mStartTime = SDL_GetTicks();

	// set game constants
	setGameConstants();
	setGameFunctions();

	// push infos into script
	lua_pushnumber(mState, mDifficulty / 25.0);
	lua_setglobal(mState, "__DIFFICULTY");
	bool debug = true; /// \todo get this setting from option file
	lua_pushboolean(mState, debug);
	lua_setglobal(mState, "__DEBUG");

	openScript("api");
	openScript("bot_api");
	openScript(filename);

	// check whether all required lua functions are available
	bool step, onserve, ongame, onoppserve;
	step = getLuaFunction("__OnStep");
	onserve = getLuaFunction("__OnServe");
	ongame = getLuaFunction("__OnGame");
	onoppserve = getLuaFunction("__OnOpponentServe");
	if (!step || !onserve || !ongame ||!onoppserve)
	{
		std::string error_message = "Missing bot functions, check bot_api.lua! ";
		std::cerr << "Lua Error: " << error_message << std::endl;

		ScriptException except;
		except.luaerror = error_message;
		BOOST_THROW_EXCEPTION(except);
	}

	// clean up stack
	lua_pop(mState, lua_gettop(mState));
}

ScriptedInputSource::~ScriptedInputSource()
{
}

PlayerInputAbs ScriptedInputSource::getNextInput()
{
	bool serving = false;
	// reset input
	lua_pushboolean(mState, false);
	lua_setglobal(mState, "__WANT_LEFT");
	lua_pushboolean(mState, false);
	lua_setglobal(mState, "__WANT_RIGHT");
	lua_pushboolean(mState, false);
	lua_setglobal(mState, "__WANT_JUMP");

	lua_pushinteger(mState, mSide);
	lua_setglobal(mState, "__SIDE");

	if (getMatch() == 0)
	{
		return PlayerInputAbs();
	} else
	{
		IScriptableComponent::setMatch( const_cast<DuelMatch*>(getMatch()) );
	}
	lua_getglobal(mState, "__OnStep");
	callLuaFunction();

	if (!getMatch()->getBallActive() && mSide ==
			// if no player is serving player, assume the left one is
			(getMatch()->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : getMatch()->getServingPlayer() ))
	{
		serving = true;
		lua_getglobal(mState, "__OnServe");
		lua_pushboolean(mState, !getMatch()->getBallDown());
		callLuaFunction(1);
	}
	else if (!getMatch()->getBallActive() && mSide !=
			(getMatch()->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : getMatch()->getServingPlayer() ))
	{
		lua_getglobal(mState, "__OnOpponentServe");
		callLuaFunction();
	}
	else
	{
		lua_getglobal(mState, "__OnGame");
		callLuaFunction();
	}

	// read input info from lua script
	lua_getglobal(mState, "__WANT_LEFT");
	lua_getglobal(mState, "__WANT_RIGHT");
	lua_getglobal(mState, "__WANT_JUMP");
	bool wantleft = lua_toboolean(mState, -3);
	bool wantright = lua_toboolean(mState, -2);
	bool wantjump = lua_toboolean(mState, -1);
	lua_pop(mState, 3);

	int stacksize = lua_gettop(mState);
	if (stacksize > 0)
	{
		std::cerr << "Warning: Stack messed up!" << std::endl;
		std::cerr << "Element on stack is a ";
		std::cerr << lua_typename(mState, -1) << std::endl;
		lua_pop(mState, stacksize);
	}

	if (mStartTime + WAITING_TIME > SDL_GetTicks() && serving)
		return PlayerInputAbs();

	// random jump delay depending on difficulty
	if( wantjump && !mLastJump )
	{
		mJumpDelay--;
		if( mJumpDelay > 0 )
			wantjump = false;
		else
		{
			mJumpDelay = std::max(0.0, std::min( mDelayDistribution(mRandom) , (double)mDifficulty));
		}
	}

	mLastJump = wantjump;

	return PlayerInputAbs(wantleft, wantright, wantjump);
}
