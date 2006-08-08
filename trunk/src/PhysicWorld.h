#pragma once

#include <iostream>

#include "Global.h"
#include "Vector.h"
#include "InputSource.h"

class PhysicWorld
{
private:
	// These methods tell about ball collisions for class-intern use
	bool int_BallHitLeftPlayerTop();
	bool int_BallHitRightPlayerTop();
	bool int_BallHitLeftPlayerBottom();
	bool int_BallHitRightPlayerBottom();
	bool int_BallHitLeftPlayer();
	bool int_BallHitRightPlayer();
	
	bool resetAreaClear();

	// is ball hit by player?
	bool mBallHitByLeftBlob;
	bool mBallHitByRightBlob;

	Vector2 mLeftBlobPosition;
	Vector2 mRightBlobPosition;
	Vector2 mBallPosition;

	Vector2 mLeftBlobVelocity;
	Vector2 mRightBlobVelocity;
	Vector2 mBallVelocity;
	Vector2 mBallVelocityTemp; // important to solve the netbug
	bool mBallNetOverlapped;   // important to solve the netbug
	
	float mBallRotation;
	float mBallAngularVelocity;
	float mLeftBlobState;
	float mRightBlobState;
	float mLeftBlobbyAnimationSpeedAtm;
	float mRightBlobbyAnimationSpeedAtm;

	PlayerInput mLeftPlayerInput;
	PlayerInput mRightPlayerInput;

	bool mIsGameRunning;
	bool mIsBallValid;

	float mLastHitIntensity;
	float mTimeSinceBallout;
public:
	PhysicWorld();
	~PhysicWorld();
	
	Vector2 getBallVelocity();
	bool getBlobJump(PlayerSide player);
	bool getBallActive();
	float estimateBallImpact();
	Vector2 estimateBallPosition(int steps);

	void setLeftInput(const PlayerInput& input);
	void setRightInput(const PlayerInput& input);

	Vector2 getBlob(int player);
	Vector2 getBall();

	float getBlobState(int player);
	float getBallRotation();

	// These functions tell about ball collisions for game logic and sound 
	bool ballHitLeftPlayer();
	bool ballHitRightPlayer();
	bool ballHitLeftGround();
	bool ballHitRightGround();
	
	bool blobbyHitGround(int player);
	
	// Blobby animation methods
	void blobbyAnimationStep(int player);
	void blobbyStartAnimation(int player);

	// This reports the intensity of the collision
	// which was detected and also queried last.
	float lastHitIntensity();

	// Here the game logic can decide whether the ball is valid.
	// If not, no ball to player collision checking is done, 
	// the input is ignored an the ball experiences a strong damping
	void setBallValidity(bool validity);

	// This returns true if the ball is not valid and the ball is steady
	bool roundFinished();
	
	// This resets everything to the starting situation and
	// wants to know, which player begins.
	void reset(int player);

	// This resets the player to their starting Positions
	void resetPlayer();

	// Important: This assumes a fixed framerate of 60 FPS!
	void step();
	
	// For reducing ball speed after rule violation
	void dampBall();
};

