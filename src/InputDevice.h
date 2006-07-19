#pragma once

#include <SDL/SDL.h>
#include "InputSource.h"

class InputDevice
{
public:
	InputDevice();


	virtual PlayerInput getInput() = 0;
};

class MouseInputDevice : public InputDevice
{
private:
	
public:
	MouseInputDevice()
		: InputDevice()
	{
	}
	
	virtual PlayerInput getInput()
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
	
public:
	KeyboardInputDevice(SDLKey leftKey, SDLKey rightKey, SDLKey jumpKey)
		: InputDevice()
	{
		mLeftKey = leftKey;
		mRightKey = rightKey;
		mJumpKey = jumpKey;	
	}
	
	virtual PlayerInput getInput()
	{
	
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

	virtual PlayerInput getInput()
	{
	
	}
};

