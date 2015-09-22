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
#include "GameConstants.h"
#include "IScriptableComponent.h"
#include "PlayerInput.h"


int lua_toint(lua_State* state, int index)
{
	double value = lua_tonumber(state, index);
	return int(value + (value > 0 ? 0.5 : -0.5));
}


/* implementation */

/// how many steps must pass until the next hit can happen
const int SQUISH_TOLERANCE = 11;

const std::string FALLBACK_RULES_NAME = "__FALLBACK__";
const std::string DUMMY_RULES_NAME = "__DUMMY__";


IGameLogic::IGameLogic( int stw )
: mScoreToWin( stw)
, mSquishWall(0)
, mSquishGround(0)
, mLastError(NO_PLAYER)
, mServingPlayer(NO_PLAYER)
, mIsBallValid(true)
, mIsGameRunning(false)
, mWinningPlayer(NO_PLAYER)
{
	// init clock
	mClock.reset();
	mClock.start();
	mScores[LEFT_PLAYER] = 0;
	mScores[RIGHT_PLAYER] = 0;
	mTouches[LEFT_PLAYER] = 0;
	mTouches[RIGHT_PLAYER] = 0;
	mSquish[LEFT_PLAYER] = 0;
	mSquish[RIGHT_PLAYER] = 0;
}

IGameLogic::~IGameLogic()
{
	// nothing to do
}

int IGameLogic::getTouches(PlayerSide side) const
{
	return mTouches[side2index(side)];
}

int IGameLogic::getScore(PlayerSide side) const
{
	return mScores[side2index(side)];
}

void IGameLogic::setScore(PlayerSide side, int score)
{
	mScores[side2index(side)] = score;
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
	return mClock;
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
	gls.hitCount[LEFT_PLAYER] = getTouches(LEFT_PLAYER);
	gls.hitCount[RIGHT_PLAYER] = getTouches(RIGHT_PLAYER);
	gls.servingPlayer = getServingPlayer();
	gls.winningPlayer = getWinningPlayer();
	gls.squish[LEFT_PLAYER] = mSquish[LEFT_PLAYER];
	gls.squish[RIGHT_PLAYER] = mSquish[RIGHT_PLAYER];
	gls.squishWall = mSquishWall;
	gls.squishGround = mSquishGround;
	gls.isGameRunning = mIsGameRunning;
	gls.isBallValid = mIsBallValid;

	return gls;
}

void IGameLogic::setState(GameLogicState gls)
{
	setScore(LEFT_PLAYER, gls.leftScore);
	setScore(RIGHT_PLAYER, gls.rightScore);
	mTouches[LEFT_PLAYER] = gls.hitCount[LEFT_PLAYER];
	mTouches[RIGHT_PLAYER] = gls.hitCount[RIGHT_PLAYER];
	setServingPlayer(gls.servingPlayer);
	mSquish[LEFT_PLAYER] = gls.squish[LEFT_PLAYER];
	mSquish[RIGHT_PLAYER] = gls.squish[RIGHT_PLAYER];
	mSquishWall = gls.squishWall;
	mSquishGround = gls.squishGround;
	mIsGameRunning = gls.isGameRunning;
	mIsBallValid = gls.isBallValid;
}

// -------------------------------------------------------------------------------------------------
//								Event Handlers
// -------------------------------------------------------------------------------------------------
void IGameLogic::step( const DuelMatchState& state )
{
	mClock.step();

	if(mClock.isRunning())
	{
		--mSquish[0];
		--mSquish[1];
		--mSquishWall;
		--mSquishGround;

		OnGameHandler( state );
	}
}

void IGameLogic::onPause()
{
	/// pausing for now only means stopping the clock
	// pausing is saved into an atomic variable, so this is safe
	mClock.stop();
}

void IGameLogic::onUnPause()
{
	mClock.start();
}

PlayerInput IGameLogic::transformInput(PlayerInput ip, PlayerSide player)
{
	return handleInput(ip, player);
}

void IGameLogic::onServe()
{
	mIsBallValid = true;
	mIsGameRunning = false;
}

void IGameLogic::onBallHitsGround(PlayerSide side)
{
	// check if collision valid
	if(!isGroundCollisionValid())
		return;

	// otherwise, set the squish value
	mSquishGround = SQUISH_TOLERANCE;

	mTouches[other_side(side)] = 0;

	OnBallHitsGroundHandler(side);
}

bool IGameLogic::isBallValid() const
{
	return mIsBallValid;
}

bool IGameLogic::isGameRunning() const
{
	return mIsGameRunning;
}

bool IGameLogic::isCollisionValid(PlayerSide side) const
{
	// check whether the ball is squished
	return mSquish[side2index(side)] <= 0;
}

