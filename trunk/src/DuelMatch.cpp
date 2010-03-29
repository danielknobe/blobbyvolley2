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
				bool global):mLogic(createGameLogic(OLD_RULES)), events(0)
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
	events = 0;

	// do steps in physic an logic
	if (mLeftInput)
		mPhysicWorld.setLeftInput(mLeftInput->getInput());
	if (mRightInput)
		mPhysicWorld.setRightInput(mRightInput->getInput());
	mPhysicWorld.step();
	mLogic->step();
	
	// check for all hit events
	if (mPhysicWorld.ballHitLeftPlayer() && mLogic->isCollisionValid(LEFT_PLAYER))
	{
		events |= EVENT_LEFT_BLOBBY_HIT;
		mLogic->onBallHitsPlayer(LEFT_PLAYER);
	}

	if (mPhysicWorld.ballHitRightPlayer() && mLogic->isCollisionValid(RIGHT_PLAYER))
	{
		events |= EVENT_RIGHT_BLOBBY_HIT;
		mLogic->onBallHitsPlayer(RIGHT_PLAYER);
	}
	
	if(mPhysicWorld.ballHitLeftGround()){
		mLogic->onBallHitsGround(LEFT_PLAYER);
		events |= EVENT_BALL_HIT_GROUND;
	}
	
	if(mPhysicWorld.ballHitRightGround()){
		mLogic->onBallHitsGround(RIGHT_PLAYER);
		events |= EVENT_BALL_HIT_GROUND;
	}

	switch(mLogic->getLastErrorSide()){
		case LEFT_PLAYER:
		case RIGHT_PLAYER:
			events |= EVENT_ERROR;
			if (!events & EVENT_BALL_HIT_GROUND)
				mPhysicWorld.dampBall();
				
			mPhysicWorld.setBallValidity(0);
			mBallDown = true;
			break;
		
	}

	if (mPhysicWorld.roundFinished())
	{
		mBallDown = false;
		mPhysicWorld.reset(mLogic->getServingPlayer());
	}
}


PlayerSide DuelMatch::winningPlayer()
{
	return mLogic->getWinningPlayer();
}

int DuelMatch::getHitcount(PlayerSide player)
{
	if (player == LEFT_PLAYER)
		return mLogic->getHits(LEFT_PLAYER);
	else if (player == RIGHT_PLAYER)
		return mLogic->getHits(RIGHT_PLAYER);
	else
		return 0;
}

int DuelMatch::getScore(PlayerSide player)
{
	return mLogic->getScore(player);
}

bool DuelMatch::getBallDown()
{
	return mBallDown;
}

bool DuelMatch::getBallActive()
{
	return mPhysicWorld.getBallActive();
}


bool DuelMatch::getBlobJump(PlayerSide player)
{
	return mPhysicWorld.getBlobJump(player);
}

Vector2 DuelMatch::getBlobPosition(PlayerSide player)
{
	if (player == LEFT_PLAYER)
		return mPhysicWorld.getBlob(LEFT_PLAYER);
	else if (player == RIGHT_PLAYER)
		return mPhysicWorld.getBlob(RIGHT_PLAYER);
	else
		return Vector2(0.0, 0.0);
}

Vector2 DuelMatch::getBallPosition()
{
	return mPhysicWorld.getBall();
}

Vector2 DuelMatch::getBallVelocity()
{
	return mPhysicWorld.getBallVelocity();
}

float DuelMatch::getBallEstimation()
{
	return mPhysicWorld.estimateBallImpact();
}

Vector2 DuelMatch::getBallTimeEstimation(int steps)
{
	return mPhysicWorld.estimateBallPosition(steps);
}

PlayerSide DuelMatch::getServingPlayer()
{	// NO_PLAYER hack was moved into ScriptedInpurSource.cpp
	return mLogic->getServingPlayer();
}

void DuelMatch::setState(RakNet::BitStream* stream)
{
	mPhysicWorld.setState(stream);
}

const PlayerInput* DuelMatch::getPlayersInput()
{
	return mPhysicWorld.getPlayersInput();
}

void DuelMatch::setPlayersInput(PlayerInput* input)
{
	mPhysicWorld.setLeftInput(input[LEFT_PLAYER]);
	mPhysicWorld.setRightInput(input[RIGHT_PLAYER]);
}

void DuelMatch::setServingPlayer(PlayerSide side)
{
	mPhysicWorld.reset(side);
}
