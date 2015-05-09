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
#include "MatchEvents.h"

/* implementation */
// Gamefeeling relevant constants:
const float BLOBBY_ANIMATION_SPEED = 0.5;

// helper function for setting FPU precision
inline short set_fpu_single_precision();
void reset_fpu_flags(short flags);

PhysicWorld::PhysicWorld()
: mBallPosition(Vector2(200, STANDARD_BALL_HEIGHT))
, mBallRotation(0)
, mBallAngularVelocity(STANDARD_BALL_ANGULAR_VELOCITY)
, mLastHitIntensity(0)
{
	mCurrentBlobbyAnimationSpeed[LEFT_PLAYER] = 0.0;
	mCurrentBlobbyAnimationSpeed[RIGHT_PLAYER] = 0.0;

	mBlobState[LEFT_PLAYER] = 0.0;
	mBlobState[RIGHT_PLAYER] = 0.0;

	mBlobPosition[LEFT_PLAYER] = Vector2( 200, GROUND_PLANE_HEIGHT);
	mBlobPosition[RIGHT_PLAYER] = Vector2(600, GROUND_PLANE_HEIGHT);
}

PhysicWorld::~PhysicWorld()
{
}

bool PhysicWorld::blobHitGround(PlayerSide player) const
{
	if (player == LEFT_PLAYER || player == RIGHT_PLAYER)
	{
		return (getBlobPosition(player).y >= GROUND_PLANE_HEIGHT);
	}
	else
		return false;
}

void PhysicWorld::setLastHitIntensity(float intensity)
{
	mLastHitIntensity = intensity;
}

float PhysicWorld::getLastHitIntensity() const
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

float PhysicWorld::getBallRotation() const
{
	return mBallRotation;
}

Vector2 PhysicWorld::getBlobPosition(PlayerSide player) const
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
		mCurrentBlobbyAnimationSpeed[player] = -BLOBBY_ANIMATION_SPEED;
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

void PhysicWorld::handleBlob(PlayerSide player, PlayerInput input)
{
	float currentBlobbyGravity = GRAVITATION;

	if (input.up)
	{
		if (blobHitGround(player))
		{
			mBlobVelocity[player].y = BLOBBY_JUMP_ACCELERATION;
			blobbyStartAnimation( player );
		}

		currentBlobbyGravity -= BLOBBY_JUMP_BUFFER;
	}

	if ((input.left || input.right) && blobHitGround(player))
	{
		blobbyStartAnimation(player);
	}

	mBlobVelocity[player].x = (input.right ? BLOBBY_SPEED : 0) -
								(input.left ? BLOBBY_SPEED : 0);

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

bool PhysicWorld::handleBlobbyBallCollision(PlayerSide player)
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
		return true;
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
		return true;
	}

	return false;
}

