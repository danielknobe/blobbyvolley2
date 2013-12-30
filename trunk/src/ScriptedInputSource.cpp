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
#include "GameConstants.h"
#include "BotAPICalculations.h"
#include "FileRead.h"

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
	mState = luaL_newstate();
	
	// set game constants
	lua_pushnumber(mState, RIGHT_PLANE);
	lua_setglobal(mState, "CONST_FIELD_WIDTH");
	lua_pushnumber(mState, 600 - GROUND_PLANE_HEIGHT_MAX);
	lua_setglobal(mState, "CONST_GROUND_HEIGHT");
	lua_pushnumber(mState, -BALL_GRAVITATION);
	lua_setglobal(mState, "CONST_BALL_GRAVITY");
	lua_pushnumber(mState, BALL_RADIUS);
	lua_setglobal(mState, "CONST_BALL_RADIUS");
	lua_pushnumber(mState, BLOBBY_JUMP_ACCELERATION);
	lua_setglobal(mState, "CONST_BLOBBY_JUMP");
	lua_pushnumber(mState, BLOBBY_LOWER_RADIUS);
	lua_setglobal(mState, "CONST_BLOBBY_BODY_RADIUS");
	lua_pushnumber(mState, BLOBBY_UPPER_RADIUS);
	lua_setglobal(mState, "CONST_BLOBBY_HEAD_RADIUS");
	lua_pushnumber(mState, BLOBBY_HEIGHT);
	lua_setglobal(mState, "CONST_BLOBBY_HEIGHT");
	lua_pushnumber(mState, -GRAVITATION);
	lua_setglobal(mState, "CONST_BLOBBY_GRAVITY");
	lua_pushnumber(mState, 600 - NET_SPHERE_POSITION);
	lua_setglobal(mState, "CONST_NET_HEIGHT");
	lua_pushnumber(mState, NET_RADIUS);
	lua_setglobal(mState, "CONST_NET_RADIUS");
	
	
	//luaopen_math(mState);
	luaL_requiref(mState, "math", luaopen_math, 1);
	lua_register(mState, "touches", touches);
	lua_register(mState, "launched", launched);
	lua_register(mState, "debug", debug);
	lua_register(mState, "jump", jump);
	lua_register(mState, "moveto", moveto);
	lua_register(mState, "left", left);
	lua_register(mState, "right", right);
	lua_register(mState, "ballx", ballx);
	lua_register(mState, "bally", bally);
	lua_register(mState, "bspeedx", bspeedx);
	lua_register(mState, "bspeedy", bspeedy);
	lua_register(mState, "posx", posx);
	lua_register(mState, "posy", posy);
	lua_register(mState, "oppx", oppx);
	lua_register(mState, "oppy", oppy);
	lua_register(mState, "estimate", estimate);
	lua_register(mState, "estimx", estimx);
	lua_register(mState, "estimy", estimy);
	lua_register(mState, "timetox", timetox);
	lua_register(mState, "timetoy", timetoy);
	lua_register(mState, "predictx", predictx);
	lua_register(mState, "predicty", predicty);
	lua_register(mState, "xaty", xaty);
	lua_register(mState, "yatx", yatx);
	lua_register(mState, "nextevent", nextevent);
	lua_register(mState, "predictImpact", predictImpact);
	lua_register(mState, "getScore", getScore);
	lua_register(mState, "getOppScore", getOppScore);
	lua_register(mState, "getScoreToWin", getScoreToWin);	
	lua_register(mState, "getGameTime", getGameTime);
	
	//lua_register(mState, "parabel", parabel);

	int error = FileRead::readLuaScript(filename, mState);
	
	if (error == 0)
		error = lua_pcall(mState, 0, 6, 0);
	
	if (error)
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
		ScriptException except;
		except.luaerror = lua_tostring(mState, -1);
		lua_pop(mState, 1);
		lua_close(mState);
		BOOST_THROW_EXCEPTION(except);
	}
	
	
	// check whether all required lua functions are available
	bool onserve, ongame, onoppserve;
	lua_getglobal(mState, "OnServe");
	onserve = lua_isfunction(mState, -1);
	lua_getglobal(mState, "OnGame");
	ongame = lua_isfunction(mState, -1);
	lua_getglobal(mState, "OnOpponentServe");
	onoppserve = lua_isfunction(mState, -1);
	if (!onserve || !ongame ||!onoppserve)
	{
		std::string error_message = "Missing bot function ";
		error_message += onserve ? "" : "OnServe() ";
		error_message += ongame ? "" : "OnGame() ";
		error_message += onoppserve ? "" : "OnOpponentServe() ";
		std::cerr << "Lua Error: " << error_message << std::endl;
		
		ScriptException except;
		except.luaerror = error_message;
		lua_pop(mState, 1);
		lua_close(mState);
		BOOST_THROW_EXCEPTION(except);
	}
	
	// record which of the optional functions are available
	lua_getglobal(mState, "OnBounce");
	mOnBounce = lua_isfunction(mState, -1);
	
	if(!mOnBounce)
	{
		std::cerr << "Lua Warning: Missing function OnBounce" << std::endl;
	}
	
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
	lua_close(mState);
}

