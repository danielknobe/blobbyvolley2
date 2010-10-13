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

#include "ScriptedInputSource.h"
#include "DuelMatch.h"
#include "GameConstants.h"

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

#include <SDL/SDL.h>
#include <physfs.h>
#include <cmath>

DuelMatch* ScriptedInputSource::mMatch = 0;
ScriptedInputSource* ScriptedInputSource::mCurrentSource = 0;

struct ReaderInfo
{
	PHYSFS_file* handle;
	char readBuffer[2048];
};

static const char* chunkReader(lua_State* state, void* data, size_t *size)
{
	ReaderInfo* info = (ReaderInfo*) data;
	int bytesRead = PHYSFS_read(info->handle, info->readBuffer, 1, 2048);
	*size = bytesRead;
	if (bytesRead == 0)
	{
		return 0;
	}
	else
	{
		return info->readBuffer;
	}
}

ScriptedInputSource::ScriptedInputSource(const std::string& filename,
						PlayerSide playerside): mLastBallSpeed(0)
{
	mStartTime = SDL_GetTicks();
	mState = lua_open();
	lua_pushnumber(mState, playerside);
	lua_setglobal(mState, "blobby_side");
	
	// set game constants
	lua_pushnumber(mState, RIGHT_PLANE);
	lua_setglobal(mState, "CONST_FIELD_WIDTH");
	lua_pushnumber(mState, 600 - GROUND_PLANE_HEIGHT_MAX);
	lua_setglobal(mState, "CONST_GROUND_HEIGHT");
	lua_pushnumber(mState, BALL_GRAVITATION);
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
	lua_pushnumber(mState, GRAVITATION);
	lua_setglobal(mState, "CONST_BLOBBY_GRAVITY");
	lua_pushnumber(mState, 600 - NET_SPHERE_POSITION);
	lua_setglobal(mState, "CONST_NET_HEIGHT");
	lua_pushnumber(mState, NET_RADIUS);
	lua_setglobal(mState, "CONST_NET_RADIUS");
	
	
	luaopen_math(mState);
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
	lua_register(mState, "predictImpact", predictImpact);
	lua_register(mState, "estimx", estimx);
	lua_register(mState, "estimy", estimy);
	lua_register(mState, "getScore", getScore);
	lua_register(mState, "getOppScore", getOppScore);
	lua_register(mState, "getScoreToWin", getScoreToWin);	
	lua_register(mState, "getGameTime", getGameTime);
	
	//lua_register(mState, "parabel", parabel);

	ReaderInfo info;
	info.handle = PHYSFS_openRead(filename.c_str());
	if (!info.handle)
	{
		throw FileLoadException(filename);
	}
	int error;
	error = lua_load(mState, chunkReader, &info, filename.c_str());
	PHYSFS_close(info.handle);
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
		throw except;
	}
	
	
	// check whether all required lua functions are available
	bool available = true;
	lua_getglobal(mState, "OnServe");
	available &= lua_isfunction(mState, -1);
	lua_getglobal(mState, "OnGame");
	available &= lua_isfunction(mState, -1);
	if (!available)
	{
		std::string error_message = "Missing bot function ";
		error_message += (lua_isfunction(mState, -1)) ? "OnServe()" : "OnGame()";
		std::cerr << "Lua Error: " << error_message << std::endl;
		
		ScriptException except;
		except.luaerror = error_message;
		lua_pop(mState, 1);
		lua_close(mState);
		throw except;
	}
	
	// record which of the optional functions are available
	lua_getglobal(mState, "OnOpponentServe");
	mOnOppServe = lua_isfunction(mState, -1);
	lua_getglobal(mState, "OnBounce");
	mOnBounce = lua_isfunction(mState, -1);
	
	if(!mOnOppServe)	std::cerr << "Lua Warning: Missing function OnOpponentServe" << std::endl; 
	if(!mOnBounce)		std::cerr << "Lua Warning: Missing function OnBounce" << std::endl; 
	
	lua_pop(mState, lua_gettop(mState));
}

ScriptedInputSource::~ScriptedInputSource()
{
	lua_close(mState);
}

