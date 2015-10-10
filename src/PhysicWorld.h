/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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
#include "PlayerInput.h"
#include "BlobbyDebug.h"
#include "PhysicState.h"
#include "MatchEvents.h"
#include <functional>	// for std::function used for the callback

/*! \brief blobby world
	\details This class encapuslates the physical world where blobby happens. It manages the two blobs,
			the ball and collisions between them and the environment, it calculates object movements etc.
*/
class PhysicWorld : public ObjectCounter<PhysicWorld>
{
	// callback function type
	typedef std::function<void(MatchEvent)> event_callback_fn;

	public:
		PhysicWorld();
		~PhysicWorld();

		// set the callback
		void setEventCallback( event_callback_fn );

		// ball information queries
		Vector2 getBallPosition() const;
		void setBallPosition( Vector2 newPosition );
		Vector2 getBallVelocity() const;
		void setBallVelocity( Vector2 newVelocity );
		float getBallRotation() const;
		void setBallAngularVelocity( float angvel );

		// blobby information queries
		Vector2 getBlobPosition(PlayerSide player) const;
		Vector2 getBlobVelocity(PlayerSide player) const;
		float getBlobState(PlayerSide player) const;
		// this function calculates whether the player is on the ground
		bool blobHitGround(PlayerSide player) const;

		// Important: This assumes a fixed framerate of 60 FPS!
		void step(const PlayerInput& leftInput, const PlayerInput& rightInput,
					bool isBallValid, bool isGameRunning);

		// gets the physic state
		PhysicState getState() const;

		// sets a new physic state
		void setState(const PhysicState& state);

		// helper functions
		/// this function calculates whether two circles overlap.
		static bool circleCircleCollision(const Vector2& pos1, float rad_1, const Vector2& pos2, float rad_2);

	private:
		// Blobby animation methods
		void blobbyStartAnimation(PlayerSide player);
		void blobbyAnimationStep(PlayerSide player);

		inline bool playerTopBallCollision(int player) const;
		inline bool playerBottomBallCollision(int player) const;

		// Do all blobby-related physic stuff which is independent from states
		void handleBlob(PlayerSide player, PlayerInput input);

		// Detect and handle ball to blobby collisions
		bool handleBlobbyBallCollision(PlayerSide player);
		// calculate ball impacts vs wall, ground and net
		void handleBallWorldCollisions();

		Vector2 mBlobPosition[MAX_PLAYERS];
		Vector2 mBallPosition;

		Vector2 mBlobVelocity[MAX_PLAYERS];
		Vector2 mBallVelocity;

		float mBallRotation;
		float mBallAngularVelocity;
		float mBlobState[MAX_PLAYERS];
		float mCurrentBlobbyAnimationSpeed[MAX_PLAYERS];

		float mLastHitIntensity;

		event_callback_fn mCallback;
};



