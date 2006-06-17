#include "PhysicWorld.h"

const short PHYSIC_STEPS = 3; // calculations per fps

// Blobby Settings
const float BLOBBY_HEIGHT = 89;
const float BLOBBY_WIDTH = 75;
const float BLOBBY_UPPER_SPHERE = 19;
const float BLOBBY_UPPER_RADIUS = 25;
const float BLOBBY_LOWER_SPHERE = 13;
const float BLOBBY_LOWER_RADIUS = 33;

// Volley Ball Net
const float NET_POSITION_X = 400;
const float NET_POSITION_Y = 438;
const float NET_RADIUS = 7;
const float NET_SPHERE = 154; 

// Ball Settings
const float BALL_RADIUS = 31.5;



const float GROUND_PLANE_HEIGHT = 500 - BLOBBY_HEIGHT / 2.0;



// Boarder Settings
const float LEFT_PLANE = 0; 
const float RIGHT_PLANE = 800.0;
// These numbers should include the blobbys width, but in the original game
// the blobbys can go a bit into the walls too.


// Gamefeeling relevant constants:

const float BLOBBY_SPEED = 8.5;
const float BLOBBY_JUMP_ACCELERATION = 14;
const float BLOBBY_JUMP_BUFFER = 0.42;
const float GRAVITATION = 0.8;
const float BALL_GRAVITATION = 0.22;
const float STANDARD_BALL_ANGULAR_VELOCITY = 0.1;
const float STANDARD_BALL_HEIGHT = 269 + BALL_RADIUS;

// Temp
float temp;

PhysicWorld::PhysicWorld()
{
	reset(0);
}

PhysicWorld::~PhysicWorld()
{
}

void PhysicWorld::reset(int player)
{

	mLeftBlobPosition = Vector2( 200, 
			GROUND_PLANE_HEIGHT + BLOBBY_HEIGHT / 2.0);
	mRightBlobPosition = Vector2(600,
			GROUND_PLANE_HEIGHT + BLOBBY_HEIGHT / 2.0);
	if (player == 0)
		mBallPosition = Vector2(200, STANDARD_BALL_HEIGHT);
	else if (player == 1)
		mBallPosition = Vector2(600, STANDARD_BALL_HEIGHT);
	else 
		mBallPosition = Vector2(400, 450);
	
	
	mLeftBlobVelocity.clear();
	mRightBlobVelocity.clear();
	mBallVelocity.clear();

	mBallRotation = 0.0;
	mBallAngularVelocity = STANDARD_BALL_ANGULAR_VELOCITY;
	mLeftBlobState = 0.0;
	mRightBlobState = 0.0;

	mIsGameRunning = false;
	mIsBallValid = true;

	mLastHitIntensity = 0.0;
}

bool PhysicWorld::ballHitRightGround()
{
	if (mIsBallValid)
		if (mBallPosition.y > GROUND_PLANE_HEIGHT && 
			mBallPosition.x > NET_POSITION_X)
			return true;
	return false;
}

bool PhysicWorld::ballHitLeftGround()
{
	if (mIsBallValid)
		if (mBallPosition.y > GROUND_PLANE_HEIGHT && 
			mBallPosition.x < NET_POSITION_X)
			return true;
	return false;
}

void PhysicWorld::setBallValidity(bool validity)
{
	mIsBallValid = validity;
}

bool PhysicWorld::roundFinished()
{
	if (!mIsBallValid)
		if (mBallVelocity.length() < 2.0 && mBallPosition.y > GROUND_PLANE_HEIGHT)
			return true;
	return false;
}

float PhysicWorld::lastHitIntensity()
{
	float intensity = mLastHitIntensity / 14.0;
	return intensity < 1.0 ? intensity : 1.0;
}

// internel collision-methods
bool PhysicWorld::int_BallHitLeftPlayerTop()
{
	if (Vector2(
	mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y-BLOBBY_UPPER_SPHERE)
	).length() <= BALL_RADIUS + BLOBBY_UPPER_RADIUS)
	return true;
	
	return false;
}

bool PhysicWorld::int_BallHitRightPlayerTop()
{
	if (Vector2(
	mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y-BLOBBY_UPPER_SPHERE)
	).length() <= BALL_RADIUS + BLOBBY_UPPER_RADIUS)
	return true;
    
    return false;
}

bool PhysicWorld::int_BallHitLeftPlayerBottom()
{
	if (Vector2(
	mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y+BLOBBY_LOWER_SPHERE)
	).length() <= BALL_RADIUS + BLOBBY_LOWER_RADIUS)
	return true;
	
	return false;
} 