PlayerInput ScriptedInputSource::getInput()
{
	bool serving = false;
	lua_pushboolean(mState, false);
	lua_pushboolean(mState, false);
	lua_pushboolean(mState, false);
	lua_setglobal(mState, "blobby_moveleft");
	lua_setglobal(mState, "blobby_moveright");
	lua_setglobal(mState, "blobby_moveup");

	mCurrentSource = this;
	mMatch = DuelMatch::getMainGame();
	if (mMatch == 0)
	{
		return PlayerInput();
	}
	
	// ball position and velocity update
	mBallPosition = mMatch->getBallPosition();
	mBallVelocity = mMatch->getBallVelocity();
	
	PlayerSide player = getSide(mState);
	int error = 0;
	
	if (!mMatch->getBallActive() && player == 
			// if no player is serving player, assume the left one is
			(mMatch->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : mMatch->getServingPlayer() ))
	{
		serving = true;
		lua_getglobal(mState, "OnServe");
		lua_pushboolean(mState, !mMatch->getBallDown());
		error = lua_pcall(mState, 1, 0, 0);
	}
	else if (!mMatch->getBallActive() && player != 
			(mMatch->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : mMatch->getServingPlayer() ))
	{
		if(mOnOppServe)
		{
			lua_getglobal(mState, "OnOpponentServe");
			error = lua_pcall(mState, 0, 0, 0);
		}
	}
	else
	{
		if ( mOnBounce && mLastBallSpeed != DuelMatch::getMainGame()->getBallVelocity().x ) {
			mLastBallSpeed = DuelMatch::getMainGame()->getBallVelocity().x;
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

	lua_getglobal(mState, "blobby_moveleft");
	lua_getglobal(mState, "blobby_moveright");
	lua_getglobal(mState, "blobby_moveup");
	bool wantLeft = lua_toboolean(mState, -3);
	bool wantRight = lua_toboolean(mState, -2);
	bool wantUp = lua_toboolean(mState, -1);
	lua_pop(mState, 3);
	PlayerInput currentInput = PlayerInput(wantLeft, wantRight, wantUp);

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

PlayerSide ScriptedInputSource::getSide(lua_State* state)
{
	lua_getglobal(state, "blobby_side");
	PlayerSide player = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	return player;
}

int ScriptedInputSource::touches(lua_State* state)
{
	lua_getglobal(state, "blobby_side");
	PlayerSide player = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, -1);
	lua_pushnumber(state, mMatch->getHitcount(player));
	return 1;
}

int ScriptedInputSource::launched(lua_State* state)
{
	lua_pushnumber(state, mMatch->getBlobJump(getSide(state)));
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
	lua_pushboolean(state, true);
	lua_setglobal(state, "blobby_moveup");
	return 0;
}

int ScriptedInputSource::left(lua_State* state)
{
	lua_pushboolean(state, true);
	if (getSide(state) == LEFT_PLAYER)
		lua_setglobal(state, "blobby_moveleft");
	else if (getSide(state) == RIGHT_PLAYER)
		lua_setglobal(state, "blobby_moveright");
	else
		lua_pop(state, 1);
	return 0;
}

int ScriptedInputSource::right(lua_State* state)
{
	lua_pushboolean(state, true);
	if (getSide(state) == LEFT_PLAYER)
		lua_setglobal(state, "blobby_moveright");
	else if (getSide(state) == RIGHT_PLAYER)
		lua_setglobal(state, "blobby_moveleft");
	else
		lua_pop(state, 1);
	return 0;
}

int ScriptedInputSource::moveto(lua_State* state)
{
	float target = lua_tonumber(state, -1);
	lua_pop(state, 1);
	PlayerSide player = getSide(state);
	float position = mMatch->getBlobPosition(player).x;
	if (player == RIGHT_PLAYER)
	{
//			target = 800.0 - target;
		position = 800.0 - position;
	}
	if (position > target + 2)
		left(state);
	if (position < target - 2)
		right(state);
	return 0;
}

const Vector2& ScriptedInputSource::getBallPosition() {
	return mCurrentSource->mBallPosition;
}
const Vector2& ScriptedInputSource::getBallVelocity() {
	return mCurrentSource->mBallVelocity;
}

int ScriptedInputSource::ballx(lua_State* state)
{
	float pos = getBallPosition().x;
	if (getSide(state) == RIGHT_PLAYER)
		pos = 800.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::bally(lua_State* state)
{
	float pos = getBallPosition().y;
	pos = 600.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::bspeedx(lua_State* state)
{
	float vel = getBallVelocity().x;
	if (getSide(state) == RIGHT_PLAYER)
		vel = -vel;
	lua_pushnumber(state, vel);
	return 1;
}

int ScriptedInputSource::bspeedy(lua_State* state)
{
	float vel = getBallVelocity().y;
	vel = -vel;
	lua_pushnumber(state, vel);
	return 1;
}

int ScriptedInputSource::posx(lua_State* state)
{
	PlayerSide player = getSide(state);
	float pos = mMatch->getBlobPosition(player).x;
	if (player == RIGHT_PLAYER)
		pos = 800.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::posy(lua_State* state)
{
	lua_getglobal(state, "blobby_side");
	PlayerSide player = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	float pos = mMatch->getBlobPosition(player).y;
	pos = 600.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::oppx(lua_State* state)
{
	PlayerSide player = getSide(state);
	PlayerSide invPlayer = 
		player == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER;
	float pos = mMatch->getBlobPosition(invPlayer).x;
	if (invPlayer == LEFT_PLAYER)
	pos = 800.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::oppy(lua_State* state)
{
	PlayerSide player = getSide(state);

	PlayerSide invPlayer =
		player == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER;
	float pos = mMatch->getBlobPosition(invPlayer).y;
	pos = 600.0 - pos;
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
		
	float estim = estimateBallImpact(GROUND_PLANE_HEIGHT_MAX - BALL_RADIUS);
	if (getSide(state) == RIGHT_PLAYER)
		estim = 800.0 - estim;
	lua_pushnumber(state, estim);
	return 1;
}

int ScriptedInputSource::predictImpact(lua_State* state) {
	float estim = estimateBallImpact(GROUND_PLANE_HEIGHT_MAX - BALL_RADIUS - BLOBBY_HEIGHT);
	if (getSide(state) == RIGHT_PLAYER)
		estim = 800.0 - estim;
	lua_pushnumber(state, estim);
	return 1;
}

int ScriptedInputSource::estimx(lua_State* state)
{
	int num = lround(lua_tonumber(state, -1));
	lua_pop(state, 1);
	float estim = calculateBallEstimation_bad(num).x;
	if (getSide(state) == RIGHT_PLAYER)
		estim = 800.0 - estim;
	lua_pushnumber(state, estim);
	return 1;
}

int ScriptedInputSource::estimy(lua_State* state)
{
	int num = lround(lua_tonumber(state, -1));
	lua_pop(state, 1);
	float estim = calculateBallEstimation_bad(num).y;
	estim = 600.0 - estim;
	lua_pushnumber(state, estim);
	return 1;
}

int ScriptedInputSource::getScore(lua_State* state) 
{
	float score = mMatch->getScore( getSide(state) );
	lua_pushnumber(state, score);
	return 1;
}

int ScriptedInputSource::getOppScore(lua_State* state) 
{
	float score = mMatch->getScore( getSide(state) == LEFT_PLAYER ? RIGHT_PLAYER: LEFT_PLAYER );
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

float ScriptedInputSource::estimateBallImpact(float target) {
	const Vector2& vel = getBallVelocity();
	const Vector2& pos = getBallPosition();
	float steps = (vel.y - std::sqrt((vel.y * vel.y)- (-2 * BALL_GRAVITATION * (-pos.y + target)))) / (-BALL_GRAVITATION);
	return calculateBallEstimation(steps).x;
}
/*
int ScriptedInputSource::parabel(lua_State* state) 
{
	// load arguments
	
	// MODE:	0 - first positive solution
	//			1 - second positive solution
	int mode = lua_tonumber(state, -1);
	lua_pop(state, 1);
	
	float distance = lua_tonumber(state, -1);
	lua_pop(state, 1);
	float velocity = lua_tonumber(state, -1);
	lua_pop(state, 1);
	float gravity = lua_tonumber(state, -1);
	lua_pop(state, 1);
	
	float sq = velocity*velocity + 2*gravity*distance;

	// if unreachable, return -1
	if ( sq < 0 ) {
		lua_pushnumber(state, -1);
		return 1;
	}
	sq = sqrt(sq);
	
	float tmin = (-velocity - sq) / gravity;
	float tmax = (-velocity + sq) / gravity;
	
	if ( gravity < 0 ) {
		float temp = tmin;
		tmin = tmax; tmax = temp;
	}

	if ( mode == 0 ) {
		if ( tmin > 0 ) 
			lua_pushnumber(state, tmin);
		else if ( tmax > 0 )
			lua_pushnumber(state, tmax);
		else 
			lua_pushnumber(state, -1);
		
	} else if ( mode == 1 ) {
		if ( tmax > 0 ) 
			lua_pushnumber(state, tmax);
		else 
			lua_pushnumber(state, -1);
	}
	return 1;
}
*/
Vector2 ScriptedInputSource::calculateBallEstimation_bad(float time) {
	Vector2 ret = getBallPosition();
	const Vector2& vel = getBallVelocity();
	ret.x += vel.x * time;
	ret.y += (vel.y + 0.5 * (BALL_GRAVITATION * time)) * time;
	return ret;
}

Vector2 ScriptedInputSource::calculateBallEstimation(float time) {
	Vector2 ret = getBallPosition();
	const Vector2& vel = getBallVelocity();
	
	// simple calculation
	ret.x += vel.x * time;
	ret.y += (vel.y + 0.5 * (BALL_GRAVITATION * time)) * time;
	
	// improvment: border collission
	if ( ret.x < BALL_RADIUS )
		ret.x = 2 * BALL_RADIUS - ret.x;
	if ( ret.x > RIGHT_PLANE - BALL_RADIUS ) 
		ret.x = 2 * (RIGHT_PLANE - BALL_RADIUS) - ret.x;
		
	return ret;
}
