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

#include <cassert>
#include <cmath>

#include "GameLogic.h"

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

#include <physfs.h>
#include <iostream>

// copied from ScriptedInputSource
// TODO avoid code duplication

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




// how many steps must pass until the next hit can happen
const int SQUISH_TOLERANCE = 10;


CGameLogic::CGameLogic():	mLastError(NO_PLAYER), 
							mServingPlayer(NO_PLAYER), 
							mWinningPlayer(NO_PLAYER),
							mScoreToWin(15)
{
	// init clock
	clock.reset();
	clock.start();
	reset();
	
	// init lua
	mState = lua_open();
	lua_pushlightuserdata(mState, this);
	lua_setglobal(mState, "__GAME_LOGIC_POINTER");
	
	// add functions
	lua_register(mState, "score", luaScore);
	lua_register(mState, "opponent", luaGetOpponent);
	lua_register(mState, "servingplayer", luaGetServingPlayer);
	
	// now load ruleset
	ReaderInfo info;
	std::string filename = "rules.lua";
	info.handle = PHYSFS_openRead(filename.c_str());
	if (!info.handle)
	{
		throw FileLoadException(filename);
	}
	int error = lua_load(mState, chunkReader, &info, filename.c_str());
	PHYSFS_close(info.handle);
	if (error == 0)
		error = lua_pcall(mState, 0, 6, 0);
	
	//! \todo thats not good, needs a hardcoded fallback ruleset
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
	
	
	
}

void CGameLogic::setScoreToWin(int stw)
{
	if(stw > 0)
		mScoreToWin = stw;
}

int CGameLogic::getScoreToWin() const 
{
	return mScoreToWin;
}

void CGameLogic::onBallHitsGround(PlayerSide side)
{
	onError(side);
}

bool CGameLogic::isCollisionValid(PlayerSide side) const{
	// check whether the ball is squished
	return mSquish[side2index(side)] < 0;
}

void CGameLogic::onBallHitsPlayer(PlayerSide side)
{
	if(!isCollisionValid(side))
		return;
	
	// otherwise, set the squish value
	mSquish[side2index(side)] = SQUISH_TOLERANCE;
	
	// count the touches
	mTouches[side2index(other_side(side))] = 0;
	if(++(mTouches[side2index(side)]) > 3)
	{
		// if a player hits a forth time, it is an error
		onError(side);
	}
}

void CGameLogic::onPause()
{
	clock.stop();
}
void CGameLogic::onUnPause()
{
	clock.start();
}

void CGameLogic::step()
{
	clock.step();
	if(clock.isRunning())
	{
		--mSquish[0];
		--mSquish[1];
	}
}

int CGameLogic::getScore(PlayerSide side) const
{
	return mScores[side2index(side)];
}

void CGameLogic::setScore(PlayerSide side, int score)
{
	mScores[side2index(side)] = score;
}

int CGameLogic::getHits(PlayerSide side) const
{
	return mTouches[side2index(side)];
}

PlayerSide CGameLogic::getServingPlayer() const
{
	return mServingPlayer;
}

void CGameLogic::setServingPlayer(PlayerSide side)
{
	mServingPlayer = side;
}

PlayerSide CGameLogic::getWinningPlayer() const
{
	return mWinningPlayer;
}

PlayerSide CGameLogic::getLastErrorSide()
{
	PlayerSide t = mLastError;
	mLastError = NO_PLAYER;
	return t;
}

void CGameLogic::score(PlayerSide side)
{
	++mScores[side2index(side)];
	mTouches[0] = 0;
	mTouches[1] = 0;
	mWinningPlayer = checkWin();
}

PlayerSide CGameLogic::checkWin() const
{
	if(mScores[LEFT_PLAYER] >= mScoreToWin && mScores[LEFT_PLAYER] >= mScores[RIGHT_PLAYER] + 2)
		return LEFT_PLAYER;
	
	if(mScores[RIGHT_PLAYER] >= mScoreToWin && mScores[RIGHT_PLAYER] >= mScores[LEFT_PLAYER] + 2)
		return RIGHT_PLAYER;
	
	return NO_PLAYER;
}

void CGameLogic::reset()
{
	mScores[0] = 0;
	mScores[1] = 0;
	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
}

void CGameLogic::onError(PlayerSide side)
{
	mLastError = side;
	
	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
	
	// call lua scoring rules
	lua_getglobal(mState, "OnMistake");
	if(lua_isfunction(mState, -1)) 
	{
		lua_pushnumber(mState, side);
		if(lua_pcall(mState, 1, 0, 0)) 
		{
			std::cerr << "Lua Error: " << lua_tostring(mState, -1);
			std::cerr << std::endl;
		};
	}
	mServingPlayer = other_side(side);
}

GameLogic createGameLogic(RuleVersion rv)
{
	switch(rv){
		case OLD_RULES:
			return std::auto_ptr<CGameLogic>(new CGameLogic());
		case NEW_RULES:
			//return std::auto_ptr<CGameLogic>(new GLImplNR);
		default:
			assert(0);
	}
}

int CGameLogic::luaScore(lua_State* state) 
{
	int pl = int(lua_tonumber(state, -1) + 0.5);
	lua_pop(state, 1);
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	CGameLogic* gl = (CGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	gl->score((PlayerSide)pl);
	return 0;
}

int CGameLogic::luaGetOpponent(lua_State* state) 
{
	int pl = int(lua_tonumber(state, -1) + 0.5);
	lua_pop(state, 1);
	lua_pushnumber(state, other_side((PlayerSide)pl));
	return 1;
}

int CGameLogic::luaGetServingPlayer(lua_State* state) 
{
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	CGameLogic* gl = (CGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);

	lua_pushnumber(state, gl->getServingPlayer());
	return 1;
}
