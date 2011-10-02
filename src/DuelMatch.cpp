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

#include "UserConfig.h"
#include "DuelMatch.h"

#include <cassert>

DuelMatch* DuelMatch::mMainGame = 0;

DuelMatch::DuelMatch(InputSource* linput, InputSource* rinput,
				bool global, bool remote):mLogic(createGameLogic("rules.lua")),
					mPaused(false), events(0), external_events(0), mRemote(remote)
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

	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");
	mLogic->setScoreToWin(gameConfig.getInteger("scoretowin"));
};

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

void DuelMatch::step()
{
	events = external_events;

	// do steps in physic an logic
	if (mLeftInput)
		mPhysicWorld.setLeftInput(mLeftInput->getInput());
	if (mRightInput)
		mPhysicWorld.setRightInput(mRightInput->getInput());
	mPhysicWorld.step();
	mLogic->step();
	
	// check for all hit events
	if(!mRemote)
	{
		if (mPhysicWorld.ballHitLeftPlayer())
			events |= EVENT_LEFT_BLOBBY_HIT;

		if (mPhysicWorld.ballHitRightPlayer())
			events |= EVENT_RIGHT_BLOBBY_HIT;

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
			
			mPhysicWorld.setBallValidity(0);
			mBallDown = true;
			break;
		
	}

	if (mPhysicWorld.roundFinished())
	{
		mBallDown = false;
		mPhysicWorld.reset(mLogic->getServingPlayer());
		events |= EVENT_RESET;
	}
	
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
		return mLogic->getHits(LEFT_PLAYER);
	else if (player == RIGHT_PLAYER)
		return mLogic->getHits(RIGHT_PLAYER);
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
	mPhysicWorld.setState(stream);
}

const PlayerInput* DuelMatch::getPlayersInput() const
{
	return mPhysicWorld.getPlayersInput();
}

void DuelMatch::setPlayersInput(const PlayerInput* input)
{
	mPhysicWorld.setLeftInput(input[LEFT_PLAYER]);
	mPhysicWorld.setRightInput(input[RIGHT_PLAYER]);
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
