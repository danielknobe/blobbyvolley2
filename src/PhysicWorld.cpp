#include "PhysicWorld.h"

const float TIMESTEP = 1.0 / 60.0;

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

const float BLOBBY_SPEED = 480;
const float BLOBBY_JUMP_ACCELERATION = 850;
const float BLOBBY_JUMP_BUFFER = 25;
const float GRAVITATION = 3000;
const float BALL_GRAVITATION = 11.4;
const float STANDARD_BALL_ANGULAR_VELOCITY = 7.0;
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
	if (mBallPosition.y > GROUND_PLANE_HEIGHT && 
			mBallPosition.x > NET_POSITION_X)
		return true;
	else return false;
}

bool PhysicWorld::ballHitLeftGround()
{
	if (mBallPosition.y > GROUND_PLANE_HEIGHT && 
			mBallPosition.x < NET_POSITION_X)
		return true;
	else return false;
}

void PhysicWorld::setBallValidity(bool validity)
{
	mIsBallValid = validity;
}

bool PhysicWorld::roundFinished()
{
//	if (!mIsBallValid)
//		if (mBallVelocity.length() < 10.0)
//			return true;
	return !mIsBallValid;
}

float PhysicWorld::lastHitIntensity()
{
//	return mLastHitIntensity;
//TODO:	implement intensity checking in hit functions and return this value
//	this is currently 1.0 because it's used for volume regulation
	return 1.0;
}

bool PhysicWorld::ballHitLeftPlayerTop()
{
	if (Vector2(
	mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y-BLOBBY_UPPER_SPHERE)
	).length() <= BALL_RADIUS + BLOBBY_UPPER_RADIUS)
	return true;
	
	return false;
	// TODO: Calculate hit intensity
}

bool PhysicWorld::ballHitRightPlayerTop()
{
	if (Vector2(
	mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y-BLOBBY_UPPER_SPHERE)
	).length() <= BALL_RADIUS + BLOBBY_UPPER_RADIUS)
	return true;
    
    return false;
	// TODO: Calculate hit intensity
}

bool PhysicWorld::ballHitLeftPlayerBottom()
{
	if (Vector2(
	mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y+BLOBBY_LOWER_SPHERE)
	).length() <= BALL_RADIUS + BLOBBY_LOWER_RADIUS)
	return true;
	
	return false;
	// TODO: Calculate hit intensity
} 

bool PhysicWorld::ballHitRightPlayerBottom()
{
	if (Vector2(
	mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y+BLOBBY_LOWER_SPHERE)
	).length() <= BALL_RADIUS + BLOBBY_LOWER_RADIUS)
	return true;
	
	return false;
	// TODO: Calculate hit intensity
}   
     
bool PhysicWorld::ballHitLeftPlayer()
{
	if (ballHitLeftPlayerTop() || ballHitLeftPlayerBottom())
	return true;
	
	return false;
	// TODO: Calculate hit intensity
} 

bool PhysicWorld::ballHitRightPlayer()
{
	if (ballHitRightPlayerTop() || ballHitRightPlayerBottom())
	return true;
	
	return false;
	// TODO: Calculate hit intensity
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
	// This is only influenced by input, so we need to reset this here
	mLeftBlobVelocity.x = 0.0;
	mRightBlobVelocity.x = 0.0;

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
			
			
     
	// Collision Detection
	if(ballHitLeftPlayerTop())
	{
		mBallVelocity = Vector2(mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y-BLOBBY_UPPER_SPHERE));
		mBallVelocity = mBallVelocity.normalise();
		mBallVelocity = mBallVelocity.scale(10);
	}

	if(ballHitRightPlayerTop())
	{
		mBallVelocity = Vector2(mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y-BLOBBY_UPPER_SPHERE));
		mBallVelocity = mBallVelocity.normalise();
		mBallVelocity = mBallVelocity.scale(10);
	}
	
	// Collision Detection
	if(ballHitLeftPlayerBottom())
	{
		mBallVelocity = Vector2(mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y+BLOBBY_LOWER_SPHERE));
		mBallVelocity = mBallVelocity.normalise();
		mBallVelocity = mBallVelocity.scale(10);
	}

	if(ballHitRightPlayerBottom())
	{
		mBallVelocity = Vector2(mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y+BLOBBY_LOWER_SPHERE));
		mBallVelocity = mBallVelocity.normalise();
		mBallVelocity = mBallVelocity.scale(10);
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
	if(mBallPosition.x-BALL_RADIUS<=NET_POSITION_X+NET_RADIUS
	&& mBallPosition.x-BALL_RADIUS>=NET_POSITION_X-NET_RADIUS-10
    && mBallVelocity.x>0
    && mBallPosition.y > NET_POSITION_Y-NET_SPHERE)
		mBallVelocity = mBallVelocity.reflectX();
		
	// Net Sphere
	if (Vector2(
	mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE)
	).length() < NET_RADIUS + BALL_RADIUS)
	{

		temp = mBallVelocity.length();
		mBallVelocity = Vector2(mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE));
		mBallVelocity = mBallVelocity.normalise();
		mBallVelocity = mBallVelocity.scale(temp).scale(0.8);
/*
		mBallVelocity = mBallVelocity.reflect(Vector2(mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE)).normalise()).scale(0.5);
*/
	}
	
	// Velocity Integration
	 
	mBallRotation += mBallAngularVelocity * TIMESTEP;
	
	mLeftBlobPosition += mLeftBlobVelocity * TIMESTEP;
	mRightBlobPosition += mRightBlobVelocity * TIMESTEP;

	// Acceleration Integration
	
	mLeftBlobVelocity.y -= -GRAVITATION * TIMESTEP;
	mRightBlobVelocity.y -= -GRAVITATION * TIMESTEP;

	// Ball Gravitation
	if (mIsGameRunning)
		mBallVelocity.y -= BALL_GRAVITATION * TIMESTEP;
	else if (ballHitLeftPlayer() || ballHitRightPlayer())
		mIsGameRunning = true;
		
	mBallPosition -= mBallVelocity;


	
	
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
	
	
}


