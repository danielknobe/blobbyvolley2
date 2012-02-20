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

#include "FileRead.h"
#include <iostream>

// copied from ScriptedInputSource
/// \todo avoid code duplication

struct ReaderInfo
{
	FileRead file;
	char buffer[2048];
};

static const char* chunkReader(lua_State* state, void* data, size_t *size)
{
	ReaderInfo* info = (ReaderInfo*) data;
	
	int bytesRead = 2048;
	if(info->file.length() - info->file.tell() < 2048)
	{
		bytesRead = info->file.length() - info->file.tell();
	}
	
	info->file.readRawBytes(info->buffer, bytesRead);
	// if this doesn't throw, bytesRead is the actual number of bytes read
	/// \todo we must do sth about this code, its just plains awful. 
	/// 		File interface has to be improved to support such buffered reading.
	*size = bytesRead;
	if (bytesRead == 0)
	{
		return 0;
	}
	else
	{
		return info->buffer;
	}
}




/// how many steps must pass until the next hit can happen
const int SQUISH_TOLERANCE = 11;


IGameLogic::IGameLogic():	mLastError(NO_PLAYER), 
							mServingPlayer(NO_PLAYER), 
							mWinningPlayer(NO_PLAYER),
							mScoreToWin(15)
{
	// init clock
	clock.reset();
	clock.start();
	reset();
}

IGameLogic::~IGameLogic() {
	// nothing to do
}

int IGameLogic::getScore(PlayerSide side) const
{
	return mScores[side2index(side)];
}

void IGameLogic::setScore(PlayerSide side, int score)
{
	mScores[side2index(side)] = score;
}


void IGameLogic::setScoreToWin(int stw)
{
	assert(stw > 0);
	mScoreToWin = stw;
	// when are the values in the lua script updated?
	//lua_pushnumber(mState, mScoreToWin);
	//lua_setglobal(mState, "SCORE_TO_WIN");
}

int IGameLogic::getScoreToWin() const 
{
	return mScoreToWin;
}

PlayerSide IGameLogic::getServingPlayer() const
{
	return mServingPlayer;
}

void IGameLogic::setServingPlayer(PlayerSide side)
{
	mServingPlayer = side;
}

int IGameLogic::getHits(PlayerSide side) const
{
	return mTouches[side2index(side)];
}

PlayerSide IGameLogic::getWinningPlayer() const
{
	return mWinningPlayer;
}

Clock& IGameLogic::getClock()
{
	return clock;
}

PlayerSide IGameLogic::getLastErrorSide()
{
	PlayerSide t = mLastError;
	mLastError = NO_PLAYER;
	/// reset mLastError to NO_PLAYER
	/// why?
	return t;
}

// -------------------------------------------------------------------------------------------------
//								Event Handlers
// -------------------------------------------------------------------------------------------------
void IGameLogic::step()
{
	clock.step();
	
	if(clock.isRunning())
	{
		--mSquish[0];
		--mSquish[1];
	}
}

void IGameLogic::onPause()
{
	/// pausing for now only means stopping the clock
	clock.stop();
}
void IGameLogic::onUnPause()
{
	clock.start();
}

void IGameLogic::onBallHitsGround(PlayerSide side) 
{
	onError(side);
}

bool IGameLogic::isCollisionValid(PlayerSide side) const
{
	// check whether the ball is squished
	return mSquish[side2index(side)] <= 0;
}

void IGameLogic::onBallHitsPlayer(PlayerSide side)
{
	if(!isCollisionValid(side))
		return;
	
	// otherwise, set the squish value
	mSquish[side2index(side)] = SQUISH_TOLERANCE;
	// now, the other blobby has to accept the new hit!
	mSquish[side2index(other_side(side))] = 0;
	
	// count the touches
	mTouches[side2index(other_side(side))] = 0;
	mTouches[side2index(side)]++;
	if( mTouches[side2index(side)] > 3 )
	{
		// if a player hits a forth time, it is an error
		onError(side);
	}
}


void IGameLogic::score(PlayerSide side)
{
	++mScores[side2index(side)];
	mTouches[0] = 0;
	mTouches[1] = 0;
	mWinningPlayer = checkWin();
}

void IGameLogic::reset()
{
	mScores[0] = 0;
	mScores[1] = 0;
	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
}

void IGameLogic::onError(PlayerSide side)
{
	mLastError = side;
	
	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
	
	OnMistake(side);
	mServingPlayer = other_side(side);
}


// -------------------------------------------------------------------------------------------------


class LuaGameLogic : public IGameLogic 
{
	public:
		LuaGameLogic(const std::string& file);
		virtual ~LuaGameLogic();
		
	private:
		
		virtual PlayerSide checkWin() const;
		virtual void OnMistake(PlayerSide side);
		
		// lua functions
		static int luaScore(lua_State* state); 
		static int luaGetOpponent(lua_State* state);
		static int luaGetServingPlayer(lua_State* state);
		static int luaGetGameTime(lua_State* state);
		
