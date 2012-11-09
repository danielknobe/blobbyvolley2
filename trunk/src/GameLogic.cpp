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


/* implementation */

/// how many steps must pass until the next hit can happen
const int SQUISH_TOLERANCE = 11;


IGameLogic::IGameLogic(DuelMatch* match):	mLastError(NO_PLAYER), 
							mServingPlayer(NO_PLAYER), 
							mWinningPlayer(NO_PLAYER),
							mScoreToWin(15),
							mMatch(match),
							mSquishWall(0),
							mSquishGround(0)
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
	gls.squishGround = mSquishGround;
	
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
	mSquishGround = gls.squishGround;
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
		--mSquishGround;
		
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

PlayerInput IGameLogic::transformInput(PlayerInput ip, PlayerSide player)
{
	return handleInput(ip, player);
}

void IGameLogic::onBallHitsGround(PlayerSide side) 
{
	// check if collision valid
	if(!isGroundCollisionValid())
		return;
	
	// otherwise, set the squish value
	mSquishGround = SQUISH_TOLERANCE;
	
	OnBallHitsGroundHandler(side);
}

bool IGameLogic::isCollisionValid(PlayerSide side) const
{
	// check whether the ball is squished
	return mSquish[side2index(side)] <= 0;
}

bool IGameLogic::isGroundCollisionValid() const
{
	// check whether the ball is squished
	return mSquishGround <= 0 && mMatch->getBallValid();
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

void IGameLogic::score(PlayerSide side, bool serve, int amount)
{
	mScores[side2index(side)] += amount;
	
	// only reset play touches when serve follows
	if(serve)
	{
		mTouches[0] = 0;
		mTouches[1] = 0;
	}
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
	OnMistake(side);
	
	mLastError = side;
	
	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
	
	mServingPlayer = other_side(side);
}

// -------------------------------------------------------------------------------------------------
// 	Fallback Game Logic
// ---------------------

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
			return std::string("");
		}
		
	protected:
		
		virtual PlayerSide checkWin() const 
		{
			if( getScore(LEFT_PLAYER) >= getScoreToWin() && getScore(LEFT_PLAYER) >= getScore(RIGHT_PLAYER) + 2 ) 
			{
				return LEFT_PLAYER;
			}
			
			if( getScore(RIGHT_PLAYER) >= getScoreToWin() && getScore(RIGHT_PLAYER) >= getScore(LEFT_PLAYER) + 2 ) 
			{
				return RIGHT_PLAYER;
			}
			
			return NO_PLAYER;
		}
		
		virtual PlayerInput handleInput(PlayerInput ip, PlayerSide player)
		{
			return ip;
		}
		
		virtual void OnMistake(PlayerSide side) 
		{
			score( other_side(side), true, 1 );
		}
		
		virtual void OnBallHitsPlayerHandler(PlayerSide side)
		{
			if (getTouches(side) > 3) onError(side);
		}
		
		virtual void OnBallHitsWallHandler(PlayerSide side)
		{
		}
		
		virtual void OnBallHitsNetHandler(PlayerSide side)
		{
		}
		
		virtual void OnBallHitsGroundHandler(PlayerSide side)
		{
			onError(side);
		}
		
		virtual void OnGameHandler()
		{
		}
};


class LuaGameLogic : public FallbackGameLogic
{
	public:
		LuaGameLogic(const std::string& file, DuelMatch* match);
		virtual ~LuaGameLogic();
		
		virtual std::string getSourceFile() const 
		{
			return mSourceFile;
		}
		
	private:
		
		virtual PlayerInput handleInput(PlayerInput ip, PlayerSide player);
		virtual PlayerSide checkWin() const;
		virtual void OnMistake(PlayerSide side);
		virtual void OnBallHitsPlayerHandler(PlayerSide side);
		virtual void OnBallHitsWallHandler(PlayerSide side);
		virtual void OnBallHitsNetHandler(PlayerSide side);
		virtual void OnBallHitsGroundHandler(PlayerSide side);
		virtual void OnGameHandler();
		
		// helper functions
		void setLuaGlobalVariable(const char* name, double value);
		
		// lua functions
		static int luaTouches(lua_State* state);
		static int luaLaunched(lua_State* state);
		static int luaBallX(lua_State* state);
		static int luaBallY(lua_State* state);
		static int luaBSpeedX(lua_State* state);
		static int luaBSpeedY(lua_State* state);
		static int luaPosX(lua_State* state);
		static int luaPosY(lua_State* state);
		static int luaSpeedX(lua_State* state);
		static int luaSpeedY(lua_State* state);
		static int luaMistake(lua_State* state);
		static int luaScore(lua_State* state);
		static int luaGetScore(lua_State* state); 
		static int luaGetOpponent(lua_State* state);
		static int luaGetServingPlayer(lua_State* state);
		static int luaGetGameTime(lua_State* state);
		
