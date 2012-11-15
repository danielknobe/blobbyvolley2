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

/* header include */
#include "PhysicWorld.h"

/* includes */
#include <limits>

#include "raknet/BitStream.h"

#include "GameConstants.h"

/* implementation */
const int TIMESTEP = 5; // calculations per frame

const float TIMEOUT_MAX = 2.5;

// Gamefeeling relevant constants:
const float BLOBBY_ANIMATION_SPEED = 0.5;

const float STANDARD_BALL_ANGULAR_VELOCITY = 0.1;

// helper function for setting FPU precision

inline void set_fpu_single_precision();

PhysicWorld::PhysicWorld()
{
	reset(LEFT_PLAYER);
	mCurrentBlobbyAnimationSpeed[LEFT_PLAYER] = 0.0;
	mCurrentBlobbyAnimationSpeed[RIGHT_PLAYER] = 0.0;
	mTimeSinceBallout = 0.0;
}

PhysicWorld::~PhysicWorld()
{
}

bool PhysicWorld::resetAreaClear() const
{
	return (blobbyHitGround(LEFT_PLAYER) && blobbyHitGround(RIGHT_PLAYER));
}

void PhysicWorld::reset(PlayerSide player)
{
	if (player == LEFT_PLAYER)
		mBallPosition = Vector2(200, STANDARD_BALL_HEIGHT);
	else if (player == RIGHT_PLAYER)
		mBallPosition = Vector2(600, STANDARD_BALL_HEIGHT);
	else
		mBallPosition = Vector2(400, 450);

	mBallVelocity.clear();

	mBallRotation = 0.0;
	mBallAngularVelocity = player == RIGHT_PLAYER ? -STANDARD_BALL_ANGULAR_VELOCITY : STANDARD_BALL_ANGULAR_VELOCITY;
	mBlobState[LEFT_PLAYER] = 0.0;
	mBlobState[RIGHT_PLAYER] = 0.0;

	mIsGameRunning = false;
	mIsBallValid = true;

	mLastHitIntensity = 0.0;
}

void PhysicWorld::resetPlayer()
{
	mBlobPosition[LEFT_PLAYER] = Vector2( 200, GROUND_PLANE_HEIGHT);
	mBlobPosition[RIGHT_PLAYER] = Vector2(600, GROUND_PLANE_HEIGHT);
}

bool PhysicWorld::ballHitRightGround() const
{
	return mBallPosition.y > GROUND_PLANE_HEIGHT && mBallPosition.x > NET_POSITION_X;
}

bool PhysicWorld::ballHitLeftGround() const
{
	return mBallPosition.y > GROUND_PLANE_HEIGHT && mBallPosition.x < NET_POSITION_X;
}

bool PhysicWorld::blobbyHitGround(PlayerSide player) const
{
	if (player == LEFT_PLAYER)
	{
		return (getBlob(LEFT_PLAYER).y >= GROUND_PLANE_HEIGHT);
	}
	else if (player == RIGHT_PLAYER)
	{
		return (getBlob(RIGHT_PLAYER).y >= GROUND_PLANE_HEIGHT);
	}
	else
		return false;
}

void PhysicWorld::setBallValidity(bool validity)
{
	mIsBallValid = validity;
}

bool PhysicWorld::getBallValid() const
{
	return mIsBallValid;
}

bool PhysicWorld::roundFinished() const
{
	if (resetAreaClear())
	{
		if (!mIsBallValid)
		{
			if (mBallVelocity.y < 1.5 && mBallVelocity.y > -1.5 && mBallPosition.y > 430)
				return true;
		}
	}
	if (mTimeSinceBallout > TIMEOUT_MAX)
		return true;
	return false;
}

float PhysicWorld::lastHitIntensity() const
{
	float intensity = mLastHitIntensity / 25.0;
	return intensity < 1.0 ? intensity : 1.0;
}