		// lua state
		lua_State* mState;
};



LuaGameLogic::LuaGameLogic( const std::string& filename ) : mState( lua_open() ) 
{
	
	lua_pushlightuserdata(mState, this);
	lua_setglobal(mState, "__GAME_LOGIC_POINTER");
	
	/// \todo how to push parameters???
	///	\todo how to react when mScoreToWin changes?
	lua_pushnumber(mState, getScoreToWin());
	lua_setglobal(mState, "SCORE_TO_WIN");
	
	// add functions
	lua_register(mState, "score", luaScore);
	lua_register(mState, "opponent", luaGetOpponent);
	lua_register(mState, "servingplayer", luaGetServingPlayer);
	lua_register(mState, "time", luaGetGameTime);
	
	
	// now load script file
	ReaderInfo info;
	// this opens the file
	info.file.open(filename);

	int error = lua_load(mState, chunkReader, &info, filename.c_str());
	
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
	
	// check that all functions are available
	lua_getglobal(mState, "IsWinning");
	if (!lua_isfunction(mState, -1)) 
	{
		std::cerr << "Script Error: Could not find function IsWinning";
		std::cerr << std::endl;
		ScriptException except;
		except.luaerror = "Could not find function IsWinning";
		lua_pop(mState, 1);
		lua_close(mState);
		throw except;
	}
	lua_getglobal(mState, "OnMistake");
	if (!lua_isfunction(mState, -1)) 
	{
		std::cerr << "Script Error: Could not find function OnMistake";
		std::cerr << std::endl;
		ScriptException except;
		except.luaerror = "Could not find function OnMistake";
		lua_pop(mState, 1);
		lua_close(mState);
		throw except;
	}
}

LuaGameLogic::~LuaGameLogic()
{
	lua_close(mState);
}

PlayerSide LuaGameLogic::checkWin() const
{
	bool won = false;
	lua_getglobal(mState, "IsWinning");
	
	lua_pushnumber(mState, getScore(LEFT_PLAYER) );
	lua_pushnumber(mState, getScore(RIGHT_PLAYER) );
	if( lua_pcall(mState, 2, 1, 0) )
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
	
	won = lua_toboolean(mState, -1);
	lua_pop(mState, 1);

	if(won) 
	{
		if( getScore(LEFT_PLAYER) > getScore(RIGHT_PLAYER) )
			return LEFT_PLAYER;
			
		if( getScore(LEFT_PLAYER) < getScore(RIGHT_PLAYER) )
			return RIGHT_PLAYER;
	}
	
	return NO_PLAYER;
}

void LuaGameLogic::OnMistake(PlayerSide side) 
{
	// call lua scoring rules
	lua_getglobal(mState, "OnMistake");
	lua_pushnumber(mState, side);
	if(lua_pcall(mState, 1, 0, 0)) 
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
}

int LuaGameLogic::luaScore(lua_State* state) 
{
	int pl = int(lua_tonumber(state, -1) + 0.5);
	lua_pop(state, 1);
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	LuaGameLogic* gl = (LuaGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	gl->score((PlayerSide)pl);
	return 0;
}

int LuaGameLogic::luaGetOpponent(lua_State* state) 
{
	int pl = int(lua_tonumber(state, -1) + 0.5);
	lua_pop(state, 1);
	lua_pushnumber(state, other_side((PlayerSide)pl));
	return 1;
}

int LuaGameLogic::luaGetServingPlayer(lua_State* state) 
{
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	LuaGameLogic* gl = (LuaGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);

	lua_pushnumber(state, gl->getServingPlayer());
	return 1;
}

int LuaGameLogic::luaGetGameTime(lua_State* state) 
{
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	LuaGameLogic* gl = (LuaGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	lua_pushnumber(state, gl->getClock().getTime());
	return 1;
}


class FallbackGameLogic : public IGameLogic 
{
	public:
		FallbackGameLogic() 
		{
			
		}
		virtual ~FallbackGameLogic()
		{
			
		}
		
	private:
		
		virtual PlayerSide checkWin() const 
		{
			if( getScore(LEFT_PLAYER) >= getScoreToWin() ) {
				return LEFT_PLAYER;
			}
			
			if( getScore(RIGHT_PLAYER) >= getScoreToWin() ) {
				return RIGHT_PLAYER;
			}
			
			return NO_PLAYER;
		}
		
		virtual void OnMistake(PlayerSide side) 
		{
			score( other_side(side) );
		}
};

GameLogic createGameLogic(const std::string& file)
{
	try 
	{
		return std::auto_ptr<IGameLogic>( new LuaGameLogic(file) );
	} 
	catch(...) 
	{
		std::cerr << "Script Error: Could not create LuaGameLogic";
		std::cerr << std::endl;
		std::cerr << "              Using fallback ruleset";
		std::cerr << std::endl;
		return std::auto_ptr<IGameLogic>(new FallbackGameLogic());
	}
	
}
