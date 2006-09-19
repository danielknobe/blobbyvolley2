#pragma once

#include <iostream>

#include "Global.h"
#include "Vector.h"
#include "InputSource.h"


const float BLOBBY_SPEED = 4.5; // BLOBBY_SPEED is necessary to determine the size of the input buffer


class PhysicWorld
{
private:
	inline bool playerTopBallCollision(PlayerSide player);
	inline bool playerBottomBallCollision(PlayerSide player);
	bool resetAreaClear();

	// is ball hit by player?
	bool mBallHitByLeftBlob;
	bool mBallHitByRightBlob;

	Vector2 mBlobPosition[MAX_PLAYERS];
	Vector2 mBallPosition;

	Vector2 mBlobVelocity[MAX_PLAYERS];
	Vector2 mBallVelocity;
	
	float mBallRotation;
	float mBallAngularVelocity;
	float mBlobState[MAX_PLAYERS];
	float mCurrentBlobbyAnimationSpeed[MAX_PLAYERS];

	PlayerInput mPlayerInput[MAX_PLAYERS];

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

	Vector2 getBlob(PlayerSide player);
	Vector2 getBall();

	float getBlobState(PlayerSide player);
	float getBallRotation();

	// These functions tell about ball collisions for game logic and sound 
	bool ballHitLeftPlayer();
	bool ballHitRightPlayer();
	bool ballHitLeftGround();
	bool ballHitRightGround();
	
	bool blobbyHitGround(int player);
	
	// Blobby animation methods
	void blobbyAnimationStep(PlayerSide player);
	void blobbyStartAnimation(PlayerSide player);

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