int PhysicWorld::step(const PlayerInput& leftInput, const PlayerInput& rightInput, bool isBallValid, bool isGameRunning)
{
	// Determistic IEEE 754 floating point computations
	short fpf = set_fpu_single_precision();

	int events = 0;

	// Compute independent actions
	handleBlob(LEFT_PLAYER, leftInput);
	handleBlob(RIGHT_PLAYER, rightInput);

	// Move ball when game is running
	if (isGameRunning)
	{
		// dt = 1 !!
		// move ball ds = a/2 * dt^2 + v * dt
		mBallPosition += Vector2(0, 0.5f * BALL_GRAVITATION) + mBallVelocity;
		// dv = a*dt
		mBallVelocity.y += BALL_GRAVITATION;
	}

	// Collision detection
	if(isBallValid)
	{
		if (handleBlobbyBallCollision(LEFT_PLAYER))
			events |= EVENT_LEFT_BLOBBY_HIT;
		if (handleBlobbyBallCollision(RIGHT_PLAYER))
			events |= EVENT_RIGHT_BLOBBY_HIT;
	}
	// Ball to ground Collision
	if (mBallPosition.y + BALL_RADIUS > GROUND_PLANE_HEIGHT_MAX)
	{
		mBallVelocity = mBallVelocity.reflectY();
		mBallVelocity = mBallVelocity.scale(0.95);
		mBallPosition.y = GROUND_PLANE_HEIGHT_MAX - BALL_RADIUS;
		events |= mBallPosition.x > NET_POSITION_X ? EVENT_BALL_HIT_RIGHT_GROUND : EVENT_BALL_HIT_LEFT_GROUND;
	}





	// Border Collision
	if (mBallPosition.x - BALL_RADIUS <= LEFT_PLANE && mBallVelocity.x < 0.0)
	{
		mBallVelocity = mBallVelocity.reflectX();
		// set the ball's position
		mBallPosition.x = LEFT_PLANE + BALL_RADIUS;
		events |= EVENT_BALL_HIT_LEFT_WALL;
	}
	else if (mBallPosition.x + BALL_RADIUS >= RIGHT_PLANE && mBallVelocity.x > 0.0)
	{
		mBallVelocity = mBallVelocity.reflectX();
		// set the ball's position
		mBallPosition.x = RIGHT_PLANE - BALL_RADIUS;
		events |= EVENT_BALL_HIT_RIGHT_WALL;
	}
	else if (mBallPosition.y > NET_SPHERE_POSITION &&
			fabs(mBallPosition.x - NET_POSITION_X) < BALL_RADIUS + NET_RADIUS)
	{
		bool right = mBallPosition.x - NET_POSITION_X > 0;
		mBallVelocity = mBallVelocity.reflectX();
		// set the ball's position so that it touches the net
		mBallPosition.x = NET_POSITION_X + (right ? (BALL_RADIUS + NET_RADIUS) : (-BALL_RADIUS - NET_RADIUS));

		events |= right ? EVENT_BALL_HIT_NET_RIGHT : EVENT_BALL_HIT_NET_LEFT;
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

			events |= EVENT_BALL_HIT_NET_TOP;
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
	if( !isGameRunning )
		mBallRotation -= mBallAngularVelocity;
	else if (mBallVelocity.x > 0.0)
		mBallRotation += mBallAngularVelocity * (mBallVelocity.length() / 6);
	else
		mBallRotation -= mBallAngularVelocity * (mBallVelocity.length()/ 6);

	// Overflow-Protection
	if (mBallRotation <= 0)
		mBallRotation = 6.25 + mBallRotation;
	else if (mBallRotation >= 6.25)
		mBallRotation = mBallRotation - 6.25;

	reset_fpu_flags(fpf);

	return events;
}

Vector2 PhysicWorld::getBallPosition() const
{
	return mBallPosition;
}

void PhysicWorld::setBallPosition( Vector2 newPosition )
{
	/// \todo should we check here if this new position is valid, i.e. not inside walls etc.
	mBallPosition = newPosition;
}


Vector2 PhysicWorld::getBallVelocity() const
{
	return mBallVelocity;
}

void PhysicWorld::setBallVelocity( Vector2 newVelocity )
{
	mBallVelocity = newVelocity;
}

void PhysicWorld::setBallAngularVelocity( float angvel )
{
	mBallAngularVelocity = angvel;
}

PhysicState PhysicWorld::getState() const
{
	PhysicState st;
	st.blobPosition[LEFT_PLAYER] = mBlobPosition[LEFT_PLAYER];
	st.blobPosition[RIGHT_PLAYER] = mBlobPosition[RIGHT_PLAYER];
	st.blobVelocity[LEFT_PLAYER] = mBlobVelocity[LEFT_PLAYER];
	st.blobVelocity[RIGHT_PLAYER] = mBlobVelocity[RIGHT_PLAYER];
	st.blobState[LEFT_PLAYER] = mBlobState[LEFT_PLAYER];
	st.blobState[RIGHT_PLAYER] = mBlobState[RIGHT_PLAYER];

	st.ballPosition = mBallPosition;
	st.ballVelocity = mBallVelocity;
	st.ballRotation = mBallRotation;
	st.ballAngularVelocity = mBallAngularVelocity;
	return st;
}

void PhysicWorld::setState(const PhysicState& ps)
{
	mBlobPosition[LEFT_PLAYER] 	= ps.blobPosition[LEFT_PLAYER];
	mBlobPosition[RIGHT_PLAYER] = ps.blobPosition[RIGHT_PLAYER];
	mBlobVelocity[LEFT_PLAYER] 	= ps.blobVelocity[LEFT_PLAYER];
	mBlobVelocity[RIGHT_PLAYER] = ps.blobVelocity[RIGHT_PLAYER];
	mBlobState[LEFT_PLAYER] 	= ps.blobState[LEFT_PLAYER];
	mBlobState[RIGHT_PLAYER] 	= ps.blobState[RIGHT_PLAYER];

	mBallPosition = ps.ballPosition;
	mBallVelocity = ps.ballVelocity;
	mBallRotation = ps.ballRotation;
	mBallAngularVelocity = ps.ballAngularVelocity;
}

inline short set_fpu_single_precision()
{
	short fl = 0;
	#if defined(i386) || defined(__x86_64) // We need to set a precision for diverse x86 hardware
	#if defined(__GNUC__)
		volatile short cw;
		asm volatile ("fstcw %0" : "=m"(cw));
		fl = cw;
		cw = cw & 0xfcff;
		asm volatile ("fldcw %0" :: "m"(cw));
	#elif defined(_MSC_VER)
		short cw;
		asm fstcw cw;
		fl = cw;
		cw = cw & 0xfcff;
		asm fldcw cw;
	#endif
	#else
	#warning FPU precision may not conform to IEEE 754
	#endif
	return fl;
}

void reset_fpu_flags(short flags)
{
	#if defined(i386) || defined(__x86_64) // We need to set a precision for diverse x86 hardware
	#if defined(__GNUC__)
		asm volatile ("fldcw %0" :: "m"(flags));
	#elif defined(_MSC_VER)
		asm fldcw flags;
	#endif
	#else
	#warning FPU precision may not conform to IEEE 754
	#endif
}
