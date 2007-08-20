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

#pragma once

#include <iostream>

#include "Global.h"
#include "Vector.h"
#include "InputSource.h"


const float BLOBBY_SPEED = 4.5; // BLOBBY_SPEED is necessary to determine the size of the input buffer

namespace RakNet
{
	class BitStream;
}

class PhysicWorld
{
private:
	inline bool playerTopBallCollision(int player);
	inline bool playerBottomBallCollision(int player);
	bool resetAreaClear();

	bool mBallHitByBlob[MAX_PLAYERS];

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

	float getBallSpeed();

	// These functions tell about ball collisions for game logic and sound
	bool ballHitLeftPlayer();
	bool ballHitRightPlayer();
	bool ballHitLeftGround();
	bool ballHitRightGround();

	bool blobbyHitGround(int player);

	// Blobby animation methods
	void blobbyAnimationStep(PlayerSide player);
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

	// Set a new state recieved from server over a RakNet BitStream
	void setState(RakNet::BitStream* stream);

	// Fill a Bitstream with the state
	void getState(RakNet::BitStream* stream);

	// Fill a Bitstream with a side reversed state
	void getSwappedState(RakNet::BitStream* stream);

	//Input stuff for recording and playing replays
	const PlayerInput* getPlayersInput();
};