		// lua state
		lua_State* mState;
		
		std::string mSourceFile;
};


LuaGameLogic::LuaGameLogic( const std::string& filename, DuelMatch* match ) : FallbackGameLogic(match), 
																				mState( lua_open() ), 
																				mSourceFile(filename) 
{
	
	lua_pushlightuserdata(mState, this);
	lua_setglobal(mState, "__GAME_LOGIC_POINTER");
	
	lua_pushlightuserdata(mState, match);
	lua_setglobal(mState, "__MATCH_POINTER");
	lua_pushnumber(mState, getScoreToWin());
	lua_setglobal(mState, "SCORE_TO_WIN");
	

	// add functions
	luaopen_math(mState);
	lua_register(mState, "touches", luaTouches);
	lua_register(mState, "launched", luaLaunched);
	lua_register(mState, "ballx", luaBallX);
	lua_register(mState, "bally", luaBallY);
	lua_register(mState, "bspeedx", luaBSpeedX);
	lua_register(mState, "bspeedy", luaBSpeedY);
	lua_register(mState, "posx", luaPosX);
	lua_register(mState, "posy", luaPosY);
	lua_register(mState, "speedx", luaSpeedX);
	lua_register(mState, "speedy", luaSpeedY);
	lua_register(mState, "getScore", luaGetScore);
	lua_register(mState, "score", luaScore);
	lua_register(mState, "mistake", luaMistake);
	lua_register(mState, "opponent", luaGetOpponent);
	lua_register(mState, "servingplayer", luaGetServingPlayer);
	lua_register(mState, "time", luaGetGameTime);
	
		
	// set game constants
	setLuaGlobalVariable("CONST_FIELD_WIDTH", RIGHT_PLANE);
	setLuaGlobalVariable("CONST_GROUND_HEIGHT", GROUND_PLANE_HEIGHT_MAX);
	setLuaGlobalVariable("CONST_BALL_GRAVITY", -BALL_GRAVITATION);
	setLuaGlobalVariable("CONST_BALL_RADIUS", BALL_RADIUS);
	setLuaGlobalVariable("CONST_BLOBBY_JUMP", BLOBBY_JUMP_ACCELERATION);
	setLuaGlobalVariable("CONST_BLOBBY_BODY_RADIUS", BLOBBY_LOWER_RADIUS);
	setLuaGlobalVariable("CONST_BLOBBY_HEAD_RADIUS", BLOBBY_UPPER_RADIUS);
	setLuaGlobalVariable("CONST_BLOBBY_HEIGHT", BLOBBY_HEIGHT);
	setLuaGlobalVariable("CONST_BLOBBY_GRAVITY", GRAVITATION);
	setLuaGlobalVariable("CONST_NET_HEIGHT", NET_SPHERE_POSITION);
	setLuaGlobalVariable("CONST_NET_RADIUS", NET_RADIUS);
	setLuaGlobalVariable("NO_PLAYER", NO_PLAYER);
	setLuaGlobalVariable("LEFT_PLAYER", LEFT_PLAYER);
	setLuaGlobalVariable("RIGHT_PLAYER", RIGHT_PLAYER);
	
	// now load script file
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
		throw except;
	}
}

LuaGameLogic::~LuaGameLogic()
{
	lua_close(mState);
}

void LuaGameLogic::setLuaGlobalVariable(const char* name, double value)
{
	lua_pushnumber(mState, value);
	lua_setglobal(mState, name);
}

PlayerSide LuaGameLogic::checkWin() const
{
	bool won = false;
	lua_getglobal(mState, "IsWinning");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
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
	lua_getglobal(mState, "HandleInput");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
		return FallbackGameLogic::handleInput(ip, player);
	}
	lua_pushnumber(mState, player);
	lua_pushboolean(mState, ip.left);
	lua_pushboolean(mState, ip.right);
	lua_pushboolean(mState, ip.up);
	if(lua_pcall(mState, 1, 0, 0)) 
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
	
	PlayerInput ret;
	ret.up = lua_toboolean(mState, -1);
	ret.right = lua_toboolean(mState, -2);
	ret.left = lua_toboolean(mState, -3);
	return ret;
}

void LuaGameLogic::OnMistake(PlayerSide side) 
{
	lua_getglobal(mState, "OnMistake");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
		FallbackGameLogic::OnMistake(side);
		return;
	}
	lua_pushnumber(mState, side);
	if(lua_pcall(mState, 1, 0, 0)) 
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
}

