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
const DuelMatch* ScriptedInputSource::mMatch = 0;
ScriptedInputSource* ScriptedInputSource::mCurrentSource = 0;

struct pos_x;
struct pos_y;
struct vel_x;
struct vel_y;

template<class T>
float convert(float f);

template<class T>
struct ScriptedInputSource::coordinate {
	coordinate(float f) : value(convert(f)) {
	}
	coordinate(double f) : value(convert(f)) {
	}

	operator float() const {
		return value;
	}

	float value;

	private:
		// other constructors ar prohibited !
		template<class U>
		coordinate(U u);

		static float convert(float f);
};

// functions for coodinate transformation
template<>
float ScriptedInputSource::coordinate<pos_x>::convert (float val) {
	return mCurrentSource->mSide == LEFT_PLAYER ? val : RIGHT_PLANE - val;
}

template<>
float ScriptedInputSource::coordinate<pos_y>::convert (float val) {
	return 600.f - val;
}
template<>
float ScriptedInputSource::coordinate<vel_x>::convert (float val) {
	return mCurrentSource->mSide == LEFT_PLAYER ? val : -val;
}
template<>
float ScriptedInputSource::coordinate<vel_y>::convert (float val) {
	return -val;
}

ScriptedInputSource::ScriptedInputSource(const std::string& filename, PlayerSide playerside, unsigned int difficulty)
: mMaxDelay(difficulty)
, mCurDelay(difficulty)
, mLastBallSpeed(0)
, mSide(playerside)
{
	mStartTime = SDL_GetTicks();

	// set game constants
	setGameConstants();
	setGameFunctions();


	//luaopen_math(mState);
	luaL_requiref(mState, "math", luaopen_math, 1);
	lua_register(mState, "touches", touches);
	lua_register(mState, "launched", launched);
	lua_register(mState, "jump", jump);
	lua_register(mState, "left", left);
	lua_register(mState, "right", right);
	lua_register(mState, "getScore", getScore);
	lua_register(mState, "getOppScore", getOppScore);
	lua_register(mState, "getScoreToWin", getScoreToWin);
	lua_register(mState, "getGameTime", getGameTime);

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

	// init delay
	mBallPositions.set_capacity(mMaxDelay + 1);
	mBallVelocities.set_capacity(mMaxDelay + 1);

	for(unsigned int i = 0; i < mMaxDelay + 1; ++i) {
		mBallPositions.push_back(Vector2(0,0));
		mBallVelocities.push_back(Vector2(0,0));
	}
}

ScriptedInputSource::~ScriptedInputSource()
{
}

PlayerInputAbs ScriptedInputSource::getNextInput()
{
	bool serving = false;
	// reset input
	mLeft = false;
	mRight = false;
	mJump = false;

	mCurrentSource = this;
	mMatch = getMatch();
	if (mMatch == 0)
	{
		return PlayerInputAbs();
	}

	if(mSide == LEFT_PLAYER)
		updateGameState( mMatch->getState() );
	else
	{
		auto s = mMatch->getState();
		s.swapSides();
		updateGameState( s );
	}

	// ball position and velocity update
	mBallPositions.push_back(mMatch->getBallPosition());
	mBallVelocities.push_back(mMatch->getBallVelocity());

	// adapt current delay
	char action = rand() % 8;
	switch(action) {
		case 0:
		case 1:
			mCurDelay--;
			break;
		case 2:
		case 3:
			mCurDelay++;
	}

	if ( mLastBallSpeed != getMatch()->getBallVelocity().x ) {
		mLastBallSpeed = getMatch()->getBallVelocity().x;
		// reaction time after bounce
		mCurDelay += rand() % (mMaxDelay+1);
	}

	if(mCurDelay == -1)
		mCurDelay = 0;
	if(mCurDelay > mMaxDelay)
		mCurDelay = mMaxDelay;

	int error = 0;

	if (!mMatch->getBallActive() && mSide ==
			// if no player is serving player, assume the left one is
			(mMatch->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : mMatch->getServingPlayer() ))
	{
		serving = true;
		lua_getglobal(mState, "OnServe");
		lua_pushboolean(mState, !mMatch->getBallDown());
		error = lua_pcall(mState, 1, 0, 0);
	}
	else if (!mMatch->getBallActive() && mCurrentSource->mSide !=
			(mMatch->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : mMatch->getServingPlayer() ))
	{
		lua_getglobal(mState, "OnOpponentServe");
		error = lua_pcall(mState, 0, 0, 0);
	}
	else
	{
		if ( mOnBounce && mLastBallSpeedVirtual != getBallVelocity().x ) {
			mLastBallSpeedVirtual = getBallVelocity().x;
			lua_getglobal(mState, "OnBounce");
			error = lua_pcall(mState, 0, 0, 0);
			if (error)
			{
				std::cerr << "Lua Error: " << lua_tostring(mState, -1);
				std::cerr << std::endl;
				lua_pop(mState, 1);
			}
		}
		lua_getglobal(mState, "OnGame");
		error = lua_pcall(mState, 0, 0, 0);
	}

	if (error)
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
		lua_pop(mState, 1);
	}

	// swap left/right if side is swapped
	if ( mSide == RIGHT_PLAYER )
		std::swap(mLeft, mRight);

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
	else
		return PlayerInputAbs(mLeft, mRight, mJump);
}

int ScriptedInputSource::touches(lua_State* state)
{
	lua_pushnumber(state, mMatch->getHitcount(mCurrentSource->mSide));
	return 1;
}

int ScriptedInputSource::launched(lua_State* state)
{
	lua_pushnumber(state, mMatch->getBlobJump(mCurrentSource->mSide));
	return 1;
}

int ScriptedInputSource::jump(lua_State* state)
{
	mCurrentSource->mJump = true;
	return 0;
}

int ScriptedInputSource::left(lua_State* state)
{
	mCurrentSource->mLeft = true;
	return 0;
}

int ScriptedInputSource::right(lua_State* state)
{
	mCurrentSource->mRight = true;
	return 0;
}

const Vector2& ScriptedInputSource::getBallPosition() {
	return mCurrentSource->mBallPositions[mCurrentSource->mMaxDelay - mCurrentSource->mCurDelay];
}
const Vector2& ScriptedInputSource::getBallVelocity() {
	return mCurrentSource->mBallVelocities[mCurrentSource->mMaxDelay - mCurrentSource->mCurDelay];
}

int ScriptedInputSource::getScore(lua_State* state)
{
	float score = mMatch->getScore( mCurrentSource->mSide );
	lua_pushnumber(state, score);
	return 1;
}

int ScriptedInputSource::getOppScore(lua_State* state)
{
	float score = mMatch->getScore( mCurrentSource->mSide == LEFT_PLAYER ? RIGHT_PLAYER: LEFT_PLAYER );
	lua_pushnumber(state, score);
	return 1;
}

int ScriptedInputSource::getScoreToWin(lua_State* state)
{
	float score = mMatch->getScoreToWin();
	lua_pushnumber(state, score);
	return 1;
}

int ScriptedInputSource::getGameTime(lua_State* state)
{
	float time = mMatch->getClock().getTime();
	lua_pushnumber(state, time);
	return 1;
}
