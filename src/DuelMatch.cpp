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

#include "IUserConfigReader.h"
#include "UserConfig.h"
#include "DuelMatchState.h"


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
	mGlobal = global;
	if (mGlobal)
	{
		assert(mMainGame == 0);
		mMainGame = this;
	}

	mLeftInput = linput;
	mRightInput = rinput;

	mBallDown = false;

	mPhysicWorld.resetPlayer();
	mPhysicWorld.step();

	/// \todo we better pass this as a parameter so DuelMatch has no coupeling with UserConfigs...s
	mLogic->setScoreToWin(IUserConfigReader::createUserConfigReader("config.xml")->getInteger("scoretowin"));
}

void DuelMatch::reset()
{
	mPhysicWorld = PhysicWorld();
	mLogic = createGameLogic( mLogic->getSourceFile(), this);
	
	mBallDown = false;

	mPhysicWorld.resetPlayer();
	mPhysicWorld.step();
	
	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");
	mLogic->setScoreToWin(gameConfig.getInteger("scoretowin"));
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

	// do steps in physic an logic
	if (mLeftInput)
		mPhysicWorld.setLeftInput(mLeftInput->getInput());
	
	if (mRightInput)
		mPhysicWorld.setRightInput(mRightInput->getInput());
		
	// in pause mode, step does nothing except input being set
	if(mPaused)
		return;
	
	
	mPhysicWorld.step();
	mLogic->step();
	
	// check for all hit events
	if(!mRemote)
	{
		// create game events
		// ball/player hit events:
		if (mPhysicWorld.ballHitLeftPlayer() && mLogic->isCollisionValid(LEFT_PLAYER))
			events |= EVENT_LEFT_BLOBBY_HIT;

		if (mPhysicWorld.ballHitRightPlayer() && mLogic->isCollisionValid(RIGHT_PLAYER))
			events |= EVENT_RIGHT_BLOBBY_HIT;

		// ball/ground hit events:
		if(mPhysicWorld.ballHitLeftGround())
			events |= EVENT_BALL_HIT_LEFT_GROUND;

		if(mPhysicWorld.ballHitRightGround())
			events |= EVENT_BALL_HIT_RIGHT_GROUND;
	}
	
	// process events
	if (events & EVENT_LEFT_BLOBBY_HIT)
		mLogic->onBallHitsPlayer(LEFT_PLAYER);
	
	if (events & EVENT_RIGHT_BLOBBY_HIT)
		mLogic->onBallHitsPlayer(RIGHT_PLAYER);
	
	if(events & EVENT_BALL_HIT_LEFT_GROUND)
		mLogic->onBallHitsGround(LEFT_PLAYER);
	
	if(events & EVENT_BALL_HIT_RIGHT_GROUND)
		mLogic->onBallHitsGround(RIGHT_PLAYER);
	
	

	switch(mLogic->getLastErrorSide()){
		case LEFT_PLAYER:
			events |= EVENT_ERROR_LEFT;
		case RIGHT_PLAYER:
			// if the error was caused by the right player
			// reset EVENT_ERROR_LEFT
			events &= ~EVENT_ERROR_LEFT;
			events |= EVENT_ERROR_RIGHT;
			if (!(events & EVENT_BALL_HIT_GROUND))
				mPhysicWorld.dampBall();
			
			// now, the ball is not valid anymore
			mPhysicWorld.setBallValidity(0);
			/// \todo why do we set balldown?
			/// 		we could get here just
			///			by for hits
			mBallDown = true;
			break;
		
	}

	// if the round is finished, we 
	// reset BallDown, reset the World
	// to let the player serve
	// and trigger the EVENT_RESET
	if (mPhysicWorld.roundFinished())
	{
		mBallDown = false;
		mPhysicWorld.reset(mLogic->getServingPlayer());
		events |= EVENT_RESET;
	}
	
	// reset external events
	external_events = 0;
}

void DuelMatch::setScore(int left, int right)
{
	mLogic->setScore(LEFT_PLAYER, left);
	mLogic->setScore(RIGHT_PLAYER, right);
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
	return mBallDown;
}

bool DuelMatch::getBallActive() const 
{
	return mPhysicWorld.getBallActive();
}

bool DuelMatch::getBallValid() const
{
	return mPhysicWorld.getBallValid();
}


bool DuelMatch::getBlobJump(PlayerSide player) const
{
	return mPhysicWorld.getBlobJump(player);
}

Vector2 DuelMatch::getBlobPosition(PlayerSide player) const
{
	if (player == LEFT_PLAYER)
		return mPhysicWorld.getBlob(LEFT_PLAYER);
	else if (player == RIGHT_PLAYER)
		return mPhysicWorld.getBlob(RIGHT_PLAYER);
	else
		return Vector2(0.0, 0.0);
}

Vector2 DuelMatch::getBlobVelocity(PlayerSide player) const
{
	if (player == LEFT_PLAYER)
		return mPhysicWorld.getBlobVelocity(LEFT_PLAYER);
	else if (player == RIGHT_PLAYER)
		return mPhysicWorld.getBlobVelocity(RIGHT_PLAYER);
	else
		return Vector2(0.0, 0.0);
}

Vector2 DuelMatch::getBallPosition() const
{
	return mPhysicWorld.getBall();
}

Vector2 DuelMatch::getBallVelocity() const
{
	return mPhysicWorld.getBallVelocity();
}

PlayerSide DuelMatch::getServingPlayer() const
{	// NO_PLAYER hack was moved into ScriptedInpurSource.cpp
	return mLogic->getServingPlayer();
}

void DuelMatch::setState(RakNet::BitStream* stream)
{
	PhysicState ps = mPhysicWorld.getState();
	ps.readFromStream(stream);
	mPhysicWorld.setState(ps);
}

void DuelMatch::setState(const DuelMatchState& state)
{
	mPhysicWorld.setState(state.worldState);
	mLogic->setState(state.logicState);
}

DuelMatchState DuelMatch::getState() const
{
	DuelMatchState state;
	state.worldState = mPhysicWorld.getState();
	state.logicState = mLogic->getState();
	
	return state;
}

const PlayerInput* DuelMatch::getPlayersInput() const
{
	return mPhysicWorld.getPlayersInput();
}

void DuelMatch::setPlayersInput(const PlayerInput& left, const PlayerInput& right)
{
	mPhysicWorld.setLeftInput( left );
	mPhysicWorld.setRightInput( right );
}

void DuelMatch::setServingPlayer(PlayerSide side)
{
	mLogic->setServingPlayer(side);
	mPhysicWorld.reset(side);
}

const Clock& DuelMatch::getClock() const
{
	return mLogic->getClock();
}

Clock& DuelMatch::getClock()
{
	return mLogic->getClock();
}