bool PhysicWorld::playerTopBallCollision(int player) const
{
	if (Vector2(mBallPosition, Vector2(mBlobPosition[player].x,	mBlobPosition[player].y - BLOBBY_UPPER_SPHERE)).length() 
							<= BALL_RADIUS + BLOBBY_UPPER_RADIUS)
		return true;

	return false;
}

inline bool PhysicWorld::playerBottomBallCollision(int player) const
{
	if (Vector2(mBallPosition, Vector2(mBlobPosition[player].x,	mBlobPosition[player].y + BLOBBY_LOWER_SPHERE)).length() 
							<= BALL_RADIUS + BLOBBY_LOWER_RADIUS)
		return true;

	return false;
}

bool PhysicWorld::ballHitLeftPlayer() const
{
	return mBallHitByBlob[LEFT_PLAYER];
}

bool PhysicWorld::ballHitRightPlayer() const
{
	return mBallHitByBlob[RIGHT_PLAYER];
}

bool PhysicWorld::ballHitWall() const
{
	return mBallHitWallSide != NO_PLAYER;
}

PlayerSide PhysicWorld::ballHitWallSide() const
{
	return mBallHitWallSide;
}

bool PhysicWorld::ballHitNet() const
{
	return mBallHitNet;
}

PlayerSide PhysicWorld::ballHitNetSide() const
{
	return mBallHitNetSide;
}

Vector2 PhysicWorld::getBall() const
{
	return mBallPosition;
}

float PhysicWorld::getBallRotation() const
{
	return mBallRotation;
}

float PhysicWorld::getBallSpeed() const
{
	return mBallVelocity.length();
}

Vector2 PhysicWorld::getBlob(PlayerSide player) const
{
	return mBlobPosition[player];
}

Vector2 PhysicWorld::getBlobVelocity(PlayerSide player) const
{
	return mBlobVelocity[player];
}

float PhysicWorld::getBlobState(PlayerSide player) const
{
	return mBlobState[player];
}

void PhysicWorld::setLeftInput(const PlayerInput& input)
{
	mPlayerInput[LEFT_PLAYER] = input;
}

void PhysicWorld::setRightInput(const PlayerInput& input)
{
	mPlayerInput[RIGHT_PLAYER] = input;
}

// Blobby animation methods
void PhysicWorld::blobbyAnimationStep(PlayerSide player)
{
	if (mBlobState[player] < 0.0)
	{
		mCurrentBlobbyAnimationSpeed[player] = 0;
		mBlobState[player] = 0;
	}

	if (mBlobState[player] >= 4.5)
	{
		mCurrentBlobbyAnimationSpeed[player]
			=- BLOBBY_ANIMATION_SPEED;
	}

	mBlobState[player] += mCurrentBlobbyAnimationSpeed[player];

	if (mBlobState[player] >= 5)
	{
		mBlobState[player] = 4.99;
	}
}

void PhysicWorld::blobbyStartAnimation(PlayerSide player)
{
	if (mCurrentBlobbyAnimationSpeed[player] == 0)
		mCurrentBlobbyAnimationSpeed[player] = BLOBBY_ANIMATION_SPEED;
}

void PhysicWorld::handleBlob(PlayerSide player)
{
	// Reset ball to blobby collision
	mBallHitByBlob[player] = false;

	float currentBlobbyGravity = GRAVITATION;

	if (mPlayerInput[player].up)
	{
		if (blobbyHitGround(player))
		{
			mBlobVelocity[player].y = -BLOBBY_JUMP_ACCELERATION;
			blobbyStartAnimation(PlayerSide(player));
		}

		currentBlobbyGravity -= BLOBBY_JUMP_BUFFER;
	}

	if ((mPlayerInput[player].left || mPlayerInput[player].right) && blobbyHitGround(player))
	{
		blobbyStartAnimation(player);
	}

	mBlobVelocity[player].x = (mPlayerInput[player].right ? BLOBBY_SPEED : 0) -
								(mPlayerInput[player].left ? BLOBBY_SPEED : 0);

	// compute blobby fall movement (dt = 1)
	// ds = a/2 * dt^2 + v * dt
	mBlobPosition[player] += Vector2(0, 0.5f * currentBlobbyGravity ) + mBlobVelocity[player];
	// dv = a * dt
	mBlobVelocity[player].y += currentBlobbyGravity;

	// Hitting the ground
	if (mBlobPosition[player].y > GROUND_PLANE_HEIGHT)
	{
		if(mBlobVelocity[player].y > 3.5)
		{
			blobbyStartAnimation(player);
		}

		mBlobPosition[player].y = GROUND_PLANE_HEIGHT;
		mBlobVelocity[player].y = 0.0;
	}

	blobbyAnimationStep(player);
}

