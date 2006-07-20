#pragma once

#include <SDL/SDL.h>

class InputDevice
{
public:
       
	InputDevice(){};

	virtual void transferInput(PlayerInput& mInput) = 0;
};

class MouseInputDevice : public InputDevice
{
private:
	
public:
	MouseInputDevice()
		: InputDevice()
	{
	}
	
	void transferInput(PlayerInput& mInput)
	{

		// Wichtig: Muss sich das RenderManager-Singleton holen
		// und die position per setMouseMarkerPosition setzen
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
	mInput = PlayerInput( mKeyState[mLeftKey]
						, mKeyState[mRightKey]
						, mKeyState[mJumpKey]);
	}
};

class JoystickInputDevice : public InputDevice
{
private:
	

public:
	JoystickInputDevice(SDL_Joystick* joy, int leftButton, int rightButton, int jumpButton,
		int walkAxis = -1, int jumpAxis = -1, int secondaryLeftButton = -1, 
		int secondaryRightButton = -1, int secondaryJumpButton = -1)
	{
	
	}

	void transferInput(PlayerInput& mInput)
	{
	
	}
};

