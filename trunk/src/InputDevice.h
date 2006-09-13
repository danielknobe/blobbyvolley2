#pragma once

#include <SDL/SDL.h>
#include "RenderManager.h"
#include "DuelMatch.h"
enum JoystickCross
{
	JOY_LEFT = 101,
	JOY_RIGHT = 102,
	JOY_UP = 103,
	JOY_DOWN = 104
};

class InputDevice
{
public:
	InputDevice(){};
	virtual ~InputDevice(){};

	virtual void transferInput(PlayerInput& mInput) = 0;
};

class MouseInputDevice : public InputDevice
{
private:
// TODO: CLEANUP!!! -> after the new input management is acted
	int mPlayer;
	float mMarkerPosition;
	bool mGameInit;
	int mMousePosition;
	bool mDelay; // The pressed button of the mainmenu must be ignored
public:
	virtual ~MouseInputDevice(){};
	MouseInputDevice(int player)
		: InputDevice()
	{
		mPlayer = player;
		mGameInit = false;
		mDelay = false;
	}
	
	void transferInput(PlayerInput& mInput)
	{
		if (DuelMatch::getMainGame() != NULL)
		{
			// clean up
			mInput = PlayerInput( 0, 0, 0);

			if(SDL_GetMouseState(&mMousePosition, NULL)&SDL_BUTTON(1))
			{
		if (mDelay == true)
			mInput.up = true;      
			}
            else
			{
				mDelay = true;
				mInput.up = 0;
			}
			
			if (mGameInit == false)
			{
				if (mPlayer == 0)
					mMarkerPosition = (DuelMatch::getMainGame()->getBlobPosition(LEFT_PLAYER)).x;

		        if (mPlayer == 1)
				mMarkerPosition = (DuelMatch::getMainGame()->getBlobPosition(RIGHT_PLAYER)).x;

				SDL_WarpMouse(400, 300);
				mGameInit = true;
				mInput.up = false;
				mDelay = false;
			}
			


			
			mMarkerPosition = mMarkerPosition + (mMousePosition - 400);
			
			if (mPlayer == 0)
			{
				SDL_WarpMouse(400, 300);
				
				if (mMarkerPosition < 1)
					mMarkerPosition = 1;
					
				if (mMarkerPosition > 393)
					mMarkerPosition = 393;
					
				if ((DuelMatch::getMainGame()->getBlobPosition(LEFT_PLAYER)).x + BLOBBY_SPEED < mMarkerPosition)
					mInput.right = true;
				
				if ((DuelMatch::getMainGame()->getBlobPosition(LEFT_PLAYER)).x - BLOBBY_SPEED > mMarkerPosition)
					mInput.left = true;
					
			}

			else if (mPlayer == 1)
			{
				SDL_WarpMouse(400, 300);
				
				if (mMarkerPosition > 800)
					mMarkerPosition = 800;
					
				if (mMarkerPosition < 408)
					mMarkerPosition = 408;

				if ((DuelMatch::getMainGame()->getBlobPosition(RIGHT_PLAYER)).x + BLOBBY_SPEED < mMarkerPosition)
					mInput.right = true;
				
				if ((DuelMatch::getMainGame()->getBlobPosition(RIGHT_PLAYER)).x - BLOBBY_SPEED > mMarkerPosition)
					mInput.left = true;
			}

		RenderManager::getSingleton().setMouseMarker(mMarkerPosition);
		}



	}
	
};

class KeyboardInputDevice : public InputDevice
{
private:
	SDLKey mLeftKey;
	SDLKey mRightKey;
	SDLKey mJumpKey;
	Uint8* mKeyState;
public:
	virtual ~KeyboardInputDevice(){};
	KeyboardInputDevice(SDLKey leftKey, SDLKey rightKey, SDLKey jumpKey)
		: InputDevice()
	{
		mLeftKey = leftKey;
		mRightKey = rightKey;
		mJumpKey = jumpKey;	
	}
	
	void transferInput(PlayerInput& mInput)
	{
	mKeyState = SDL_GetKeyState(0);	
	mInput = PlayerInput( mKeyState[mLeftKey], mKeyState[mRightKey], mKeyState[mJumpKey]);
	}
};

//TODO: second analogstick
class JoystickInputDevice : public InputDevice
{
private:
	SDL_Joystick* mJoystickNumber;
	int mLeftInput;
	int mRightInput;
	int mJumpInput;
	bool mDiagonal;
public:
	~JoystickInputDevice() {};
	JoystickInputDevice(SDL_Joystick* joystickNumber, int leftInput, int rightInput, int jumpInput,
		bool diagonal, int leftJumpInput, int rightJumpInput)
	{
		mJoystickNumber = joystickNumber;
		mLeftInput = leftInput;
		mRightInput = rightInput;
		mJumpInput = jumpInput;
		mDiagonal = diagonal;
		// Button: 1-100
	}

	bool inputToBool(int &input)
	{
		if( input <= 100 )
			return SDL_JoystickGetButton(mJoystickNumber, input);
		if (input == 101)
			return (-10 > SDL_JoystickGetAxis(mJoystickNumber, 0));
		if (input == 102)
			return (10 < SDL_JoystickGetAxis(mJoystickNumber, 0));
		if (input == 103)
			return (10 < SDL_JoystickGetAxis(mJoystickNumber, 1));
		if (input == 104)
			return (-10 > SDL_JoystickGetAxis(mJoystickNumber, 1));
		return false;
	}

	void transferInput(PlayerInput& mInput)
	{
	// clean up
	mInput = PlayerInput( 0, 0, 0);

	mInput = PlayerInput(inputToBool(mLeftInput), inputToBool(mRightInput), inputToBool(mJumpInput));
	}
};

