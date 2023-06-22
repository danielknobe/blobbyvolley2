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
#include <boost/exception/all.hpp>

#include <SDL.h>

#include "lua.hpp"

#include "DuelMatch.h"
#include "DuelMatchState.h"
#include "GameConstants.h"

/* implementation */

ScriptedInputSource::ScriptedInputSource(const std::string& filename, PlayerSide playerside, unsigned int difficulty,
										 const DuelMatch* match)
: mSide(playerside)
, mDifficulty(difficulty)
, mMatch(match)
{
	mStartTime = SDL_GetTicks();

	// set game constants
	setGameConstants();
	setGameFunctions();

	// push infos into script
	lua_pushnumber(mState, mDifficulty / 25.0);
	lua_setglobal(mState, "__DIFFICULTY");
	lua_pushinteger(mState, mSide);
	lua_setglobal(mState, "__SIDE");

	openScript("api");
	openScript("bot_api");
	openScript(filename);

	// check whether all required lua functions are available
	bool step = getLuaFunction("__OnStep");
	if (!step)
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

ScriptedInputSource::~ScriptedInputSource() = default;

void ScriptedInputSource::setWaitTime(int wait_in_ms) {
	mWaitTime = wait_in_ms;
}

PlayerInputAbs ScriptedInputSource::getNextInput()
{
	DuelMatchState state = mMatch->getState();
	if(mSide == RIGHT_PLAYER) {
		state.swapSides();
	}

	// if the ball is on the bots side, decrease the error timers more quickly.
	// if we did not do this, then the difficulty would have almost no effect on
	// any plays made by the player from the back of their half, as the counters
	// would have reached zero by the time the ball arrives at the bot's half. This
	// way, we can give the counters larger initial values, and still don't get completely
	// stupid play.
	if(state.getBallPosition().x < 400) {
		--mBallPosErrorTimer;
		--mReactionTime;
	}

	// decrement counters regularly
	--mReactionTime;
	--mBallPosErrorTimer;

	// bot has not reacted yet
	if( mReactionTime > 0 && mDifficulty > 0 ) {
		return {false, false, false};
	}

	// mis-estimated ball velocity handling
	if(mBallPosErrorTimer > 0)
	{
		mBallPosError += mBallVelError;
	}
	else
	{
		mBallPosError.clear();
		mBallVelError.clear();
	}

	state.worldState.ballPosition += mBallPosError;
	state.worldState.ballVelocity += mBallVelError;
	state.worldState.blobPosition[LEFT_PLAYER].x += mBlobPosError;

	setMatchState( state );


	bool serving = false;
	// reset input
	lua_pushboolean(mState, false);
	lua_setglobal(mState, "__WANT_LEFT");
	lua_pushboolean(mState, false);
	lua_setglobal(mState, "__WANT_RIGHT");
	lua_pushboolean(mState, false);
	lua_setglobal(mState, "__WANT_JUMP");

	lua_getglobal(mState, "__OnStep");
	callLuaFunction();

	if (!mMatch->getBallActive() && mSide ==
			// if no player is serving player, assume the left one is
			(mMatch->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : mMatch->getServingPlayer() ))
	{
		serving = true;
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

	if (mStartTime + mWaitTime > SDL_GetTicks() && serving)
		return {};

	if(!mMatch->getBallActive())
	{
		if(!serving || mDifficulty < 15) {
			mRoundStepCounter = 0;
			setInputDelay(0);
		}
	}
	else
	{
		mRoundStepCounter += 1;
	}

	// whenever the opponent touches the ball, the bot pauses for a short, random amount of time
	// to orient itself. This time depends on the current difficulty level, and increases as the game
	// progresses.
	int opp_touches = state.getHitcount(RIGHT_PLAYER);
	if( opp_touches != mOldOppTouches)
	{
		mOldOppTouches = opp_touches;
		// the number of opponent touches get reset to zero if the bot touches the ball -- ignore these cases
		if(opp_touches != 0)
		{
			int base_difficulty = std::max( 0, 2 * mDifficulty - 30 );
			int max_difficulty = std::min(75, base_difficulty + 2 * getCurrentDifficulty());
			std::uniform_int_distribution<int> dist{base_difficulty, max_difficulty};
			setInputDelay( dist(mRandom) );
		}
	}

	int own_touches = state.getHitcount(LEFT_PLAYER);
	// for very easy difficulties, we modify the "perceived" x-coordinate of the bot's blob.
	// This needs to happen whenever the bot's blob touches the ball - if we had it also based
	// on `opp_touches`, then the errors while the ball is on the bot's side would be identical
	// for all its attempts to play the ball, which can look a bit stupid. This way, it is less
	// likely to lead directly to a point for the player, but still takes the "speed" out of the
	// bot's game.
	if(own_touches != mOldOwnTouches) {
		mOldOwnTouches = own_touches;
		// Note that this shift is one-sided, making the bot think it is closer to the wall than
		// it really is. This will result in it standing further to the net in reality, and being
		// less likely to play aggressive.
		std::uniform_int_distribution<int> dice{0, 100};
		if( dice(mRandom) < (mDifficulty - 15) * 8 && mSide == LEFT_PLAYER ) {
			std::uniform_real_distribution<float> error_dist{0.f, BALL_RADIUS};
			mBlobPosError = -error_dist( mRandom );
		} else {
			mBlobPosError = 0;
		}
	}

	// check if the x-velocity of the ball has changed. This only happens when the ball collides with something.
	// as this results in a change of trajectory of the ball, this is a relatively natural place for the bot to
	// change its estimated position and start moving the blob.
	// important: get the actual speed, not the simulated one. Otherwise, applying the error would trigger this condition
	// immediately again.
	float bv_x = mMatch->getBallVelocity().x;
	if(bv_x != mOldBallVx) {
		mOldBallVx = bv_x;
		// don't apply an error after every collision -- results in very jittery bot.
		// instead, only do this in a fraction of the cases, up to 25% for very easy.
		std::uniform_int_distribution<int> dist{0, 100};
		if( dist( mRandom ) < mDifficulty ) {
			// generate a random amount, and random duration, for the error effect.
			float amount = std::min(25, getCurrentDifficulty()) / 50.f + std::max(0, mDifficulty - 5) / 25.f;
			int err_time = 25 + mDifficulty + std::min(75, getCurrentDifficulty());
			setBallError(err_time, amount);
		}
	}

	PlayerInputAbs raw_input{wantleft, wantright, wantjump};
	if(mSide == RIGHT_PLAYER) {
		raw_input.swapSides();
	}
	return raw_input;
}

void ScriptedInputSource::setInputDelay(int delay)
{
	if(delay <= 0)
	{
		delay = 0;
	} else {
		delay = std::max(delay, mReactionTime);
	}

	mReactionTime = delay;
}

int ScriptedInputSource::getCurrentDifficulty() const
{
	int exchange_seconds = mRoundStepCounter / 75;
	float difficulty_effect = std::sqrt( mDifficulty );
	// minimum game time until the bot starts making mistakes:
	// ~5 minutes at highest difficulty, 10 seconds for very easy.
	int min_duration = 300 - static_cast<int>(difficulty_effect * 58);
	int offset_seconds = std::max( 0, exchange_seconds - min_duration );
	int diff_mod = (offset_seconds * mDifficulty) / 25;
	return diff_mod;
}

void ScriptedInputSource::setBallError(int duration, float amount)
{
	mBallPosErrorTimer = duration;
	std::uniform_real_distribution<float> f_dist{};
	float angle = 2 * M_PI * f_dist(mRandom);
	mBallVelError.x = std::sin(angle) * amount;
	mBallVelError.y = std::cos(angle) * amount;
	mBallPosError = mBallVelError * 5;
}