void PhysicWorld::handleBlobbyBallCollision(PlayerSide player)
{
	// Check for bottom circles
	if(playerBottomBallCollision(player))
	{
		mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[player]).length();

		const Vector2& blobpos = mBlobPosition[player];
		const Vector2 circlepos = Vector2(blobpos.x, blobpos.y + BLOBBY_LOWER_SPHERE);

		mBallVelocity = -Vector2(mBallPosition, circlepos);
		mBallVelocity = mBallVelocity.normalise();
		mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
		mBallPosition += mBallVelocity;
		mBallHitByBlob[player] = true;
	}
	 else if(playerTopBallCollision(player))
	{
		mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[player]).length();

		const Vector2& blobpos = mBlobPosition[player];
		const Vector2 circlepos = Vector2(blobpos.x, blobpos.y - BLOBBY_UPPER_SPHERE);

		mBallVelocity = -Vector2(mBallPosition, circlepos);
		mBallVelocity = mBallVelocity.normalise();
		mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
		mBallPosition += mBallVelocity;
		mBallHitByBlob[player] = true;
	}

}

void PhysicWorld::step()
{
	// Determistic IEEE 754 floating point computations
	set_fpu_single_precision();

	// Compute independent actions
	handleBlob(LEFT_PLAYER);
	handleBlob(RIGHT_PLAYER);

	// Move ball when game is running
	if (mIsGameRunning)
	{
		// dt = 1 !!
		// move ball ds = a/2 * dt^2 + v * dt
		mBallPosition += Vector2(0, 0.5f * BALL_GRAVITATION) + mBallVelocity;
		// dv = a*dt
		mBallVelocity.y += BALL_GRAVITATION;
	}

	// Collision detection
	if(mIsBallValid)
	{
		handleBlobbyBallCollision(LEFT_PLAYER);
		handleBlobbyBallCollision(RIGHT_PLAYER);
	}
	// Ball to ground Collision
	if (mBallPosition.y + BALL_RADIUS > 500.0)
	{
		mBallVelocity = mBallVelocity.reflectY();
		mBallVelocity = mBallVelocity.scaleX(0.95);
		mBallPosition.y = 500 - BALL_RADIUS;
	}

	if (ballHitLeftPlayer() || ballHitRightPlayer())
		mIsGameRunning = true;

	// Border Collision
	mBallHitWallSide = NO_PLAYER;
	mBallHitNet = false;
	if (mBallPosition.x - BALL_RADIUS <= LEFT_PLANE && mBallVelocity.x < 0.0)
	{
		mBallVelocity = mBallVelocity.reflectX();
		// set the ball's position
		mBallPosition.x = LEFT_PLANE + BALL_RADIUS;
		mBallHitWallSide = LEFT_PLAYER;
	}
	else if (mBallPosition.x + BALL_RADIUS >= RIGHT_PLANE && mBallVelocity.x > 0.0)
	{
		mBallVelocity = mBallVelocity.reflectX();
		// set the ball's position
		mBallPosition.x = RIGHT_PLANE - BALL_RADIUS;
		mBallHitWallSide = RIGHT_PLAYER;
	}
	else if (mBallPosition.y > NET_SPHERE_POSITION &&
			fabs(mBallPosition.x - NET_POSITION_X) < BALL_RADIUS + NET_RADIUS)
	{
		bool right = mBallPosition.x - NET_POSITION_X > 0;
		mBallVelocity = mBallVelocity.reflectX();
		// set the ball's position so that it touches the net
		mBallPosition.x = NET_POSITION_X + (right ? (BALL_RADIUS + NET_RADIUS) : (-BALL_RADIUS - NET_RADIUS));

		mBallHitNet = true;
		mBallHitNetSide = right ? RIGHT_PLAYER : LEFT_PLAYER;
	}
	else
	{
		// Net Collisions

		float ballNetDistance = Vector2(mBallPosition, Vector2(NET_POSITION_X, NET_SPHERE_POSITION)).length();

		if (ballNetDistance < NET_RADIUS + BALL_RADIUS)
		{
			// calculate
			Vector2 normal = Vector2(mBallPosition,	Vector2(NET_POSITION_X, NET_SPHERE_POSITION)).normalise();

			// normal component of kinetic energy
			float perp_ekin = normal.dotProduct(mBallVelocity);
			perp_ekin *= perp_ekin;
			// parallel component of kinetic energy
			float para_ekin = mBallVelocity.length() * mBallVelocity.length() - perp_ekin;

			// the normal component is damped stronger than the parallel component
			// the values are ~ 0.85 and ca. 0.95, because speed is sqrt(ekin)
			perp_ekin *= 0.7;
			para_ekin *= 0.9;

			float nspeed = sqrt(perp_ekin + para_ekin);

			mBallVelocity = Vector2(mBallVelocity.reflect(normal).normalise().scale(nspeed));

			// pushes the ball out of the net
			mBallPosition = (Vector2(NET_POSITION_X, NET_SPHERE_POSITION) - normal * (NET_RADIUS + BALL_RADIUS));

			mBallHitNet = true;
			mBallHitNetSide = NO_PLAYER;
		}
		// mBallVelocity = mBallVelocity.reflect( Vector2( mBallPosition, Vector2 (NET_POSITION_X, temp) ).normalise()).scale(0.75);
	}

	// Collision between blobby and the net
	if (mBlobPosition[LEFT_PLAYER].x+BLOBBY_LOWER_RADIUS>NET_POSITION_X-NET_RADIUS) // Collision with the net
		mBlobPosition[LEFT_PLAYER].x=NET_POSITION_X-NET_RADIUS-BLOBBY_LOWER_RADIUS;

	if (mBlobPosition[RIGHT_PLAYER].x-BLOBBY_LOWER_RADIUS<NET_POSITION_X+NET_RADIUS)
		mBlobPosition[RIGHT_PLAYER].x=NET_POSITION_X+NET_RADIUS+BLOBBY_LOWER_RADIUS;

	// Collision between blobby and the border
	if (mBlobPosition[LEFT_PLAYER].x < LEFT_PLANE)
		mBlobPosition[LEFT_PLAYER].x=LEFT_PLANE;

	if (mBlobPosition[RIGHT_PLAYER].x > RIGHT_PLANE)
		mBlobPosition[RIGHT_PLAYER].x=RIGHT_PLANE;

	// Velocity Integration
	if( !getBallActive() )
		mBallRotation -= mBallAngularVelocity;
	else if (mBallVelocity.x > 0.0)
		mBallRotation += mBallAngularVelocity * (getBallSpeed() / 6);
	else
		mBallRotation -= mBallAngularVelocity * (getBallSpeed() / 6);
	
	// Overflow-Protection
	if (mBallRotation <= 0)
		mBallRotation = 6.25 + mBallRotation;
	else if (mBallRotation >= 6.25)
		mBallRotation = mBallRotation - 6.25;

	mTimeSinceBallout = mIsBallValid ? 0.0 : mTimeSinceBallout + 1.0 / 60;
}

