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
const float BLOBBY_JUMP_ACCELERATION = 13.50;

// This is exactly the half of the gravitation, i checked it in
// the original code
const float BLOBBY_JUMP_BUFFER = 0.39;
const float GRAVITATION = 0.78;
const float BALL_GRAVITATION = 0.248;
const float STANDARD_BALL_ANGULAR_VELOCITY = 0.1;
const float STANDARD_BALL_HEIGHT = 269 + BALL_RADIUS;

// Temp
float temp = 0;



PhysicWorld::PhysicWorld()
{
	reset(0);
	mLeftBlobbyAnimationSpeedAtm = 0.0;
	mRightBlobbyAnimationSpeedAtm = 0.0;
	mTimeSinceBallout = 0.0;
}

PhysicWorld::~PhysicWorld()
{
}

bool PhysicWorld::resetAreaClear()
{
	Vector2 oldBallPos = mBallPosition;
	mBallPosition = Vector2(200, STANDARD_BALL_HEIGHT);
	if (int_BallHitLeftPlayer())
	{
		mBallPosition = oldBallPos;
		return false;
	}
	
	mBallPosition = Vector2(600, STANDARD_BALL_HEIGHT);
	if (int_BallHitRightPlayer())
	{
		mBallPosition = oldBallPos;
		return false;
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
	mLeftBlobState = 0.0;
	mRightBlobState = 0.0;

	mIsGameRunning = false;
	mIsBallValid = true;
	
	mLastHitIntensity = 0.0;
}

void PhysicWorld::resetPlayer()
{
	mLeftBlobPosition = Vector2( 200,
		GROUND_PLANE_HEIGHT + BLOBBY_HEIGHT / 2.0);
        mRightBlobPosition = Vector2(600,
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

Vector2 PhysicWorld::getBlob(int player)
{
	if (player == 0)
		return mLeftBlobPosition;
	else if (player == 1)
		return mRightBlobPosition;
	else
		return Vector2(0,0);
}



float PhysicWorld::getBlobState(int player)
{
	if (player == 0)
		return mLeftBlobState;
	else if (player == 1)
		return mRightBlobState;
	else
		return 0;
}

void PhysicWorld::setLeftInput(const PlayerInput& input)
{
	mLeftPlayerInput = input;
}

void PhysicWorld::setRightInput(const PlayerInput& input)
{
	mRightPlayerInput = input;
}

// Blobby animation methods
void PhysicWorld::blobbyAnimationStep(int player)
{
	if (player == 0)
	{
		if (mLeftBlobState < 0.0)
		{
			mLeftBlobbyAnimationSpeedAtm = 0;
			mLeftBlobState = 0;
		}

		if (mLeftBlobState >= 4.5)
		{
			mLeftBlobbyAnimationSpeedAtm =- BLOBBY_ANIMATION_SPEED;
		}
		
		mLeftBlobState += mLeftBlobbyAnimationSpeedAtm;
		
		if (mLeftBlobState >= 5)
		{
			mLeftBlobState = 4.99;
   		}
	}
	else if (player == 1)
	{
		if (mRightBlobState < 0.0)
		{
			mRightBlobbyAnimationSpeedAtm = 0;
			mRightBlobState = 0;
		}
	
		if (mRightBlobState >= 4.5)
		{
			mRightBlobbyAnimationSpeedAtm =- BLOBBY_ANIMATION_SPEED;
    	}
		mRightBlobState += mRightBlobbyAnimationSpeedAtm;
		if (mRightBlobState >= 5)
		{
			mRightBlobState = 4.99;
 		}
	}
}

void PhysicWorld::blobbyStartAnimation(int player)
{
	if (player == 0)
	{
		if (mLeftBlobbyAnimationSpeedAtm == 0)
		mLeftBlobbyAnimationSpeedAtm = BLOBBY_ANIMATION_SPEED;
	}
	else if (player == 1)
	{
		if (mRightBlobbyAnimationSpeedAtm == 0)
		mRightBlobbyAnimationSpeedAtm = BLOBBY_ANIMATION_SPEED;
	}
}

void PhysicWorld::step()
{
    // Input Handling
	if (blobbyHitGround(LEFT_PLAYER))
		if (mLeftPlayerInput.up)
		{
			mLeftBlobVelocity.y = -BLOBBY_JUMP_ACCELERATION;
			blobbyStartAnimation(LEFT_PLAYER);
		}
	if (mLeftPlayerInput.up)
		mLeftBlobVelocity.y -= BLOBBY_JUMP_BUFFER;
	
	if (blobbyHitGround(RIGHT_PLAYER))
		if (mRightPlayerInput.up)
		{
			mRightBlobVelocity.y = -BLOBBY_JUMP_ACCELERATION;
			blobbyStartAnimation(RIGHT_PLAYER);
		}
		
	if (mRightPlayerInput.up)
		mRightBlobVelocity.y -= BLOBBY_JUMP_BUFFER;
		// This is only influenced by input, so we need to reset this here
	mLeftBlobVelocity.x = 0.0;
	mRightBlobVelocity.x = 0.0;
		if (mLeftPlayerInput.left)
	{
		if(blobbyHitGround(LEFT_PLAYER))
			blobbyStartAnimation(LEFT_PLAYER);
		mLeftBlobVelocity.x -= BLOBBY_SPEED;
	}
	
	if (mLeftPlayerInput.right)
	{
		if(blobbyHitGround(LEFT_PLAYER))
			blobbyStartAnimation(LEFT_PLAYER);
		mLeftBlobVelocity.x += BLOBBY_SPEED;
	}
	
	if (mRightPlayerInput.left)
	{
		if(blobbyHitGround(RIGHT_PLAYER))
			blobbyStartAnimation(RIGHT_PLAYER);
		mRightBlobVelocity.x -= BLOBBY_SPEED;
	}	
	
	if (mRightPlayerInput.right)
	{
		if(blobbyHitGround(RIGHT_PLAYER))
			blobbyStartAnimation(RIGHT_PLAYER);
		mRightBlobVelocity.x += BLOBBY_SPEED;
	}

	// Reset the ball-blobby collision
	mBallHitByRightBlob = false;
	mBallHitByLeftBlob = false;

    for (short counter = 1; counter <= TIMESTEP; counter++)
	{
		// Collision detection
		if(mIsBallValid)
		{
			if(int_BallHitLeftPlayerBottom())
			{
				mBallVelocity = Vector2(mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y+BLOBBY_LOWER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition -= mBallVelocity.scale(3);
				mBallVelocity = mBallVelocity.scale(13);
				mBallHitByLeftBlob=true;
				mLastHitIntensity = Vector2(mBallVelocity, mRightBlobVelocity).length();
			}

			else if(int_BallHitRightPlayerBottom())
			{
				mBallVelocity = Vector2(mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y+BLOBBY_LOWER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition -= mBallVelocity.scale(3);
				mBallVelocity = mBallVelocity.scale(13);
				mBallHitByRightBlob=true;
				mLastHitIntensity = Vector2(mBallVelocity, mRightBlobVelocity).length();
			}	
			else if(int_BallHitLeftPlayerTop())
			{
				mBallVelocity = Vector2(mBallPosition,Vector2(mLeftBlobPosition.x,mLeftBlobPosition.y-BLOBBY_UPPER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition -= mBallVelocity.scale(3);
				mBallVelocity = mBallVelocity.scale(13);
				mBallHitByLeftBlob = true;
				mLastHitIntensity = Vector2(mBallVelocity, mLeftBlobVelocity).length();
			}

			else if(int_BallHitRightPlayerTop())
			{
				mBallVelocity = Vector2(mBallPosition,Vector2(mRightBlobPosition.x,mRightBlobPosition.y-BLOBBY_UPPER_SPHERE));
				mBallVelocity = mBallVelocity.normalise();
				mBallPosition -= mBallVelocity.scale(3);
				mBallVelocity = mBallVelocity.scale(13);
				mBallHitByRightBlob=true;
				mLastHitIntensity = Vector2(mBallVelocity, mLeftBlobVelocity).length();
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
		if(mBallPosition.x-BALL_RADIUS<=LEFT_PLANE && mBallVelocity.x>0)
			mBallVelocity = mBallVelocity.reflectX();
		if(mBallPosition.x+BALL_RADIUS>=RIGHT_PLANE && mBallVelocity.x<0)
			mBallVelocity = mBallVelocity.reflectX();

		// Net Collision
		
		// Left Net Border
		if(Vector2(
			mBallPosition,Vector2(NET_POSITION_X,mBallPosition.y)
			).length() <= NET_RADIUS + BALL_RADIUS // This is the sync for the "netball" and the border of the net
			&& mBallPosition.x+BALL_RADIUS<=NET_POSITION_X+NET_RADIUS+10
	    	&& mBallVelocity.x<0
	    	&& mBallPosition.y >= NET_POSITION_Y-NET_SPHERE-1)
				mBallVelocity = mBallVelocity.reflectX();

		// Right Net Border  
		else if(Vector2(
			mBallPosition,Vector2(NET_POSITION_X,mBallPosition.y)
			).length() <= NET_RADIUS + BALL_RADIUS // This is the sync for the "netball" and the border of the net
			&& mBallPosition.x-BALL_RADIUS>=NET_POSITION_X-NET_RADIUS-10
		    && mBallVelocity.x>0
		    && mBallPosition.y >= NET_POSITION_Y-NET_SPHERE-1)
				mBallVelocity = mBallVelocity.reflectX();
				
		// Net Sphere
		else if (Vector2(
			mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE)
			).length() <= NET_RADIUS + BALL_RADIUS)
		{
			if(mBallNetOverlapped==0)
			{
			mBallVelocityTemp=mBallVelocity;
			mBallNetOverlapped=1;
			}
			mBallVelocity = mBallVelocityTemp.reflect(
            Vector2(mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE))
			.normalise()).scale(0.7);
		}
		else
			mBallNetOverlapped=0;
		
		mLeftBlobPosition += mLeftBlobVelocity/TIMESTEP;
		mRightBlobPosition += mRightBlobVelocity/TIMESTEP;

		// Collision between blobby and the net
		if (mLeftBlobPosition.x+BLOBBY_LOWER_RADIUS>NET_POSITION_X-NET_RADIUS) // Collision with the net
			mLeftBlobPosition.x=NET_POSITION_X-NET_RADIUS-BLOBBY_LOWER_RADIUS;
			
		// Collision between blobby and the net
		if (mRightBlobPosition.x-BLOBBY_LOWER_RADIUS<NET_POSITION_X+NET_RADIUS) // Collision with the net
			mRightBlobPosition.x=NET_POSITION_X+NET_RADIUS+BLOBBY_LOWER_RADIUS;
		
		// Collision between blobby and the border
		if (mLeftBlobPosition.x < LEFT_PLANE)
			mLeftBlobPosition.x=LEFT_PLANE;

		// Collision between blobby and the border
		if (mRightBlobPosition.x > RIGHT_PLANE)
			mRightBlobPosition.x=RIGHT_PLANE;
			

		// Acceleration Integration
	
		mLeftBlobVelocity.y -= -GRAVITATION/TIMESTEP;
		mRightBlobVelocity.y -= -GRAVITATION/TIMESTEP;

		// Ball Gravitation
		if (mIsGameRunning)
			mBallVelocity.y -= BALL_GRAVITATION/TIMESTEP;
		else if (ballHitLeftPlayer() || ballHitRightPlayer())
			mIsGameRunning = true;
		
		mBallPosition -= mBallVelocity/TIMESTEP;


	
		if (mLeftBlobPosition.y > GROUND_PLANE_HEIGHT)
		{
			if(mLeftBlobVelocity.y>0.7)
				blobbyStartAnimation(LEFT_PLAYER);
			mLeftBlobPosition.y = GROUND_PLANE_HEIGHT;
			mLeftBlobVelocity.y = 0.0;
			// We need an error correction here because the y coordinate
			// is computed with a physical simulation of the gravitation.
		}
		if (mRightBlobPosition.y > GROUND_PLANE_HEIGHT)
		{
 			if(mRightBlobVelocity.y>0.7)
				blobbyStartAnimation(RIGHT_PLAYER);
			mRightBlobPosition.y = GROUND_PLANE_HEIGHT;
			mRightBlobVelocity.y = 0.0;
		}
	
	} // Ende der Schleife
	
	// Velocity Integration
	if(mBallVelocity.x<0)
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
	return -mBallVelocity;
}

bool PhysicWorld::getBlobJump(PlayerSide player)
{
	return blobbyHitGround(player);
}

float PhysicWorld::estimateBallImpact()
{
// v = a*t + v0
// sy = 1/2 * v * t
// sy = (a * t * t + v0 * t) / 2

// sx = vx * t

// t = (sqrt(v0*v0 + 8 * a * s) + v0) / (2 *a)  
// sx = (sqrt(v0*v0 + 8 * a * s) + v0) / (2 *a) * vx

	const float v0 = mBallVelocity.y;
	const float a = BALL_GRAVITATION;
	float height = GROUND_PLANE_HEIGHT - mBallPosition.y;
	float delta = fabs((sqrt(v0 * v0 + 8 * a * height) + v0) / (2 * a));
	return mBallPosition.x - mBallVelocity.x * delta;
}

Vector2 PhysicWorld::estimateBallPosition(int steps)
{
	return mBallPosition + mBallVelocity * float(steps);
}

bool PhysicWorld::getBallActive()
{
	return mIsGameRunning;
}
