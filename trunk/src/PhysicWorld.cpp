#include "PhysicWorld.h"

const int TIMESTEP = 5; // calculations per frame

const float TIMEOUT_MAX = 4.0;

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
const float BLOBBY_ANIMATION_SPEED = 0.5;
const float BLOBBY_JUMP_ACCELERATION = 14.5;

// This is exactly the half of the gravitation, i checked it in
// the original code
const float BLOBBY_JUMP_BUFFER = 0.44;
const float GRAVITATION = 0.88;
const float BALL_GRAVITATION = 0.28;
const float STANDARD_BALL_ANGULAR_VELOCITY = 0.1;
const float STANDARD_BALL_HEIGHT = 269 + BALL_RADIUS;

const float BALL_COLLISION_CORRECTION = 3.0;
const float BALL_COLLISION_VELOCITY = 13.125;

// Temp
float temp = 0;



PhysicWorld::PhysicWorld()
{
	reset(0);
	mCurrentBlobbyAnimationSpeed[LEFT_PLAYER] = 0.0;
	mCurrentBlobbyAnimationSpeed[RIGHT_PLAYER] = 0.0;
	mTimeSinceBallout = 0.0;
}

PhysicWorld::~PhysicWorld()
{
}

bool PhysicWorld::resetAreaClear()
{
	Vector2 oldBallPos = mBallPosition;
	bool ret = true;
	for (int i = LEFT_PLAYER; i <= RIGHT_PLAYER; ++i)
	{
		mBallPosition = Vector2(200 + i * 400, STANDARD_BALL_HEIGHT);
		if (playerTopBallCollision(PlayerSide(i)) ||
				playerBottomBallCollision(PlayerSide(i)))
		{
			ret = false;
		}
	}
	mBallPosition = oldBallPos;
	return true;
}

void PhysicWorld::reset(int player)
{
	if (player == 0)
		mBallPosition = Vector2(200, STANDARD_BALL_HEIGHT);
	else if (player == 1)
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
		GROUND_PLANE_HEIGHT + BLOBBY_HEIGHT / 2.0);
	mBlobPosition[RIGHT_PLAYER] = Vector2(600,
		GROUND_PLANE_HEIGHT + BLOBBY_HEIGHT / 2.0);
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