PlayerInput ScriptedInputSource::getNextInput()
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
		return PlayerInput();
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
	PlayerInput currentInput = PlayerInput(mLeft, mRight, mJump);

	int stacksize = lua_gettop(mState);
	if (stacksize > 0)
	{
		std::cerr << "Warning: Stack messed up!" << std::endl;
		std::cerr << "Element on stack is a ";
		std::cerr << lua_typename(mState, -1) << std::endl;
		lua_pop(mState, stacksize);
	}
	
	if (mStartTime + WAITING_TIME > SDL_GetTicks() && serving)
		return PlayerInput();
	else
		return currentInput;
}

void ScriptedInputSource::setflags(lua_State* state) {
	lua_pushnumber(state, FLAG_BOUNCE);
	lua_setglobal(state, "FLAG_BOUNCE");
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

int ScriptedInputSource::debug(lua_State* state)
{
	float number = lua_tonumber(state, -1);
	lua_pop(state, 1);
	std::cerr << "Lua Debug: " << number << std::endl;
	return 0;
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

int ScriptedInputSource::moveto(lua_State* state)
{
	float target = lua_tonumber(state, -1);
	lua_pop(state, 1);
	coordinate<pos_x> position = mMatch->getBlobPosition(mCurrentSource->mSide).x;

	if (position > target + 2)
		mCurrentSource->mLeft = true;
	if (position < target - 2)
		mCurrentSource->mRight = true;
	return 0;
}

const Vector2& ScriptedInputSource::getBallPosition() {
	return mCurrentSource->mBallPositions[mCurrentSource->mMaxDelay - mCurrentSource->mCurDelay];
}
const Vector2& ScriptedInputSource::getBallVelocity() {
	return mCurrentSource->mBallVelocities[mCurrentSource->mMaxDelay - mCurrentSource->mCurDelay];
}

int ScriptedInputSource::ballx(lua_State* state)
{
	coordinate<pos_x> pos = getBallPosition().x;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::bally(lua_State* state)
{
	coordinate<pos_y> pos = getBallPosition().y;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::bspeedx(lua_State* state)
{
	coordinate<vel_x> vel = getBallVelocity().x;
	lua_pushnumber(state, vel);
	return 1;
}

int ScriptedInputSource::bspeedy(lua_State* state)
{
	coordinate<vel_y> vel = getBallVelocity().y;
	lua_pushnumber(state, vel);
	return 1;
}

int ScriptedInputSource::posx(lua_State* state)
{
	coordinate<pos_x> pos = mMatch->getBlobPosition(mCurrentSource->mSide).x;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::posy(lua_State* state)
{
	coordinate<pos_y> pos = mMatch->getBlobPosition(mCurrentSource->mSide).y;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::oppx(lua_State* state)
{
	PlayerSide invPlayer = 
		mCurrentSource->mSide == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER;
	coordinate<pos_x> pos = mMatch->getBlobPosition(invPlayer).x;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::oppy(lua_State* state)
{
	PlayerSide invPlayer =
		mCurrentSource->mSide == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER;
	coordinate<pos_y> pos = mMatch->getBlobPosition(invPlayer).y;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::estimate(lua_State* state)
{
	static bool warning_issued = false;
	if( !warning_issued ) 
	{
		warning_issued = true;
		std::cerr << "Lua Warning: function estimate() is deprecated!" << std::endl;
	}
	
	Vector2 pos = getBallPosition();
	const Vector2& vel = getBallVelocity();
	
	float time = (vel.y - std::sqrt((vel.y * vel.y)- (-2 * BALL_GRAVITATION * (-pos.y + GROUND_PLANE_HEIGHT_MAX - BALL_RADIUS)))) / (-BALL_GRAVITATION);
	
	coordinate<pos_x> estim = pos.x + vel.x * time;
	
	lua_pushnumber(state, estim);
	return 1;
}

int ScriptedInputSource::estimx(lua_State* state)
{
	static bool warning_issued = false;
	if( !warning_issued ) 
	{
		warning_issued = true;
		std::cerr << "Lua Warning: function estimx() is deprecated!" << std::endl;
	}
	int num = lround(lua_tonumber(state, -1));
	lua_pop(state, 1);
	coordinate<pos_x> estim = getBallPosition().x + num * getBallVelocity().x;

	lua_pushnumber(state, estim);
	return 1;
}

int ScriptedInputSource::estimy(lua_State* state)
{
	static bool warning_issued = false;
	if( !warning_issued ) 
	{
		warning_issued = true;
		std::cerr << "Lua Warning: function estimy() is deprecated!" << std::endl;
	}
	int num = lround(lua_tonumber(state, -1));
	lua_pop(state, 1);
	coordinate<pos_y> estim = getBallPosition().y + num * (getBallVelocity().y + 0.5*BALL_GRAVITATION*num);
	lua_pushnumber(state, estim);
	return 1;
}

int ScriptedInputSource::predictx(lua_State* state) {
	reset_flags();
	float time = lua_tonumber(state, -1);
	lua_pop(state, 1);
	coordinate<pos_x> estim = predict_x(getBallPosition(), getBallVelocity(), time);
	lua_pushnumber(state, estim);
	setflags(state);
	return 1;
}
int ScriptedInputSource::predicty(lua_State* state) {
	reset_flags();
	float time = lua_tonumber(state, -1);
	lua_pop(state, 1);
	coordinate<pos_y> estim = predict_y(getBallPosition(), getBallVelocity(), time);
	lua_pushnumber(state, estim);
	setflags(state);
	return 1;
}
int ScriptedInputSource::timetox(lua_State* state) {
	reset_flags();
	coordinate<pos_x> destination = lua_tonumber(state, -1);
	lua_pop(state, 1);
	
	float time = time_to_x(getBallPosition(), getBallVelocity(), destination);
	
	lua_pushnumber(state, time);
	setflags(state);
	return 1;
}
int ScriptedInputSource::timetoy(lua_State* state) {
	reset_flags();
	coordinate<pos_y> destination = lua_tonumber(state, -1);
	lua_pop(state, 1);
	
	float time = time_to_y(getBallPosition(), getBallVelocity(), destination);
	
	lua_pushnumber(state, time);
	setflags(state);
	return 1;
}
int ScriptedInputSource::xaty(lua_State* state) {
	reset_flags();
	coordinate<pos_y> destination = lua_tonumber(state, -1);
	lua_pop(state, 1);
	
	coordinate<pos_x> x = x_at_y(getBallPosition(), getBallVelocity(), destination);
		
	lua_pushnumber(state, x);
	setflags(state);
	return 1;
}
int ScriptedInputSource::yatx(lua_State* state) {
	reset_flags();
	coordinate<pos_x> destination = lua_tonumber(state, -1);
	lua_pop(state, 1);
	
	coordinate<pos_y> y = y_at_x(getBallPosition(), getBallVelocity(), destination);

	lua_pushnumber(state, y);
	setflags(state);
	return 1;
}

int ScriptedInputSource::predictImpact(lua_State* state) {
	reset_flags();
	coordinate<pos_x> x = x_at_y(getBallPosition(), getBallVelocity(), GROUND_PLANE_HEIGHT_MAX - BLOBBY_HEIGHT - BALL_RADIUS);
	lua_pushnumber(state, x);
	setflags(state);
	return 1;
}

int ScriptedInputSource::nextevent(lua_State* state) {
	reset_flags();
	float time = next_event(getBallPosition(), getBallVelocity());
	lua_pushnumber(state, time);
	setflags(state);
	return 1;
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