bool IGameLogic::isGroundCollisionValid() const
{
	// check whether the ball is squished
	return mSquishGround <= 0 && isBallValid();
}

bool IGameLogic::isWallCollisionValid() const
{
	// check whether the ball is squished
	return mSquishWall <= 0 && isBallValid();
}

void IGameLogic::onBallHitsPlayer(PlayerSide side)
{
	if(!isCollisionValid(side))
		return;

	// otherwise, set the squish value
	mSquish[side2index(side)] = SQUISH_TOLERANCE;
	// now, the other blobby has to accept the new hit!
	mSquish[side2index(other_side(side))] = 0;

	// set the ball activity
	mIsGameRunning = true;

	// count the touches
	mTouches[side2index(side)]++;
	OnBallHitsPlayerHandler(side);

	// reset other players touches after OnBallHitsPlayerHandler is called, so
	// we have still access to its old value inside the handler function
	mTouches[side2index(other_side(side))] = 0;
}

void IGameLogic::onBallHitsWall(PlayerSide side)
{
	if(!isWallCollisionValid())
		return;

	// otherwise, set the squish value
	mSquishWall = SQUISH_TOLERANCE;

	OnBallHitsWallHandler(side);
}

void IGameLogic::onBallHitsNet(PlayerSide side)
{
	if(!isWallCollisionValid())
		return;

	// otherwise, set the squish value
	mSquishWall = SQUISH_TOLERANCE;

	OnBallHitsNetHandler(side);
}

void IGameLogic::score(PlayerSide side, int amount)
{
	int index = side2index(side);
	mScores[index] += amount;
	if (mScores[index] < 0)
		mScores[index] = 0;

	mWinningPlayer = checkWin();
}

void IGameLogic::onError(PlayerSide errorSide, PlayerSide serveSide)
{
	mLastError = errorSide;
	mIsBallValid = false;

	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
	mSquishWall = 0;
	mSquishGround = 0;

	mServingPlayer = serveSide;
}

// -------------------------------------------------------------------------------------------------
// 	Dummy Game Logic
// ---------------------

class DummyGameLogic : public IGameLogic
{
	public:
		DummyGameLogic(int stw ) : IGameLogic( stw )
		{
		}
		virtual ~DummyGameLogic()
		{

		}

		virtual GameLogic clone() const
		{
			return GameLogic(new DummyGameLogic( getScoreToWin() ));
		}

		virtual std::string getSourceFile() const
		{
			return std::string("");
		}

		virtual std::string getAuthor() const
		{
			return "Blobby Volley 2 Developers";
		}

		virtual std::string getTitle() const
		{
			return DUMMY_RULES_NAME;
		}

	protected:

		virtual PlayerSide checkWin() const
		{
			return NO_PLAYER;
		}

		virtual PlayerInput handleInput(PlayerInput ip, PlayerSide player)
		{
			return ip;
		}

		virtual void OnBallHitsPlayerHandler(PlayerSide side)
		{
		}

		virtual void OnBallHitsWallHandler(PlayerSide side)
		{
		}

		virtual void OnBallHitsNetHandler(PlayerSide side)
		{
		}

		virtual void OnBallHitsGroundHandler(PlayerSide side)
		{
		}

		virtual void OnGameHandler( const DuelMatchState& state )
		{
		}
};


// -------------------------------------------------------------------------------------------------
// 	Fallback Game Logic
// ---------------------

class FallbackGameLogic : public DummyGameLogic
{
	public:
		FallbackGameLogic( int stw ) : DummyGameLogic( stw )
		{
		}
		virtual ~FallbackGameLogic()
		{

		}

		virtual GameLogic clone() const
		{
			return GameLogic(new FallbackGameLogic( getScoreToWin() ));
		}

		virtual std::string getTitle() const
		{
			return FALLBACK_RULES_NAME;
		}

protected:

		virtual PlayerSide checkWin() const
		{
			int left = getScore(LEFT_PLAYER);
			int right = getScore(RIGHT_PLAYER);
			int stw = getScoreToWin();
			if( left >= stw && left >= right + 2 )
			{
				return LEFT_PLAYER;
			}

			if( right >= stw && right >= left + 2 )
			{
				return RIGHT_PLAYER;
			}

			return NO_PLAYER;
		}

		virtual void OnBallHitsPlayerHandler(PlayerSide side)
		{
			if (getTouches(side) > 3)
			{
				score( other_side(side), 1 );
				onError( side, other_side(side) );
			}
		}

		virtual void OnBallHitsGroundHandler(PlayerSide side)
		{
			score( other_side(side), 1 );
			onError( side, other_side(side) );
		}
};


