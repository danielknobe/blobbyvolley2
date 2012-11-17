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
#include "DuelMatch.h"

/* includes */
#include <cassert>

#include "DuelMatchState.h"
#include "MatchEvents.h"
#include "PhysicWorld.h"
#include "GenericIO.h"

/* implementation */

DuelMatch* DuelMatch::mMainGame = 0;

DuelMatch::DuelMatch(InputSource* linput, InputSource* rinput, bool global, bool remote, std::string rules) :
		// we send a pointer to an unconstructed object here!
		mLogic(createGameLogic(rules, this)),
		mPaused(false), 
		events(0), 
		external_events(0), 
		mRemote(remote)
{
	mPhysicWorld.reset( new PhysicWorld() );
	mGlobal = global;
	if (mGlobal)
	{
		assert(mMainGame == 0);
		mMainGame = this;
	}

	mLeftInput = linput ? linput : new InputSource();
	mRightInput = rinput ? rinput : new InputSource();
}

void DuelMatch::reset()
{
	mPhysicWorld.reset(new PhysicWorld());
	mLogic = mLogic->clone();
}

DuelMatch::~DuelMatch()
{
	if (mGlobal)
	{
		mMainGame = 0;
	}
}

DuelMatch* DuelMatch::getMainGame()
{
	return mMainGame;
}

void DuelMatch::setRules(std::string rulesFile)
{
	mLogic = createGameLogic(rulesFile, this);
}


void DuelMatch::step()
{
	events = external_events;

	// in pause mode, step does nothing
	if(mPaused)
		return;
		
	mTransformedInput[LEFT_PLAYER] = mLeftInput->updateInput();
	mTransformedInput[RIGHT_PLAYER] = mRightInput->updateInput();
	
	int leftScore = mLogic->getScore(LEFT_PLAYER);
	int rightScore = mLogic->getScore(RIGHT_PLAYER);

	if(!mRemote)
	{
		mTransformedInput[LEFT_PLAYER] = mLogic->transformInput( mTransformedInput[LEFT_PLAYER], LEFT_PLAYER );
		mTransformedInput[RIGHT_PLAYER] = mLogic->transformInput( mTransformedInput[RIGHT_PLAYER], RIGHT_PLAYER );
	}

	// do steps in physic an logic
	mLogic->step();
	int physicEvents = mPhysicWorld->step( mTransformedInput[LEFT_PLAYER], mTransformedInput[RIGHT_PLAYER], 
											mLogic->isBallValid(), mLogic->isGameRunning() );

	// check for all hit events
	if(!mRemote)
		events |= physicEvents;
	
	// process events
	if(events & EVENT_LEFT_BLOBBY_HIT)
		mLogic->onBallHitsPlayer(LEFT_PLAYER);
	
	if(events & EVENT_RIGHT_BLOBBY_HIT)
		mLogic->onBallHitsPlayer(RIGHT_PLAYER);
	
	if(events & EVENT_BALL_HIT_LEFT_GROUND)
		mLogic->onBallHitsGround(LEFT_PLAYER);
	
	if(events & EVENT_BALL_HIT_RIGHT_GROUND)
		mLogic->onBallHitsGround(RIGHT_PLAYER);
	
	if(events & EVENT_BALL_HIT_LEFT_WALL)
		mLogic->onBallHitsWall(LEFT_PLAYER);

	if(events & EVENT_BALL_HIT_RIGHT_WALL)
		mLogic->onBallHitsWall(RIGHT_PLAYER);

	if(events & EVENT_BALL_HIT_NET_LEFT)
		mLogic->onBallHitsNet(LEFT_PLAYER);

	if(events & EVENT_BALL_HIT_NET_RIGHT)
		mLogic->onBallHitsNet(RIGHT_PLAYER);

	if(events & EVENT_BALL_HIT_NET_TOP)
		mLogic->onBallHitsNet(NO_PLAYER);
	

	switch(mLogic->getLastErrorSide()){
		case LEFT_PLAYER:
			events |= EVENT_ERROR_LEFT;
		case RIGHT_PLAYER:
			// if the error was caused by the right player
			// reset EVENT_ERROR_LEFT
			events &= ~EVENT_ERROR_LEFT;
			events |= EVENT_ERROR_RIGHT;
			mPhysicWorld->setBallVelocity( mPhysicWorld->getBallVelocity().scale(0.6) );
			break;
		default:
			if ((events & EVENT_BALL_HIT_GROUND) && !mLogic->isBallValid())
			{
				mPhysicWorld->setBallVelocity( mPhysicWorld->getBallVelocity().scale(0.6) );
			}
			break;
	}

	// if the round is finished, we 
	// reset BallDown, reset the World
	// to let the player serve
	// and trigger the EVENT_RESET
	if (!mLogic->isBallValid() && mPhysicWorld->canStartRound(mLogic->getServingPlayer()))
	{
		mPhysicWorld->reset(mLogic->getServingPlayer());
		mLogic->onServe();
		events |= EVENT_RESET;
	}
	
	// if score was changed, we send it to clients
	if (!mRemote && (leftScore != mLogic->getScore(LEFT_PLAYER) || rightScore != mLogic->getScore(RIGHT_PLAYER)))
	{
		events |= EVENT_SEND_SCORE;
	}
	
	// reset external events
	external_events = 0;
}

