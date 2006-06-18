#pragma once

#include <iostream>

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

	// ball is hit by player?
	bool mBallHitByLeftBlob;
	bool mBallHitByRightBlob;

	Vector2 mLeftBlobPosition;
	Vector2 mRightBlobPosition;
	Vector2 mBallPosition;

	Vector2 mLeftBlobVelocity;
	Vector2 mRightBlobVelocity;
	Vector2 mBallVelocity;

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
public:
	PhysicWorld();
	~PhysicWorld();

	void setLeftInput(const PlayerInput& input);
	void setRightInput(const PlayerInput& input);

	Vector2 getLeftBlob();
	Vector2 getRightBlob();
	Vector2 getBall();

	float getLeftBlobState();
	float getRightBlobState();
	float getBallRotation();

	// These functions tell about ball collisions for game logic and sound 
	bool ballHitLeftPlayer();
	bool ballHitRightPlayer();
	bool ballHitLeftGround();
	bool ballHitRightGround();
	
	// Blobby animation methods
	void leftBlobbyAnimationStep();
	void leftBlobbyStartAnimation();
	void rightBlobbyAnimationStep();
	void rightBlobbyStartAnimation();

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

	// Important: This assumes a fixed framerate of 60 FPS!
	void step();
	
	// For reducing ball speed after rule violation
	void dampBall();
};