void LuaGameLogic::OnBallHitsPlayerHandler(PlayerSide side)
{
	lua_getglobal(mState, "OnBallHitsPlayer");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
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
	lua_getglobal(mState, "OnBallHitsWall");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
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
	lua_getglobal(mState, "OnBallHitsNet");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
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
	lua_getglobal(mState, "OnBallHitsGround");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
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

void LuaGameLogic::OnGameHandler()
{
	lua_getglobal(mState, "OnGame");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
		FallbackGameLogic::OnGameHandler();
		return;
	}
	if( lua_pcall(mState, 0, 0, 0) )
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
	};
}

int LuaGameLogic::luaTouches(lua_State* state)
{
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	LuaGameLogic* gl = (LuaGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	PlayerSide side = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	lua_pushnumber(state, gl->getTouches(side));
	return 1;
}

int LuaGameLogic::luaLaunched(lua_State* state)
{
	lua_getglobal(state, "__MATCH_POINTER");
	DuelMatch* match = (DuelMatch*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	PlayerSide side = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	lua_pushnumber(state, match->getBlobJump(side));
	return 1;
}

int LuaGameLogic::luaBallX(lua_State* state)
{
	lua_getglobal(state, "__MATCH_POINTER");
	DuelMatch* match = (DuelMatch*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	float pos = match->getBallPosition().x;
	lua_pushnumber(state, pos);
	return 1;
}

int LuaGameLogic::luaBallY(lua_State* state)
{
	lua_getglobal(state, "__MATCH_POINTER");
	DuelMatch* match = (DuelMatch*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	float pos = match->getBallPosition().y;
	lua_pushnumber(state, pos);
	return 1;
}

int LuaGameLogic::luaBSpeedX(lua_State* state)
{
	lua_getglobal(state, "__MATCH_POINTER");
	DuelMatch* match = (DuelMatch*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	float vel = match->getBallVelocity().x;
	lua_pushnumber(state, vel);
	return 1;
}

int LuaGameLogic::luaBSpeedY(lua_State* state)
{
	lua_getglobal(state, "__MATCH_POINTER");
	DuelMatch* match = (DuelMatch*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	float vel = match->getBallVelocity().y;
	lua_pushnumber(state, vel);
	return 1;
}

int LuaGameLogic::luaPosX(lua_State* state)
{
	lua_getglobal(state, "__MATCH_POINTER");
	DuelMatch* match = (DuelMatch*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	PlayerSide side = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	float pos = match->getBlobPosition(side).x;
	lua_pushnumber(state, pos);
	return 1;
}

int LuaGameLogic::luaPosY(lua_State* state)
{
	lua_getglobal(state, "__MATCH_POINTER");
	DuelMatch* match = (DuelMatch*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	PlayerSide side = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	float pos = match->getBlobPosition(side).y;
	lua_pushnumber(state, pos);
	return 1;
}

int LuaGameLogic::luaSpeedX(lua_State* state)
{
	lua_getglobal(state, "__MATCH_POINTER");
	DuelMatch* match = (DuelMatch*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	PlayerSide side = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	float pos = match->getBlobVelocity(side).x;
	lua_pushnumber(state, pos);
	return 1;
}

int LuaGameLogic::luaSpeedY(lua_State* state)
{
	lua_getglobal(state, "__MATCH_POINTER");
	DuelMatch* match = (DuelMatch*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	PlayerSide side = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	float pos = match->getBlobVelocity(side).y;
	lua_pushnumber(state, pos);
	return 1;
}

int LuaGameLogic::luaGetScore(lua_State* state)
{
	int pl = int(lua_tonumber(state, -1) + 0.5);
	lua_pop(state, 1);
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	LuaGameLogic* gl = (LuaGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	lua_pushnumber(state, gl->getScore((PlayerSide)pl));
	return 1;
}

int LuaGameLogic::luaMistake(lua_State* state) 
{
	int pl = int(lua_tonumber(state, -1) + 0.5);
	lua_pop(state, 1);
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	LuaGameLogic* gl = (LuaGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	gl->onError((PlayerSide)pl);
	return 0;
}

int LuaGameLogic::luaScore(lua_State* state) 
{
	bool am = lua_tointeger(state, -1);
	lua_pop(state, 1);
	bool sc = lua_toboolean(state, -1);
	lua_pop(state, 1);
	int pl = int(lua_tonumber(state, -1) + 0.5);
	lua_pop(state, 1);
	lua_getglobal(state, "__GAME_LOGIC_POINTER");
	LuaGameLogic* gl = (LuaGameLogic*)lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	gl->score((PlayerSide)pl, sc, am);
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