void DuelMatch::setScore(int left, int right)
{
	mLogic->setScore(LEFT_PLAYER, left);
	mLogic->setScore(RIGHT_PLAYER, right);
}

void DuelMatch::setLastHitIntensity(float intensity)
{
	mPhysicWorld->setLastHitIntensity(intensity);
}

void DuelMatch::trigger(int event)
{
	external_events |= event;
}

void DuelMatch::resetTriggeredEvents()
{
	external_events = 0;
}

void DuelMatch::pause()
{
	mLogic->onPause();
	mPaused = true;
}

void DuelMatch::unpause()
{
	mLogic->onUnPause();
	mPaused = false;
}

PlayerSide DuelMatch::winningPlayer()
{
	return mLogic->getWinningPlayer();
}

int DuelMatch::getHitcount(PlayerSide player) const
{
	if (player == LEFT_PLAYER)
		return mLogic->getTouches(LEFT_PLAYER);
	else if (player == RIGHT_PLAYER)
		return mLogic->getTouches(RIGHT_PLAYER);
	else
		return 0;
}

int DuelMatch::getScore(PlayerSide player) const
{
	return mLogic->getScore(player);
}

int DuelMatch::getScoreToWin() const 
{
	return mLogic->getScoreToWin();
}

bool DuelMatch::getBallDown() const
{
	return !mLogic->isBallValid();
}

bool DuelMatch::getBallActive() const 
{
	return mLogic->isGameRunning();
}

bool DuelMatch::getBlobJump(PlayerSide player) const
{
	return mPhysicWorld->getBlobJump(player);
}

Vector2 DuelMatch::getBlobPosition(PlayerSide player) const
{
	if (player == LEFT_PLAYER || player == RIGHT_PLAYER)
		return mPhysicWorld->getBlobPosition(player);
	else
		return Vector2(0.0, 0.0);
}

Vector2 DuelMatch::getBlobVelocity(PlayerSide player) const
{
	if (player == LEFT_PLAYER || player == RIGHT_PLAYER)
		return mPhysicWorld->getBlobVelocity(player);
	else
		return Vector2(0.0, 0.0);
}

Vector2 DuelMatch::getBallPosition() const
{
	return mPhysicWorld->getBallPosition();
}

Vector2 DuelMatch::getBallVelocity() const
{
	return mPhysicWorld->getBallVelocity();
}

PlayerSide DuelMatch::getServingPlayer() const
{	// NO_PLAYER hack was moved into ScriptedInpurSource.cpp
	return mLogic->getServingPlayer();
}

void DuelMatch::setState(const DuelMatchState& state)
{
	mPhysicWorld->setState(state.worldState);
	mLogic->setState(state.logicState);
	
	mTransformedInput[LEFT_PLAYER] = state.playerInput[LEFT_PLAYER];
	mTransformedInput[RIGHT_PLAYER] = state.playerInput[RIGHT_PLAYER];
	
	mLeftInput->setInput(mTransformedInput[LEFT_PLAYER]);
	mRightInput->setInput(mTransformedInput[RIGHT_PLAYER]);
}

DuelMatchState DuelMatch::getState() const
{
	DuelMatchState state;
	state.worldState = mPhysicWorld->getState();
	state.logicState = mLogic->getState();
	state.playerInput[LEFT_PLAYER] = mTransformedInput[LEFT_PLAYER];
	state.playerInput[RIGHT_PLAYER] = mTransformedInput[RIGHT_PLAYER];
	
	return state;
}

void DuelMatch::setServingPlayer(PlayerSide side)
{
	mLogic->setServingPlayer(side);
	mLogic->onServe();
	mPhysicWorld->reset(side);
}

const Clock& DuelMatch::getClock() const
{
	return mLogic->getClock();
}

Clock& DuelMatch::getClock()
{
	return mLogic->getClock();
}

InputSource* DuelMatch::getInputSource(PlayerSide player) const
{
	if(player == LEFT_PLAYER)
	{
		return mLeftInput;
	} 
	 else if (player == RIGHT_PLAYER)
	{
		return mRightInput;
	}
	
	return 0;
}
