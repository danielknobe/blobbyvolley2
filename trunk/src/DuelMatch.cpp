#include "RenderManager.h"
#include "SoundManager.h"
#include "DuelMatch.h"

#include <cassert>

DuelMatch* DuelMatch::mMainGame = 0;

DuelMatch::DuelMatch(InputSource* linput, InputSource* rinput,
				bool output, bool global)
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

	mLeftScore = 0;
	mRightScore = 0;
	mServingPlayer = NO_PLAYER;
	mLeftHitcount = 0;
	mRightHitcount = 0;

	mSquishLeft = 0;
	mSquishRight = 0;
	
	mBallDown = false;
	mWinningPlayer = 0;

	mPhysicWorld.resetPlayer();
	mPhysicWorld.step();
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
	
	mPhysicWorld.setLeftInput(mLeftInput->getInput());
	mPhysicWorld.setRightInput(mRightInput->getInput());
	mPhysicWorld.step();

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
	if (0 == mSquishLeft)
	{
		if (mPhysicWorld.ballHitLeftPlayer())
		{
			if (mOutput)
				smanager->playSound("sounds/bums.wav",
					mPhysicWorld.lastHitIntensity() + 0.4);
			mLeftHitcount++;
			mRightHitcount = 0;
			mSquishLeft = 1;
		}
	}
	else
	{
		mSquishLeft += 1;
		if(mSquishLeft > 9)
			mSquishLeft=0;
	}
	
	if(0 == mSquishRight)
	{
		if (mPhysicWorld.ballHitRightPlayer())
		{
			if (mOutput)
				smanager->playSound("sounds/bums.wav",
					mPhysicWorld.lastHitIntensity() + 0.4);
			mRightHitcount++;
			mLeftHitcount = 0;
			mSquishRight = 1;
		}
	}
	else
	{
		mSquishRight += 1;
		if(mSquishRight > 9)
			mSquishRight=0;	
	}
	
	if (mPhysicWorld.ballHitLeftGround() || mLeftHitcount > 3)
	{
		if (mLeftHitcount > 3)
			mPhysicWorld.dampBall();
		if (mOutput)
			smanager->playSound("sounds/pfiff.wav", 0.2);
		if (mServingPlayer == 1)
			mRightScore++;
		mServingPlayer = RIGHT_PLAYER;
		mPhysicWorld.setBallValidity(0);
		mBallDown = true;
		mRightHitcount = 0;
		mLeftHitcount = 0;
	}
	
	if (mPhysicWorld.ballHitRightGround() || mRightHitcount > 3)
	{
		if(mRightHitcount > 3)
			mPhysicWorld.dampBall();
		if (mOutput)
			smanager->playSound("sounds/pfiff.wav", 0.2);
		if (mServingPlayer == 0)
			mLeftScore++;
		mServingPlayer = LEFT_PLAYER;
		mPhysicWorld.setBallValidity(0);
		mBallDown = true;
		mRightHitcount = 0;
		mLeftHitcount = 0;
	}

	if (mOutput)
	{
		// This is done seperate from other output because the
		// winning screen would display old scores otherwise
		rmanager->setScore(mLeftScore, mRightScore,
			mServingPlayer == 0, mServingPlayer == 1);
	}
	
	if (mPhysicWorld.roundFinished())
	{
		mBallDown = false;
		mPhysicWorld.reset(mServingPlayer);
	}
}


PlayerSide DuelMatch::winningPlayer()
{
	if (mLeftScore >= 15 && mLeftScore >= mRightScore + 2)
		return LEFT_PLAYER;
	if (mRightScore >= 15 && mRightScore >= mLeftScore + 2)
		return RIGHT_PLAYER;
	return NO_PLAYER;
}

int DuelMatch::getHitcount(PlayerSide player)
{
	if (player == LEFT_PLAYER)
		return mLeftHitcount;
	else if (player == RIGHT_PLAYER)
		return mRightHitcount;
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
	PlayerSide side = mServingPlayer;
	if (side == NO_PLAYER)
	{
		// This not exactly the situation, but it is necessary to tell
		// the script system this value, so it doesn't get confused.
		side = LEFT_PLAYER;
	}
	return side;
}
