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

#include "Global.h"
#include "Vector.h"
#include "InputSource.h"


const float BLOBBY_SPEED = 4.5; // BLOBBY_SPEED is necessary to determine the size of the input buffer

namespace RakNet
{
	class BitStream;
}

/*! \brief blobby world
	\details This class encapuslates the physical world where blobby happens. It manages the two blobs,
			the ball and collisions between them and the environment, it calculates object movements etc.
	\todo remove all game logic related stuff!
*/
class PhysicWorld
{
	public:
		PhysicWorld();
		~PhysicWorld();

		Vector2 getBallVelocity() const;
		bool getBlobJump(PlayerSide player) const;
		bool getBallActive() const;

		void setLeftInput(const PlayerInput& input);
		void setRightInput(const PlayerInput& input);

		Vector2 getBlob(PlayerSide player) const;
		Vector2 getBall() const;

		float getBlobState(PlayerSide player) const;
		float getBallRotation() const;

		float getBallSpeed() const;

		// These functions tell about ball collisions for game logic and sound
		bool ballHitLeftPlayer() const;
		bool ballHitRightPlayer() const;
		bool ballHitLeftGround() const;
		bool ballHitRightGround() const;

		bool blobbyHitGround(PlayerSide player) const;

		// Blobby animation methods
		void blobbyAnimationStep(PlayerSide player);
		void blobbyStartAnimation(PlayerSide player);

		// This reports the intensity of the collision
		// which was detected and also queried last.
		float lastHitIntensity() const;

		// Here the game logic can decide whether the ball is valid.
		// If not, no ball to player collision checking is done,
		// the input is ignored an the ball experiences a strong damping
		void setBallValidity(bool validity);

		// This returns true if the ball is not valid and the ball is steady
		bool roundFinished() const;

		// This resets everything to the starting situation and
		// wants to know, which player begins.
		void reset(PlayerSide player);

		// This resets the player to their starting positions
		void resetPlayer();

		// Important: This assumes a fixed framerate of 60 FPS!
		void step();

		// For reducing ball speed after rule violation
		void dampBall();

		// Set a new state received from server over a RakNet BitStream
		void setState(RakNet::BitStream* stream);

		// Fill a Bitstream with the state
		void getState(RakNet::BitStream* stream) const;

		// Fill a Bitstream with a side reversed state
		void getSwappedState(RakNet::BitStream* stream) const;

		//Input stuff for recording and playing replays
		const PlayerInput* getPlayersInput() const;
		
		#ifdef DEBUG
		bool checkPhysicStateValidity() const;
		#endif

	private:
		inline bool playerTopBallCollision(int player) const;
		inline bool playerBottomBallCollision(int player) const;
		bool resetAreaClear()const;

		// Do all blobby-related physic stuff which is independent from states
		void handleBlob(PlayerSide player);

		// Detect and handle ball to blobby collisions
		void handleBlobbyBallCollision(PlayerSide player);

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
		///! \todo thats not relevant for physics! It's game logic!!
		float mTimeSinceBallout;
};



