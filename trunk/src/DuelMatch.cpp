#include "RenderManager.h"
#include "SoundManager.h"
#include "DuelMatch.h"

DuelMatch::DuelMatch(InputSource* linput, InputSource* rinput, bool output)
{
	mLeftInput = linput;
	mRightInput = rinput;
	mOutput = output;

	mLeftScore = 0;
	mRightScore = 0;
	mServingPlayer = -1;                
	mLeftHitcount = 0;
	mRightHitcount = 0;

	mSquishLeft = 0;
	mSquishRight = 0;
	
	mWinningPlayer = 0;

	mPhysicWorld.resetPlayer();
	mPhysicWorld.step();
};

DuelMatch::~DuelMatch()
{
	
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
		rmanager->setBlob(0, mPhysicWorld.getLeftBlob(),
			mPhysicWorld.getLeftBlobState());
		rmanager->setBlob(1, mPhysicWorld.getRightBlob(),
			mPhysicWorld.getRightBlobState());
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
		if(mSquishLeft > 5)
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
		if(mSquishRight > 5)
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
		mServingPlayer = 1;
		mPhysicWorld.setBallValidity(0);
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
		mServingPlayer = 0;
		mPhysicWorld.setBallValidity(0);
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
		mPhysicWorld.reset(mServingPlayer);
}


PlayerSide DuelMatch::winningPlayer()
{
	if (mLeftScore >= 15 && mLeftScore >= mRightScore + 2)
		return LEFT_PLAYER;
	if (mRightScore >= 15 && mRightScore >= mLeftScore + 2)
		return RIGHT_PLAYER;
	return NO_PLAYER;
}