bool PhysicWorld::int_BallHitRightPlayerBottom()
{
	if (Vector2(
	mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y+BLOBBY_LOWER_SPHERE)
	).length() <= BALL_RADIUS + BLOBBY_LOWER_RADIUS)
	return true;
	
	return false;
}   
     
bool PhysicWorld::int_BallHitLeftPlayer()
{
	if (int_BallHitLeftPlayerTop() || int_BallHitLeftPlayerBottom())
	return true;
	
	return false;
} 

bool PhysicWorld::int_BallHitRightPlayer()
{
	if (int_BallHitRightPlayerTop() || int_BallHitRightPlayerBottom())
	return true;
	
	return false;
}   

// externel collision-methods
bool PhysicWorld::ballHitLeftPlayer()
{
	return mBallHitByLeftBlob;
} 

bool PhysicWorld::ballHitRightPlayer()
{
	return mBallHitByRightBlob;
}   

Vector2 PhysicWorld::getBall()
{
	return mBallPosition;
}

float PhysicWorld::getBallRotation()
{
	return mBallRotation;
}

Vector2 PhysicWorld::getLeftBlob()
{
	return mLeftBlobPosition;
}

Vector2 PhysicWorld::getRightBlob()
{
	return mRightBlobPosition;
}

float PhysicWorld::getLeftBlobState()
{
	return mLeftBlobState;
}

float PhysicWorld::getRightBlobState()
{
	return mRightBlobState;
}

void PhysicWorld::setLeftInput(const PlayerInput& input)
{
	mLeftPlayerInput = input;
}

void PhysicWorld::setRightInput(const PlayerInput& input)
{
	mRightPlayerInput = input;
}