bool PhysicWorld::blobbyHitGround(int player)
{
	if (player == 0)
	{ 
		if (getBlob(LEFT_PLAYER).y >= GROUND_PLANE_HEIGHT)
			return true;
		else
			return false;
	}
	else if (player == 1)
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

bool PhysicWorld::roundFinished()
{
	if (resetAreaClear())
	{
		if (!mIsBallValid)
			if (mBallVelocity.y < 1.5 &&
				mBallVelocity.y > -1.5 && mBallPosition.y > 430)
				return true;
		if (mTimeSinceBallout > TIMEOUT_MAX)
			return true;
	}
	return false;
}

float PhysicWorld::lastHitIntensity()
{
	float intensity = mLastHitIntensity / 14.0;
	return intensity < 1.0 ? intensity : 1.0;
}

bool PhysicWorld::playerTopBallCollision(PlayerSide player)
{
	if (Vector2(mBallPosition,
		Vector2(mBlobPosition[player].x,
			mBlobPosition[player].y - BLOBBY_UPPER_SPHERE)
			).length() <= BALL_RADIUS + BLOBBY_UPPER_RADIUS)
		return true;
	return false;
}

inline bool PhysicWorld::playerBottomBallCollision(PlayerSide player)
{
	if (Vector2(mBallPosition,
		Vector2(mBlobPosition[player].x,
			mBlobPosition[player].y + BLOBBY_LOWER_SPHERE)
     			).length() <= BALL_RADIUS + BLOBBY_LOWER_RADIUS)
		return true;
	return false;
}

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

Vector2 PhysicWorld::getBlob(PlayerSide player)
{
	return mBlobPosition[player];
}



float PhysicWorld::getBlobState(PlayerSide player)
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

void PhysicWorld::step()
{
    // Input Handling
	for (int i = LEFT_PLAYER; i <= RIGHT_PLAYER; ++i)
	{
		if (blobbyHitGround(PlayerSide(i)))
			if (mPlayerInput[i].up)
			{
				mBlobVelocity[i].y =
					-BLOBBY_JUMP_ACCELERATION;
				blobbyStartAnimation(PlayerSide(i));
		}
		if (mPlayerInput[i].up)
			mBlobVelocity[i].y -= BLOBBY_JUMP_BUFFER;
	
		mBlobVelocity[i].x = 0.0;
		
		if (mPlayerInput[i].left)
		{
			if(blobbyHitGround(PlayerSide(i)))
					blobbyStartAnimation(PlayerSide(i));
				mBlobVelocity[i].x -= BLOBBY_SPEED;
		}
		if (mPlayerInput[i].right)
		{
			if(blobbyHitGround(PlayerSide(i)))
					blobbyStartAnimation(PlayerSide(i));
				mBlobVelocity[i].x += BLOBBY_SPEED;
		}
	}

	// Reset the ball-blobby collision
	mBallHitByRightBlob = false;
	mBallHitByLeftBlob = false;

    for (short counter = 1; counter <= TIMESTEP; counter++)
	{
		// Collision detection
		if(mIsBallValid)
		{
			if(playerBottomBallCollision(LEFT_PLAYER))
			{
				mBallVelocity = -Vector2(mBallPosition,Vector2(mBlobPosition[LEFT_PLAYER].x,mBlobPosition[LEFT_PLAYER].y+BLOBBY_LOWER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition += mBallVelocity.scale(BALL_COLLISION_CORRECTION);
				mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
				mBallHitByLeftBlob=true;
				mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[RIGHT_PLAYER]).length();
			}

			else if(playerBottomBallCollision(RIGHT_PLAYER))
			{
				mBallVelocity = -Vector2(mBallPosition,Vector2(mBlobPosition[RIGHT_PLAYER].x,mBlobPosition[RIGHT_PLAYER].y+BLOBBY_LOWER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition += mBallVelocity.scale(BALL_COLLISION_CORRECTION);
				mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
				mBallHitByRightBlob=true;
				mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[RIGHT_PLAYER]).length();
			}	
			else if(playerTopBallCollision(LEFT_PLAYER))
			{
				mBallVelocity = -Vector2(mBallPosition,Vector2(mBlobPosition[LEFT_PLAYER].x,mBlobPosition[LEFT_PLAYER].y-BLOBBY_UPPER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition += mBallVelocity.scale(BALL_COLLISION_CORRECTION);
				mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
				mBallHitByLeftBlob = true;
				mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[LEFT_PLAYER]).length();
			}

			else if(playerTopBallCollision(RIGHT_PLAYER))
			{
				mBallVelocity = -Vector2(mBallPosition,Vector2(mBlobPosition[RIGHT_PLAYER].x,mBlobPosition[RIGHT_PLAYER].y-BLOBBY_UPPER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition += mBallVelocity.scale(BALL_COLLISION_CORRECTION);
				mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
				mBallHitByRightBlob=true;
				mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[LEFT_PLAYER]).length();
			}
		

		}
		// Ball to ground Collision
		else
		{
			if (mBallPosition.y + BALL_RADIUS > 500.0)
			{

				mBallVelocity = mBallVelocity.reflectY().scaleY(0.5);
				mBallVelocity = mBallVelocity.scaleX(0.55);

				mBallPosition.y=500 - BALL_RADIUS;
			}

		}

		// Border Collision
		if(mBallPosition.x-BALL_RADIUS<=LEFT_PLANE && mBallVelocity.x < 0.0)
			mBallVelocity = mBallVelocity.reflectX();
		if(mBallPosition.x+BALL_RADIUS>=RIGHT_PLANE && mBallVelocity.x > 0.0)
			mBallVelocity = mBallVelocity.reflectX();

		// Net Collision
		
		// Left Net Border
		if(Vector2(
			mBallPosition,Vector2(NET_POSITION_X,mBallPosition.y)
			).length() <= NET_RADIUS + BALL_RADIUS // This is the sync for the "netball" and the border of the net
			&& mBallPosition.x+BALL_RADIUS<=NET_POSITION_X+NET_RADIUS+15.1
	    	&& mBallVelocity.x > 0.0
	    	&& mBallPosition.y >= NET_POSITION_Y-NET_SPHERE)
				mBallVelocity = mBallVelocity.reflectX();

		// Right Net Border  
		else if(Vector2(
			mBallPosition,Vector2(NET_POSITION_X,mBallPosition.y)
			).length() <= NET_RADIUS + BALL_RADIUS // This is the sync for the "netball" and the border of the net
			&& mBallPosition.x-BALL_RADIUS>=NET_POSITION_X-NET_RADIUS-15.1
		    && mBallVelocity.x < 0.0
		    && mBallPosition.y >= NET_POSITION_Y-NET_SPHERE)
				mBallVelocity = mBallVelocity.reflectX();
				
		// Net Sphere
		else if (Vector2(
			mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE)
			).length() <= NET_RADIUS + BALL_RADIUS && mBallPosition.y < NET_POSITION_Y-NET_SPHERE)
		{
			mBallVelocity = mBallVelocity.reflect(
            Vector2(mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE))
			.normalise()).scale(0.75);
			mBallPosition += mBallVelocity;
		}
		
		mBlobPosition[LEFT_PLAYER] += mBlobVelocity[LEFT_PLAYER]/TIMESTEP;
		mBlobPosition[RIGHT_PLAYER] += mBlobVelocity[RIGHT_PLAYER]/TIMESTEP;

		// Collision between blobby and the net
		if (mBlobPosition[LEFT_PLAYER].x+BLOBBY_LOWER_RADIUS>NET_POSITION_X-NET_RADIUS) // Collision with the net
			mBlobPosition[LEFT_PLAYER].x=NET_POSITION_X-NET_RADIUS-BLOBBY_LOWER_RADIUS;
			
		// Collision between blobby and the net
		if (mBlobPosition[RIGHT_PLAYER].x-BLOBBY_LOWER_RADIUS<NET_POSITION_X+NET_RADIUS) // Collision with the net
			mBlobPosition[RIGHT_PLAYER].x=NET_POSITION_X+NET_RADIUS+BLOBBY_LOWER_RADIUS;
		
		// Collision between blobby and the border
		if (mBlobPosition[LEFT_PLAYER].x < LEFT_PLANE)
			mBlobPosition[LEFT_PLAYER].x=LEFT_PLANE;

		// Collision between blobby and the border
		if (mBlobPosition[RIGHT_PLAYER].x > RIGHT_PLANE)
			mBlobPosition[RIGHT_PLAYER].x=RIGHT_PLANE;
			

		// Acceleration Integration
	
		mBlobVelocity[LEFT_PLAYER].y += GRAVITATION/TIMESTEP;
		mBlobVelocity[RIGHT_PLAYER].y += GRAVITATION/TIMESTEP;

		// Ball Gravitation
		if (mIsGameRunning)
			mBallVelocity.y += BALL_GRAVITATION/TIMESTEP;
		else if (ballHitLeftPlayer() || ballHitRightPlayer())
			mIsGameRunning = true;
		
		mBallPosition += mBallVelocity/TIMESTEP;


	
		if (mBlobPosition[LEFT_PLAYER].y > GROUND_PLANE_HEIGHT)
		{
			if(mBlobVelocity[LEFT_PLAYER].y>0.7)
				blobbyStartAnimation(LEFT_PLAYER);
			mBlobPosition[LEFT_PLAYER].y = GROUND_PLANE_HEIGHT;
			mBlobVelocity[LEFT_PLAYER].y = 0.0;
			// We need an error correction here because the y coordinate
			// is computed with a physical simulation of the gravitation.
		}
		if (mBlobPosition[RIGHT_PLAYER].y > GROUND_PLANE_HEIGHT)
		{
 			if(mBlobVelocity[RIGHT_PLAYER].y>0.7)
				blobbyStartAnimation(RIGHT_PLAYER);
			mBlobPosition[RIGHT_PLAYER].y = GROUND_PLANE_HEIGHT;
			mBlobVelocity[RIGHT_PLAYER].y = 0.0;
		}
	
	} // Ende der Schleife
	
	// Velocity Integration
	if(mBallVelocity.x > 0.0)
		mBallRotation += mBallAngularVelocity;
		else
		mBallRotation -= mBallAngularVelocity;
		
	// Overflow-Protection
	if(mBallRotation<=0)
		mBallRotation=6.25;
	else
	if(mBallRotation>=6.25)
		mBallRotation=0;

	// Blobbyanimationstep
	blobbyAnimationStep(LEFT_PLAYER);
	blobbyAnimationStep(RIGHT_PLAYER);
	
	mTimeSinceBallout = mIsBallValid ? 0.0 : 
		mTimeSinceBallout + 1.0 / 60;
}

void PhysicWorld::dampBall()
{
	mBallVelocity = mBallVelocity.scale(0.6);
}

Vector2 PhysicWorld::getBallVelocity()
{
	return mBallVelocity;
}

bool PhysicWorld::getBlobJump(PlayerSide player)
{
	return blobbyHitGround(player);
}

#include "RenderManager.h"

float PhysicWorld::estimateBallImpact()
{
	const float a = BALL_GRAVITATION;
	const float height = GROUND_PLANE_HEIGHT - mBallPosition.y - BALL_RADIUS;
	const float vby = mBallVelocity.y;
	float delta1 = -(sqrt(vby * vby + 8 * a * height) + vby) / (2 * a);
	float delta2 = (sqrt(vby * vby + 8 * a * height) - vby) / (2 * a);

	float delta = delta1 > delta2 ? delta1 : delta2;
	return mBallPosition.x + mBallVelocity.x * delta;
}

Vector2 PhysicWorld::estimateBallPosition(int steps)
{
	Vector2 ret;
	ret.x = mBallVelocity.x * float(steps);
	ret.y = (mBallVelocity.y + 0.5 * (BALL_GRAVITATION * float(steps))) * float(steps);
	return mBallPosition + ret;
}

bool PhysicWorld::getBallActive()
{
	return mIsGameRunning;
}
