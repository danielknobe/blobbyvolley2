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
, mLastBallSpeed(0)
, mSide(playerside)
, mDelayDistribution( difficulty/3, difficulty/2 )
, mPositionErrorDistribution( 0.0, difficulty / 75.0 * BALL_RADIUS )
{
	mStartTime = SDL_GetTicks();

	// set game constants
	setGameConstants();
	setGameFunctions();

	openScript("api");
	openScript("bot_api");
	openScript(filename);

	// check whether all required lua functions are available
	bool onserve, ongame, onoppserve;
	onserve = getLuaFunction("OnServe");
	ongame = getLuaFunction("OnGame");
	onoppserve = getLuaFunction("OnOpponentServe");
	if (!onserve || !ongame ||!onoppserve)
	{
		std::string error_message = "Missing bot function ";
		error_message += onserve ? "" : "OnServe() ";
		error_message += ongame ? "" : "OnGame() ";
		error_message += onoppserve ? "" : "OnOpponentServe() ";
		std::cerr << "Lua Error: " << error_message << std::endl;

		ScriptException except;
		except.luaerror = error_message;
		BOOST_THROW_EXCEPTION(except);
	}

	// record which of the optional functions are available
	mOnBounce = getLuaFunction("OnBounce");

	if(!mOnBounce)
	{
		std::cerr << "Lua Warning: Missing function OnBounce" << std::endl;
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

	if (getMatch() == 0)
	{
		return PlayerInputAbs();
	}

	// collect match states
	auto matchstate = getMatch()->getState();
	if(mSide != LEFT_PLAYER)
		matchstate.swapSides();

	matchstate.worldState.ballPosition += mBallPosError;
	matchstate.worldState.ballVelocity *= Vector2(mBallVelError, mBallVelError);
	updateGameState( matchstate );

	// detect bounce
	if ( mLastBallSpeed != getMatch()->getBallVelocity().x )
	{
		// change ball pos error
		mBallPosError.x = mPositionErrorDistribution(mRandom);
		mBallPosError.y = mPositionErrorDistribution(mRandom);
		mBallVelError = 1.0 + mPositionErrorDistribution(mRandom) / (2*BALL_RADIUS);
		// clamp max vel error
		mBallVelError = std::max(0.85, std::min(1.15, mBallVelError) );

		mLastBallSpeed = getMatch()->getBallVelocity().x;
		if ( mOnBounce )
		{
			lua_getglobal(mState, "OnBounce");
			callLuaFunction();
		}
	}

	if (!getMatch()->getBallActive() && mSide ==
			// if no player is serving player, assume the left one is
			(getMatch()->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : getMatch()->getServingPlayer() ))
	{
		serving = true;
		lua_getglobal(mState, "OnServe");
		lua_pushboolean(mState, !getMatch()->getBallDown());
		callLuaFunction(1);
	}
	else if (!getMatch()->getBallActive() && mSide !=
			(getMatch()->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : getMatch()->getServingPlayer() ))
	{
		lua_getglobal(mState, "OnOpponentServe");
		callLuaFunction();
	}
	else
	{
		lua_getglobal(mState, "OnGame");
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

	// swap left/right if side is swapped
	if ( mSide == RIGHT_PLAYER )
		std::swap(wantleft, wantright);

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
