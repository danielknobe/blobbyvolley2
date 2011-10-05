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

#include "PhysicWorld.h"
#include "GameConstants.h"
#include "raknet/BitStream.h"

#include <limits>

const int TIMESTEP = 5; // calculations per frame

const float TIMEOUT_MAX = 2.5;

// Gamefeeling relevant constants:
const float BLOBBY_ANIMATION_SPEED = 0.5;

const float STANDARD_BALL_ANGULAR_VELOCITY = 0.1;
const float STANDARD_BALL_HEIGHT = 269 + BALL_RADIUS;

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
	if (blobbyHitGround(LEFT_PLAYER) && blobbyHitGround(RIGHT_PLAYER))
		return true;
	return false;
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
	mBallAngularVelocity = STANDARD_BALL_ANGULAR_VELOCITY;
	mBlobState[LEFT_PLAYER] = 0.0;
	mBlobState[RIGHT_PLAYER] = 0.0;

	mIsGameRunning = false;
	mIsBallValid = true;

	mLastHitIntensity = 0.0;
}

void PhysicWorld::resetPlayer()
{
	mBlobPosition[LEFT_PLAYER] = Vector2( 200,
		GROUND_PLANE_HEIGHT);
	mBlobPosition[RIGHT_PLAYER] = Vector2(600,
		GROUND_PLANE_HEIGHT);
}

bool PhysicWorld::ballHitRightGround() const
{
	if (mIsBallValid)
		if (mBallPosition.y > GROUND_PLANE_HEIGHT &&
			mBallPosition.x > NET_POSITION_X)
			return true;
	return false;
}

bool PhysicWorld::ballHitLeftGround() const
{
	if (mIsBallValid)
		if (mBallPosition.y > GROUND_PLANE_HEIGHT &&
			mBallPosition.x < NET_POSITION_X)
			return true;
	return false;
}

bool PhysicWorld::blobbyHitGround(PlayerSide player) const
{
	if (player == LEFT_PLAYER)
	{
		if (getBlob(LEFT_PLAYER).y >= GROUND_PLANE_HEIGHT)
			return true;
		else
			return false;
	}
	else if (player == RIGHT_PLAYER)
	{
		if (getBlob(RIGHT_PLAYER).y >= GROUND_PLANE_HEIGHT)
			return true;
		else
			return false;
	}
	else
		return false;
}

void PhysicWorld::setBallValidity(bool validity)
{
	mIsBallValid = validity;
}

