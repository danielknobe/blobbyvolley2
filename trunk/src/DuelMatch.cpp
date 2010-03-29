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
#include "RenderManager.h"
#include "SoundManager.h"
#include "DuelMatch.h"
#include "Blood.h"

#include <cassert>

DuelMatch* DuelMatch::mMainGame = 0;

DuelMatch::DuelMatch(InputSource* linput, InputSource* rinput,
				bool output, bool global):mLogic(createGameLogic(OLD_RULES))
{
	mGlobal = global;
	if (mGlobal)
	{
		assert(mMainGame == 0);
		mMainGame = this;
	}

	mLeftInput = linput;
	mRightInput = rinput;
	mOutput = output;

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
	RenderManager* rmanager = &RenderManager::getSingleton();
	SoundManager* smanager = &SoundManager::getSingleton();

	if (mLeftInput)
		mPhysicWorld.setLeftInput(mLeftInput->getInput());
	if (mRightInput)
		mPhysicWorld.setRightInput(mRightInput->getInput());
	mPhysicWorld.step();
	mLogic->step();

	if (mOutput)
	{
		rmanager->setBlob(0, mPhysicWorld.getBlob(LEFT_PLAYER),
			mPhysicWorld.getBlobState(LEFT_PLAYER));
		rmanager->setBlob(1, mPhysicWorld.getBlob(RIGHT_PLAYER),
			mPhysicWorld.getBlobState(RIGHT_PLAYER));
		rmanager->setBall(mPhysicWorld.getBall(),
				mPhysicWorld.getBallRotation());
	}

	// Protection of multiple hit counts when the ball is squeezed
	if (mPhysicWorld.ballHitLeftPlayer())
	{
		if (mLogic->onBallHitsPlayer(LEFT_PLAYER) && mOutput)
		{
			smanager->playSound("sounds/bums.wav",
				mPhysicWorld.lastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
			Vector2 hitPos = mPhysicWorld.getBall() +
				(mPhysicWorld.getBlob(LEFT_PLAYER) - mPhysicWorld.getBall()).normalise().scale(31.5);
			BloodManager::getSingleton().spillBlood(hitPos, mPhysicWorld.lastHitIntensity(), 0);
		}
	}

	if (mPhysicWorld.ballHitRightPlayer())
	{
		if (mLogic->onBallHitsPlayer(RIGHT_PLAYER) && mOutput)
		{
			smanager->playSound("sounds/bums.wav",
				mPhysicWorld.lastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
			Vector2 hitPos = mPhysicWorld.getBall() +
				(mPhysicWorld.getBlob(RIGHT_PLAYER) - mPhysicWorld.getBall()).normalise().scale(31.5);
			BloodManager::getSingleton().spillBlood(hitPos, mPhysicWorld.lastHitIntensity(), 1);
		}
	}
	
	if(mPhysicWorld.ballHitLeftGround())
		mLogic->onBallHitsGround(LEFT_PLAYER);
	
	if(mPhysicWorld.ballHitRightGround())
		mLogic->onBallHitsGround(RIGHT_PLAYER);

	switch(mLogic->getLastErrorSide()){
		case LEFT_PLAYER:
			if (!mPhysicWorld.ballHitLeftGround())
				mPhysicWorld.dampBall();
			if (mOutput)
				smanager->playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
			mPhysicWorld.setBallValidity(0);
			mBallDown = true;
			break;
			
		case RIGHT_PLAYER:
			if(!mPhysicWorld.ballHitRightGround())
				mPhysicWorld.dampBall();
			if (mOutput)
				smanager->playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
			mPhysicWorld.setBallValidity(0);
			mBallDown = true;
			break;
		
	}

	if (mOutput)
	{
		// This is done seperate from other output because the
		// winning screen would display old scores otherwise
		rmanager->setScore(mLogic->getScore(LEFT_PLAYER), mLogic->getScore(RIGHT_PLAYER),
			mLogic->getServingPlayer() == 0, mLogic->getServingPlayer() == 1);
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
{
	PlayerSide side = mLogic->getServingPlayer();
	if (side == NO_PLAYER)
	{
		// This not exactly the situation, but it is necessary to tell
		// the script system this value, so it doesn't get confused.
		side = LEFT_PLAYER;
	}
	return side;
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
