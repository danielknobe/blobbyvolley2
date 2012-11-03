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
#include "GameLogic.h"

/* includes */
#include <cassert>
#include <cmath>
#include <iostream>

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

#include "FileRead.h"
#include "GameLogicState.h"
#include "DuelMatch.h"


/* implementation */

/// how many steps must pass until the next hit can happen
const int SQUISH_TOLERANCE = 11;


IGameLogic::IGameLogic(DuelMatch* match):	mLastError(NO_PLAYER), 
							mServingPlayer(NO_PLAYER), 
							mWinningPlayer(NO_PLAYER),
							mScoreToWin(15),
							mMatch(match),
							mSquishWall(0)
{
	// init clock
	clock.reset();
	clock.start();
	reset();
	mSquish[LEFT_PLAYER] = 0;
	mSquish[RIGHT_PLAYER] = 0;
}

IGameLogic::~IGameLogic() 
{
	// nothing to do
}

int IGameLogic::getScore(PlayerSide side) const
{
	return mScores[side2index(side)];
}

int IGameLogic::getTouches(PlayerSide side) const
{
	return mTouches[side2index(side)];
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

GameLogicState IGameLogic::getState() const
{
	GameLogicState gls;
	gls.leftScore = getScore(LEFT_PLAYER);
	gls.rightScore = getScore(RIGHT_PLAYER);
	gls.servingPlayer = getServingPlayer();
	gls.leftSquish = mSquish[LEFT_PLAYER];
	gls.rightSquish = mSquish[RIGHT_PLAYER];
	gls.squishWall = mSquishWall;
	
	return gls;
}

void IGameLogic::setState(GameLogicState gls)
{
	setScore(LEFT_PLAYER, gls.leftScore);
	setScore(RIGHT_PLAYER, gls.rightScore);
	setServingPlayer(gls.servingPlayer);
	mSquish[LEFT_PLAYER] = gls.leftSquish;
	mSquish[RIGHT_PLAYER] = gls.rightSquish;
	mSquishWall = gls.squishWall;
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
		--mSquishWall;
		
		OnGameHandler();
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

bool IGameLogic::isWallCollisionValid() const
{
	// check whether the ball is squished
	return mSquishWall <= 0 && mMatch->getBallValid();
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
	
	if( ! OnBallHitsPlayerHandler(side, mTouches[side2index(side)]) )
	{
		// if a player hits a forth time, it is an error
		onError(side);
	}
}

void IGameLogic::onBallHitsWall(PlayerSide side)
{
	if(!isWallCollisionValid())
		return;
	
	// otherwise, set the squish value
	mSquishWall = SQUISH_TOLERANCE;
	
	OnBallHitsWallHandler(side);
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
	mSquishWall = 0;
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
		LuaGameLogic(const std::string& file, DuelMatch* match);
		virtual ~LuaGameLogic();
		
		virtual std::string getSourceFile() const 
		{
			return mSourceFile;
		}
		
	private:
		
		virtual PlayerSide checkWin() const;
		virtual void OnMistake(PlayerSide side);
		virtual bool OnBallHitsPlayerHandler(PlayerSide side, int numOfHits);
		virtual void OnBallHitsWallHandler(PlayerSide side);
		virtual void OnGameHandler();

		// lua functions
		static int luaScore(lua_State* state); 
		static int luaGetOpponent(lua_State* state);
		static int luaGetServingPlayer(lua_State* state);
		static int luaGetGameTime(lua_State* state);
		
		// lua state
		lua_State* mState;
		
		std::string mSourceFile;
};



LuaGameLogic::LuaGameLogic( const std::string& filename, DuelMatch* match ) : IGameLogic(match), 
																				mState( lua_open() ), 
																				mSourceFile(filename) 
{
	
	lua_pushlightuserdata(mState, this);
	lua_setglobal(mState, "__GAME_LOGIC_POINTER");
	
	/// \todo how to push parameters???
	///	\todo how to react when mScoreToWin changes?
	lua_pushlightuserdata(mState, match);
	lua_setglobal(mState, "__MATCH_POINTER");
	lua_pushnumber(mState, getScoreToWin());
	lua_setglobal(mState, "SCORE_TO_WIN");
	
	// add functions
	lua_register(mState, "score", luaScore);
	lua_register(mState, "opponent", luaGetOpponent);
	lua_register(mState, "servingplayer", luaGetServingPlayer);
	lua_register(mState, "time", luaGetGameTime);
	
	
	// now load script file
	int error = FileRead::readLuaScript(filename, mState);
	
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

bool LuaGameLogic::OnBallHitsPlayerHandler(PlayerSide side, int numOfHits)
{
	bool valid = false;
	lua_getglobal(mState, "OnBallHitsPlayer");
	
	lua_pushnumber(mState, side );
	lua_pushnumber(mState, numOfHits );
	if( lua_pcall(mState, 2, 1, 0) )
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
	
	valid = lua_toboolean(mState, -1);
	lua_pop(mState, 1);

	return valid;
}

void LuaGameLogic::OnBallHitsWallHandler(PlayerSide side)
{
	lua_getglobal(mState, "OnBallHitsWall");
	lua_pushnumber(mState, side);
	if( lua_pcall(mState, 1, 0, 0) )
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
}


void LuaGameLogic::OnGameHandler()
{
	lua_getglobal(mState, "OnGame");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
		return;
	}
	if( lua_pcall(mState, 0, 0, 0) )
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
		FallbackGameLogic(DuelMatch* match) : IGameLogic(match) 
		{
			
		}
		virtual ~FallbackGameLogic()
		{
			
		}
		
		virtual std::string getSourceFile() const 
		{
			return std::string("?FALLBACK?");
		}

	private:
		
		virtual PlayerSide checkWin() const 
		{
			if( getScore(LEFT_PLAYER) >= getScoreToWin() ) 
			{
				return LEFT_PLAYER;
			}
			
			if( getScore(RIGHT_PLAYER) >= getScoreToWin() ) 
			{
				return RIGHT_PLAYER;
			}
			
			return NO_PLAYER;
		}
		
		virtual void OnMistake(PlayerSide side) 
		{
			score( other_side(side) );
		}
		
		bool OnBallHitsPlayerHandler(PlayerSide ply, int numOfHits)
		{
			return numOfHits <= 3;
		}

		void OnBallHitsWallHandler(PlayerSide ply)
		{
		}
		
		virtual void OnGameHandler()
		{
		}
};

GameLogic createGameLogic(const std::string& file, DuelMatch* match)
{
	try 
	{
		return std::auto_ptr<IGameLogic>( new LuaGameLogic(file, match) );
	} 
	catch(...) 
	{
		std::cerr << "Script Error: Could not create LuaGameLogic";
		std::cerr << std::endl;
		std::cerr << "              Using fallback ruleset";
		std::cerr << std::endl;
		return std::auto_ptr<IGameLogic>(new FallbackGameLogic(match));
	}
	
}