void PhysicWorld::dampBall()
{
	mBallVelocity = mBallVelocity.scale(0.6);
}

Vector2 PhysicWorld::getBallVelocity() const
{
	return mBallVelocity;
}

bool PhysicWorld::getBlobJump(PlayerSide player) const
{
	return !blobbyHitGround(player);
}

bool PhysicWorld::getBallActive() const
{
	return mIsGameRunning;
}

PhysicState PhysicWorld::getState() const
{
	PhysicState st;
	st.blobPosition[LEFT_PLAYER] = mBlobPosition[LEFT_PLAYER];
	st.blobPosition[RIGHT_PLAYER] = mBlobPosition[RIGHT_PLAYER];
	st.blobVelocity[LEFT_PLAYER] = mBlobVelocity[LEFT_PLAYER];
	st.blobVelocity[RIGHT_PLAYER] = mBlobVelocity[RIGHT_PLAYER];

	st.ballPosition = mBallPosition;
	st.ballVelocity = mBallVelocity;
	st.ballAngularVelocity = mBallAngularVelocity;

	st.isGameRunning = mIsGameRunning;
	st.isBallValid = mIsBallValid;

	st.playerInput[LEFT_PLAYER] = mPlayerInput[LEFT_PLAYER];
	st.playerInput[RIGHT_PLAYER] = mPlayerInput[RIGHT_PLAYER];
	return st;
}