class LuaGameLogic : public FallbackGameLogic, public IScriptableComponent
{
	public:
		LuaGameLogic(const std::string& file, DuelMatch* match, int score_to_win);
		virtual ~LuaGameLogic();

		virtual std::string getSourceFile() const
		{
			return mSourceFile;
		}

		virtual GameLogic clone() const
		{
			lua_getglobal(mState, "__MATCH_POINTER");
			DuelMatch* match = (DuelMatch*)lua_touserdata(mState, -1);
			lua_pop(mState, 1);
			return GameLogic(new LuaGameLogic(mSourceFile, match, getScoreToWin()));
		}

		virtual std::string getAuthor() const
		{
			return mAuthor;
		}

		virtual std::string getTitle() const
		{
			return mTitle;
		}


	protected:

		virtual PlayerInput handleInput(PlayerInput ip, PlayerSide player);
		virtual PlayerSide checkWin() const;
		virtual void OnBallHitsPlayerHandler(PlayerSide side);
		virtual void OnBallHitsWallHandler(PlayerSide side);
		virtual void OnBallHitsNetHandler(PlayerSide side);
		virtual void OnBallHitsGroundHandler(PlayerSide side);
		virtual void OnGameHandler( const DuelMatchState& state );

		static LuaGameLogic* getGameLogic(lua_State* state);

	private:

		// lua functions
		static int luaMistake(lua_State* state);
		static int luaScore(lua_State* state);
		static int luaGetServingPlayer(lua_State* state);
		static int luaGetGameTime(lua_State* state);
		static int luaIsGameRunning(lua_State* state);

		// lua state
		std::string mSourceFile;

		std::string mAuthor;
		std::string mTitle;
};


LuaGameLogic::LuaGameLogic( const std::string& filename, DuelMatch* match, int score_to_win ) : FallbackGameLogic( score_to_win ), mSourceFile(filename)
{
	lua_pushlightuserdata(mState, this);
	lua_setglobal(mState, "__GAME_LOGIC_POINTER");

	/// \todo use lua registry instead of globals!
	lua_pushlightuserdata(mState, match);
	lua_setglobal(mState, "__MATCH_POINTER");
	lua_pushnumber(mState, getScoreToWin());
	lua_setglobal(mState, "SCORE_TO_WIN");

	setGameConstants();
	setGameFunctions();

	// add functions
	luaL_requiref(mState, "math", luaopen_math, 1);
	lua_register(mState, "score", luaScore);
	lua_register(mState, "mistake", luaMistake);
	lua_register(mState, "servingplayer", luaGetServingPlayer);
	lua_register(mState, "time", luaGetGameTime);
	lua_register(mState, "isgamerunning", luaIsGameRunning);

	// now load script file
	openScript("api");
	openScript("rules_api");
	openScript("rules/"+mSourceFile);

	lua_getglobal(mState, "SCORE_TO_WIN");
	mScoreToWin = lua_toint(mState, -1);
	lua_pop(mState, 1);

	lua_getglobal(mState, "__AUTHOR__");
	const char* author = lua_tostring(mState, -1);
	mAuthor = ( author ? author : "unknown author" );
	lua_pop(mState, 1);

	lua_getglobal(mState, "__TITLE__");
	const char* title = lua_tostring(mState, -1);
	mTitle = ( title ? title : "untitled script" );
	lua_pop(mState, 1);

	std::cout << "loaded rules "<< getTitle()<< " by " << getAuthor() << " from " << mSourceFile << std::endl;
}

LuaGameLogic::~LuaGameLogic()
{
}

PlayerSide LuaGameLogic::checkWin() const
{
	const_cast<LuaGameLogic*>(this)->updateGameState( getState( ) ); // make sure the scripting engine gets current version
	bool won = false;
	if (!getLuaFunction("IsWinning"))
	{
		return FallbackGameLogic::checkWin();
	}

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

PlayerInput LuaGameLogic::handleInput(PlayerInput ip, PlayerSide player)
{
	updateGameState( getState( ) ); // make sure the scripting engine gets current version
	if (!getLuaFunction( "HandleInput" ))
	{
		return FallbackGameLogic::handleInput(ip, player);
	}
	lua_pushnumber(mState, player);
	lua_pushboolean(mState, ip.left);
	lua_pushboolean(mState, ip.right);
	lua_pushboolean(mState, ip.up);
	if(lua_pcall(mState, 4, 3, 0))
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};

	PlayerInput ret;
	ret.up = lua_toboolean(mState, -1);
	ret.right = lua_toboolean(mState, -2);
	ret.left = lua_toboolean(mState, -3);

	// cleanup stack
	lua_pop(mState, lua_gettop(mState));

	return ret;
}