bool PhysicWorld::roundFinished() const
{
	if (resetAreaClear())
	{
		if (!mIsBallValid)
			if (mBallVelocity.y < 1.5 &&
				mBallVelocity.y > -1.5 && mBallPosition.y > 430)
				return true;
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
	if (Vector2(mBallPosition,
		Vector2(mBlobPosition[player].x,
			mBlobPosition[player].y - BLOBBY_UPPER_SPHERE)
			).length() <= BALL_RADIUS + BLOBBY_UPPER_RADIUS)
		return true;
	return false;
}

inline bool PhysicWorld::playerBottomBallCollision(int player) const
{
	if (Vector2(mBallPosition,
		Vector2(mBlobPosition[player].x,
			mBlobPosition[player].y + BLOBBY_LOWER_SPHERE)
			).length() <= BALL_RADIUS + BLOBBY_LOWER_RADIUS)
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
		mCurrentBlobbyAnimationSpeed[player] =
			BLOBBY_ANIMATION_SPEED;
}

void PhysicWorld::handleBlob(PlayerSide player)
{
	// Reset ball to blobby collision
	mBallHitByBlob[player] = false;

	if (mPlayerInput[player].up)
	{
		if (blobbyHitGround(player))
		{
			mBlobVelocity[player].y = -BLOBBY_JUMP_ACCELERATION;
			blobbyStartAnimation(PlayerSide(player));
		}
		mBlobVelocity[player].y -= BLOBBY_JUMP_BUFFER;
	}

	if ((mPlayerInput[player].left || mPlayerInput[player].right)
			&& blobbyHitGround(player))
	{
		blobbyStartAnimation(player);
	}

	mBlobVelocity[player].x =
		(mPlayerInput[player].right ? BLOBBY_SPEED : 0) -
		(mPlayerInput[player].left ? BLOBBY_SPEED : 0);

	// Acceleration Integration
	mBlobVelocity[player].y += GRAVITATION;

	// Compute new position
	mBlobPosition[player] += mBlobVelocity[player];

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

void PhysicWorld::checkBlobbyBallCollision(PlayerSide player)
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

	// Ball Gravitation
	if (mIsGameRunning)
		mBallVelocity.y += BALL_GRAVITATION;

	// move ball
	mBallPosition += mBallVelocity;

	// Collision detection
	if(mIsBallValid)
	{
		checkBlobbyBallCollision(LEFT_PLAYER);
		checkBlobbyBallCollision(RIGHT_PLAYER);
	}
	// Ball to ground Collision
	else if (mBallPosition.y + BALL_RADIUS > 500.0)
	{
		mBallVelocity = mBallVelocity.reflectY().scaleY(0.5);
		mBallVelocity = mBallVelocity.scaleX(0.55);
		mBallPosition.y = 500 - BALL_RADIUS;
	}

	if (ballHitLeftPlayer() || ballHitRightPlayer())
		mIsGameRunning = true;
	
	// Border Collision
	if (mBallPosition.x - BALL_RADIUS <= LEFT_PLANE && mBallVelocity.x < 0.0)
	{
		mBallVelocity = mBallVelocity.reflectX();
		// set the ball's position
		mBallPosition.x = LEFT_PLANE + BALL_RADIUS;
	}
	else if (mBallPosition.x + BALL_RADIUS >= RIGHT_PLANE && mBallVelocity.x > 0.0)
	{
		mBallVelocity = mBallVelocity.reflectX();
		// set the ball's position
		mBallPosition.x = RIGHT_PLANE - BALL_RADIUS;
	}
	else if (mBallPosition.y > NET_SPHERE_POSITION &&
			fabs(mBallPosition.x - NET_POSITION_X) < BALL_RADIUS + NET_RADIUS)
	{
		mBallVelocity = mBallVelocity.reflectX();
		// set the ball's position so that it touches the net
		mBallPosition.x = NET_POSITION_X + ((mBallPosition.x - NET_POSITION_X > 0) ?( BALL_RADIUS + NET_RADIUS) : (- BALL_RADIUS - NET_RADIUS));
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
			// the values are ~ 0.85² and ca. 0.95², because speed is sqrt(ekin)
			perp_ekin *= 0.7;
			para_ekin *= 0.9;
			
			float nspeed = sqrt(perp_ekin + para_ekin);
			
			mBallVelocity = Vector2(mBallVelocity.reflect(normal).normalise().scale(nspeed));
			
			// pushes the ball out of the net
			mBallPosition = (Vector2(NET_POSITION_X, NET_SPHERE_POSITION) - normal * (NET_RADIUS + BALL_RADIUS));
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
	if (mBallVelocity.x > 0.0)
		mBallRotation += mBallAngularVelocity * (getBallSpeed() / 6);
	else if (mBallVelocity.x < 0.0)
		mBallRotation -= mBallAngularVelocity * (getBallSpeed() / 6);
	else
		mBallRotation -= mBallAngularVelocity;

	// Overflow-Protection
	if (mBallRotation <= 0)
		mBallRotation = 6.25 + mBallRotation;
	else if (mBallRotation >= 6.25)
		mBallRotation = mBallRotation - 6.25;

	mTimeSinceBallout = mIsBallValid ? 0.0 :
		mTimeSinceBallout + 1.0 / 60;
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

void writeCompressedToBitStream(RakNet::BitStream* stream, float value, float min, float max)
{
	assert(min <= value && value <= max);
	assert(stream);
	unsigned short only2bytes = static_cast<unsigned short>((value - min) / (max - min) * std::numeric_limits<unsigned short>::max());
	stream->Write(only2bytes);
}

void readCompressedFromBitStream(RakNet::BitStream* stream, float& value, float min, float max)
{
	unsigned short only2bytes;
	stream->Read(only2bytes);
	value = static_cast<float>(only2bytes) / static_cast<float>(std::numeric_limits<unsigned short>::max()) * (max - min) + min;
}


void PhysicWorld::setState(RakNet::BitStream* stream)
{
	bool leftGround;
	bool rightGround;
	stream->Read(leftGround);
	stream->Read(rightGround);
	if(leftGround){
		mBlobPosition[LEFT_PLAYER].y = GROUND_PLANE_HEIGHT;
		mBlobVelocity[LEFT_PLAYER].y = 0;
	}else{
		readCompressedFromBitStream(stream, mBlobPosition[LEFT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		readCompressedFromBitStream(stream, mBlobVelocity[LEFT_PLAYER].y, -30, 30);
	}
	
	if(rightGround){
		mBlobPosition[RIGHT_PLAYER].y = GROUND_PLANE_HEIGHT;
		mBlobVelocity[RIGHT_PLAYER].y = 0;
	}else{
		readCompressedFromBitStream(stream, mBlobPosition[RIGHT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		readCompressedFromBitStream(stream, mBlobVelocity[RIGHT_PLAYER].y, -30, 30);
	}
	
	readCompressedFromBitStream(stream, mBlobPosition[LEFT_PLAYER].x, LEFT_PLANE, NET_POSITION_X);
	readCompressedFromBitStream(stream, mBlobPosition[RIGHT_PLAYER].x, NET_POSITION_X, RIGHT_PLANE);
	
	readCompressedFromBitStream(stream, mBallPosition.x, LEFT_PLANE, RIGHT_PLANE);
	// maybe these values is a bit too pessimistic...
	// but we have 65535 values, hence it should be precise enough
	readCompressedFromBitStream(stream, mBallPosition.y, -500, GROUND_PLANE_HEIGHT_MAX);

	readCompressedFromBitStream(stream, mBallVelocity.x, -30, 30);
	readCompressedFromBitStream(stream, mBallVelocity.y, -30, 30);

	stream->Read(mPlayerInput[LEFT_PLAYER].left);
	stream->Read(mPlayerInput[LEFT_PLAYER].right);
	stream->Read(mPlayerInput[LEFT_PLAYER].up);
	stream->Read(mPlayerInput[RIGHT_PLAYER].left);
	stream->Read(mPlayerInput[RIGHT_PLAYER].right);
	stream->Read(mPlayerInput[RIGHT_PLAYER].up);
}

void PhysicWorld::getState(RakNet::BitStream* stream) const
{
	// if the blobbys are standing on the ground, we need not send
	// y position and velocity
	stream->Write(blobbyHitGround(LEFT_PLAYER));
	stream->Write(blobbyHitGround(RIGHT_PLAYER));
	
	if(!blobbyHitGround(LEFT_PLAYER)){
		writeCompressedToBitStream(stream, mBlobPosition[LEFT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, mBlobVelocity[LEFT_PLAYER].y, -30, 30);
	}
	
	if(!blobbyHitGround(RIGHT_PLAYER)){
		writeCompressedToBitStream(stream, mBlobPosition[RIGHT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, mBlobVelocity[RIGHT_PLAYER].y, -30, 30);
	}
	
	writeCompressedToBitStream(stream, mBlobPosition[LEFT_PLAYER].x, LEFT_PLANE, NET_POSITION_X);
	writeCompressedToBitStream(stream, mBlobPosition[RIGHT_PLAYER].x, NET_POSITION_X, RIGHT_PLANE);
	
	writeCompressedToBitStream(stream, mBallPosition.x, LEFT_PLANE, RIGHT_PLANE);
	writeCompressedToBitStream(stream, mBallPosition.y, -500, GROUND_PLANE_HEIGHT_MAX);

	writeCompressedToBitStream(stream, mBallVelocity.x, -30, 30);
	writeCompressedToBitStream(stream, mBallVelocity.y, -30, 30);

	stream->Write(mPlayerInput[LEFT_PLAYER].left);
	stream->Write(mPlayerInput[LEFT_PLAYER].right);
	stream->Write(mPlayerInput[LEFT_PLAYER].up);
	stream->Write(mPlayerInput[RIGHT_PLAYER].left);
	stream->Write(mPlayerInput[RIGHT_PLAYER].right);
	stream->Write(mPlayerInput[RIGHT_PLAYER].up);

}

void PhysicWorld::getSwappedState(RakNet::BitStream* stream) const
{
	// if the blobbys are standing on the ground, we need not send
	// y position and velocity
	stream->Write(blobbyHitGround(RIGHT_PLAYER));
	stream->Write(blobbyHitGround(LEFT_PLAYER));
	
	if(!blobbyHitGround(LEFT_PLAYER)){
		writeCompressedToBitStream(stream, mBlobPosition[LEFT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, mBlobVelocity[LEFT_PLAYER].y, -30, 30);
	}
	
	if(!blobbyHitGround(RIGHT_PLAYER)){
		writeCompressedToBitStream(stream, mBlobPosition[RIGHT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, mBlobVelocity[RIGHT_PLAYER].y, -30, 30);
	}
	
	writeCompressedToBitStream(stream, 800 - mBlobPosition[RIGHT_PLAYER].x, LEFT_PLANE, NET_POSITION_X);
	writeCompressedToBitStream(stream, 800 - mBlobPosition[LEFT_PLAYER].x, NET_POSITION_X, RIGHT_PLANE);
	
	writeCompressedToBitStream(stream, 800 - mBallPosition.x, LEFT_PLANE, RIGHT_PLANE);
	writeCompressedToBitStream(stream, mBallPosition.y, -500, GROUND_PLANE_HEIGHT_MAX);

	writeCompressedToBitStream(stream, -mBallVelocity.x, -30, 30);
	writeCompressedToBitStream(stream, mBallVelocity.y, -30, 30);

	stream->Write(mPlayerInput[RIGHT_PLAYER].right);
	stream->Write(mPlayerInput[RIGHT_PLAYER].left);
	stream->Write(mPlayerInput[RIGHT_PLAYER].up);
	stream->Write(mPlayerInput[LEFT_PLAYER].right);
	stream->Write(mPlayerInput[LEFT_PLAYER].left);
	stream->Write(mPlayerInput[LEFT_PLAYER].up);
}

const PlayerInput* PhysicWorld::getPlayersInput() const
{
	return mPlayerInput;
}