void PhysicWorld::setState(const PhysicState& ps)
{
	mBlobPosition[LEFT_PLAYER] = ps.blobPosition[LEFT_PLAYER];
	mBlobPosition[RIGHT_PLAYER] = ps.blobPosition[RIGHT_PLAYER];
	mBlobVelocity[LEFT_PLAYER] = ps.blobVelocity[LEFT_PLAYER];
	mBlobVelocity[RIGHT_PLAYER] = ps.blobVelocity[RIGHT_PLAYER];

	mBallPosition = ps.ballPosition;
	mBallVelocity = ps.ballVelocity;
	mBallAngularVelocity = ps.ballAngularVelocity;

	mIsGameRunning = ps.isGameRunning;
	mIsBallValid = ps.isBallValid;

	mPlayerInput[LEFT_PLAYER] = ps.playerInput[LEFT_PLAYER];
	mPlayerInput[RIGHT_PLAYER] = ps.playerInput[RIGHT_PLAYER];
}

const PlayerInput* PhysicWorld::getPlayersInput() const
{
	return mPlayerInput;
}

// debugging:
#ifdef DEBUG

#include <iostream>

bool PhysicWorld::checkPhysicStateValidity() const
{
	// check for blobby ball collisions
	if(playerTopBallCollision(LEFT_PLAYER) || playerBottomBallCollision(LEFT_PLAYER))
	{
		std::cout << mBallPosition.x << " " << mBallPosition.y << "\n";
		std::cout << mBlobPosition[LEFT_PLAYER].x << " " << mBlobPosition[LEFT_PLAYER].y << "\n";
		return false;
	}

	if(playerTopBallCollision(RIGHT_PLAYER) || playerBottomBallCollision(RIGHT_PLAYER))
	{
		std::cout << mBallPosition.x << " " << mBallPosition.y << "\n";
		std::cout << mBlobPosition[RIGHT_PLAYER].x << " " << mBlobPosition[RIGHT_PLAYER].y << "\n";

		return false;
	}

	return true;
}

#endif


inline void set_fpu_single_precision()
{
#if defined(i386) || defined(__x86_64) // We need to set a precision for diverse x86 hardware
	#if defined(__GNUC__)
		volatile short cw;
		asm volatile ("fstcw %0" : "=m"(cw));
		cw = cw & 0xfcff;
		asm volatile ("fldcw %0" :: "m"(cw));
	#elif defined(_MSC_VER)
		short cw;
		asm fstcw cw;
		cw = cw & 0xfcff;
		asm fldcw cw;
	#endif
#else
	#warning FPU precision may not conform to IEEE 754
#endif
}