void LuaGameLogic::OnBallHitsPlayerHandler(PlayerSide side)
{
	updateGameState( getState( ) ); // make sure the scripting engine gets current version
	if (!getLuaFunction("OnBallHitsPlayer"))
	{
		FallbackGameLogic::OnBallHitsPlayerHandler(side);
		return;
	}
	lua_pushnumber(mState, side);
	if( lua_pcall(mState, 1, 0, 0) )
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
}

void LuaGameLogic::OnBallHitsWallHandler(PlayerSide side)
{
	updateGameState( getState( ) ); // make sure the scripting engine gets current version
	if (!getLuaFunction("OnBallHitsWall"))
	{
		FallbackGameLogic::OnBallHitsWallHandler(side);
		return;
	}

	lua_pushnumber(mState, side);
	if( lua_pcall(mState, 1, 0, 0) )
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
}

void LuaGameLogic::OnBallHitsNetHandler(PlayerSide side)
{
	updateGameState( getState( ) ); // make sure the scripting engine gets current version
	if (!getLuaFunction( "OnBallHitsNet" ))
	{
		FallbackGameLogic::OnBallHitsNetHandler(side);
		return;
	}

	lua_pushnumber(mState, side);

	if( lua_pcall(mState, 1, 0, 0) )
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
}

void LuaGameLogic::OnBallHitsGroundHandler(PlayerSide side)
{
	updateGameState( getState( ) ); // make sure the scripting engine gets current version
	if (!getLuaFunction( "OnBallHitsGround" ))
	{
		FallbackGameLogic::OnBallHitsGroundHandler(side);
		return;
	}

	lua_pushnumber(mState, side);

	if( lua_pcall(mState, 1, 0, 0) )
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
}

void LuaGameLogic::OnGameHandler( const DuelMatchState& state )
{
	// we need to do this on every function call, so maybe it was not the best idea
	// to save the state instead of the pointer to a duel match inside the lua script.
	/// \todo find a better way to do this.
	updateGameState( state );		// set state of world
	updateGameState( getState( ) ); // make sure the scripting engine gets current version
	if (!getLuaFunction( "OnGame" ))
	{
		FallbackGameLogic::OnGameHandler( state );
		return;
	}
	if( lua_pcall(mState, 0, 0, 0) )
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
}

LuaGameLogic* LuaGameLogic::getGameLogic(lua_State* state)
{
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	LuaGameLogic* gl = (LuaGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	return gl;
}

int LuaGameLogic::luaMistake(lua_State* state)
{
	int amount = lua_toint(state, -1);
 	lua_pop(state, 1);
	PlayerSide serveSide = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	PlayerSide mistakeSide = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	LuaGameLogic* gl = getGameLogic(state);

	gl->score(other_side(mistakeSide), amount);
	gl->onError(mistakeSide, serveSide);
	return 0;
}

int LuaGameLogic::luaScore(lua_State* state)
{
	int amount = lua_toint(state, -1);
	lua_pop(state, 1);
	int player = lua_toint(state, -1);
	lua_pop(state, 1);
	LuaGameLogic* gl = getGameLogic(state);

	gl->score((PlayerSide)player, amount);
	return 0;
}

int LuaGameLogic::luaGetServingPlayer(lua_State* state)
{
	LuaGameLogic* gl = getGameLogic(state);
	lua_pushnumber(state, gl->getServingPlayer());
	return 1;
}

int LuaGameLogic::luaGetGameTime(lua_State* state)
{
	LuaGameLogic* gl = getGameLogic(state);
	lua_pushnumber(state, gl->getClock().getTime());
	return 1;
}

int LuaGameLogic::luaIsGameRunning(lua_State* state)
{
	LuaGameLogic* gl = getGameLogic(state);
	lua_pushboolean(state, gl->isGameRunning());
	return 1;
}

GameLogic createGameLogic()
{
	return GameLogic(new DummyGameLogic( 15 ));
}

GameLogic createGameLogic(const std::string& file, DuelMatch* match, int score_to_win )
{
	if(file == DUMMY_RULES_NAME)
	{
		return GameLogic(new DummyGameLogic( score_to_win ));
	}
		else if (file == FALLBACK_RULES_NAME)
	{
		return GameLogic(new FallbackGameLogic( score_to_win ));
	}

	try
	{
		return GameLogic( new LuaGameLogic(file, match, score_to_win ) );
	}
	catch( std::exception& exp)
	{
		std::cerr << "Script Error: Could not create LuaGameLogic: \n";
		std::cerr << exp.what() << std::endl;
		std::cerr << "              Using fallback ruleset";
		std::cerr << std::endl;
		return GameLogic(new FallbackGameLogic( score_to_win ));
	}

}