void PhysicWorld::step()
{
    // Input Handling
	if (mLeftBlobPosition.y >= GROUND_PLANE_HEIGHT)
		if (mLeftPlayerInput.up)
			mLeftBlobVelocity.y = -BLOBBY_JUMP_ACCELERATION;
		
	if (mLeftPlayerInput.up)
		mLeftBlobVelocity.y -= BLOBBY_JUMP_BUFFER;
		
	if (mRightBlobPosition.y >= GROUND_PLANE_HEIGHT)
		if (mRightPlayerInput.up)
			mRightBlobVelocity.y = -BLOBBY_JUMP_ACCELERATION;
			
	if (mRightPlayerInput.up)
		mRightBlobVelocity.y -= BLOBBY_JUMP_BUFFER;
    
	// Reset the ball-blobby collision
	mBallHitByRightBlob=false;
	mBallHitByLeftBlob=false;
    
    for (short counter = 1; counter <= PHYSIC_STEPS; counter++)
	{
		// This is only influenced by input, so we need to reset this here
		mLeftBlobVelocity.x = 0.0;
		mRightBlobVelocity.x = 0.0;

		if (mIsBallValid)
		{
			if (mLeftPlayerInput.left)
				if (mLeftBlobPosition.x > LEFT_PLANE)
					mLeftBlobVelocity.x = -BLOBBY_SPEED;
			
			if (mLeftBlobPosition.x+BLOBBY_LOWER_RADIUS<NET_POSITION_X-NET_RADIUS) // Collision with the net
				if (mLeftPlayerInput.right)
					if (mLeftBlobPosition.x < RIGHT_PLANE)
						mLeftBlobVelocity.x = +BLOBBY_SPEED;

			if (mRightBlobPosition.x-BLOBBY_LOWER_RADIUS>NET_POSITION_X+NET_RADIUS)	// Collision with the net			
				if (mRightPlayerInput.left)
					if (mRightBlobPosition.x > LEFT_PLANE)
						mRightBlobVelocity.x = -BLOBBY_SPEED;
				
			if (mRightPlayerInput.right)
				if (mRightBlobPosition.x < RIGHT_PLANE)
					mRightBlobVelocity.x = +BLOBBY_SPEED;
		}	
     
		// Collision Detection

		if(mIsBallValid)
		{
			if(int_BallHitLeftPlayerTop())
			{
				mBallVelocity = Vector2(mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y-BLOBBY_UPPER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition -= mBallVelocity.scale(3);
				mBallVelocity = mBallVelocity.scale(11);
				mBallHitByLeftBlob = true;
				mLastHitIntensity = Vector2(mBallVelocity, mLeftBlobVelocity).length();
			}

			else if(int_BallHitRightPlayerTop())
			{
				mBallVelocity = Vector2(mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y-BLOBBY_UPPER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition -= mBallVelocity.scale(3);
				mBallVelocity = mBallVelocity.scale(11);
				mBallHitByRightBlob=true;
				mLastHitIntensity = Vector2(mBallVelocity, mLeftBlobVelocity).length();
			}
		
			else if(int_BallHitLeftPlayerBottom())
			{
				mBallVelocity = Vector2(mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y+BLOBBY_LOWER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition -= mBallVelocity.scale(3);
				mBallVelocity = mBallVelocity.scale(11);
				mBallHitByLeftBlob=true;
				mLastHitIntensity = Vector2(mBallVelocity, mRightBlobVelocity).length();
			}

			else if(int_BallHitRightPlayerBottom())
			{
				mBallVelocity = Vector2(mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y+BLOBBY_LOWER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition -= mBallVelocity.scale(3);
				mBallVelocity = mBallVelocity.scale(11);
				mBallHitByRightBlob=true;
				mLastHitIntensity = Vector2(mBallVelocity, mRightBlobVelocity).length();
			}	
		}
		// Ball to ground Collision
		else
		{
			if (mBallPosition.y + BALL_RADIUS > 500.0)
			{
				mBallVelocity = mBallVelocity.reflectY().scale(0.6);
				mBallPosition -= mBallVelocity;
			}
		}

		// Border Collision
		if(mBallPosition.x-BALL_RADIUS<=LEFT_PLANE && mBallVelocity.x>0)
			mBallVelocity = mBallVelocity.reflectX();
		if(mBallPosition.x+BALL_RADIUS>=RIGHT_PLANE && mBallVelocity.x<0)
			mBallVelocity = mBallVelocity.reflectX();
		    	
		// Net Collision
		// Left Net Border
		if(mBallPosition.x+BALL_RADIUS>=NET_POSITION_X-NET_RADIUS
			&& mBallPosition.x+BALL_RADIUS<=NET_POSITION_X+NET_RADIUS+10
	    	&& mBallVelocity.x<0
	    	&& mBallPosition.y > NET_POSITION_Y-NET_SPHERE)
				mBallVelocity = mBallVelocity.reflectX();

		// Right Net Border  
		else if(mBallPosition.x-BALL_RADIUS<=NET_POSITION_X+NET_RADIUS
			&& mBallPosition.x-BALL_RADIUS>=NET_POSITION_X-NET_RADIUS-10
		    && mBallVelocity.x>0
		    && mBallPosition.y > NET_POSITION_Y-NET_SPHERE)
				mBallVelocity = mBallVelocity.reflectX();
		// Net Sphere
		else if (Vector2(
			mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE)
			).length() <= NET_RADIUS + BALL_RADIUS - 1)
		{
			Vector2 reflectionNormal = 
				Vector2(mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE))
				.normalise();
			mBallVelocity = mBallVelocity.reflect(reflectionNormal);
		}

		
		// Velocity Integration
		if(mBallVelocity.x<0)
		mBallRotation += mBallAngularVelocity/PHYSIC_STEPS;
		else
		mBallRotation -= mBallAngularVelocity/PHYSIC_STEPS;
		
		//Overflow-Protection
		if(mBallRotation <= 0.0)
			mBallRotation += 6.25;
		if(mBallRotation >= 6.25)
			mBallRotation -= 6.25;

		mLeftBlobPosition += mLeftBlobVelocity/PHYSIC_STEPS;
		mRightBlobPosition += mRightBlobVelocity/PHYSIC_STEPS;

		// Acceleration Integration
	
		mLeftBlobVelocity.y -= -GRAVITATION/PHYSIC_STEPS;
		mRightBlobVelocity.y -= -GRAVITATION/PHYSIC_STEPS;

		// Ball Gravitation
		if (mIsGameRunning)
			mBallVelocity.y -= BALL_GRAVITATION/PHYSIC_STEPS;
		else if (ballHitLeftPlayer() || ballHitRightPlayer())
			mIsGameRunning = true;
		
		mBallPosition -= mBallVelocity/PHYSIC_STEPS;


	
		if (mLeftBlobPosition.y > GROUND_PLANE_HEIGHT)
		{
			mLeftBlobPosition.y = GROUND_PLANE_HEIGHT;
			mLeftBlobVelocity.y = 0.0;
			// We need an error correction here because the y coordinate
			// is computed with a physical simulation of the gravitation.
		}
		if (mRightBlobPosition.y > GROUND_PLANE_HEIGHT)
		{
			mRightBlobPosition.y = GROUND_PLANE_HEIGHT;
			mRightBlobVelocity.y = 0.0;
		}
	
	} // Ende der Schleift
}

void PhysicWorld::dampBall()
{
	mBallVelocity = mBallVelocity.scale(0.5);
}

