#pragma once

#include <iostream>

#include "Vector.h"

struct PlayerInput
{
	PlayerInput()
	{
		left = false;
		right = false;
		up = false;
	}
	
	PlayerInput(bool l, bool r, bool u)
	{
		left = l;
		right = r;
		up = u;
	}

	bool left : 1;
	bool right : 1;
	bool up : 1;
};

class PhysicWorld
{
private:
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
	bool ballHitLeftPlayerTop();
	bool ballHitRightPlayerTop();
	bool ballHitLeftPlayerBottom();
	bool ballHitRightPlayerBottom();
	bool ballHitLeftPlayer();
	bool ballHitRightPlayer();
	bool ballHitLeftGround();
	bool ballHitRightGround();

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
};

