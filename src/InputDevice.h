#pragma once

#include <SDL/SDL.h>
#include "RenderManager.h"
#include "DuelMatch.h"

struct JoystickAction
{
	JoystickAction(std::string string);
	~JoystickAction();
	JoystickAction(const JoystickAction& action);

	std::string toString();
	
	enum
	{
		AXIS,
		BUTTON,
// 	We don't implement these exotic input methods here
//		HAT,
//		TRACKBALL
	} type;

	SDL_Joystick* joy;
	int joyid;
	
	// Note: Axis are stored as the SDL axis +1, so we can used
	// the signedness as direction indication
	int number;
};

class InputDevice
{
public:
	InputDevice() {}
	virtual ~InputDevice() {}

	virtual void transferInput(PlayerInput& mInput) = 0;
};

class MouseInputDevice : public InputDevice
{
private:
	PlayerSide mPlayer;
	int mJumpButton;
	bool mDelay; // The pressed button of the mainmenu must be ignored
public:
	virtual ~MouseInputDevice(){};
	MouseInputDevice(PlayerSide player, int jumpbutton)
		: InputDevice()
	{
		mJumpButton = jumpbutton;
		mPlayer = player;
		if (SDL_GetMouseState(NULL, NULL))
			mDelay = true;
		else
			mDelay = false;
	}
	
	void transferInput(PlayerInput& input)
	{
		input = PlayerInput();
		DuelMatch* match = DuelMatch::getMainGame();
		if (match == 0)
		{	
			return;
		}
		
  		int mousexpos;
		int mouseState = SDL_GetMouseState(&mousexpos, NULL);
		if (mouseState == 0)
			mDelay = false;

		if((mouseState & SDL_BUTTON(mJumpButton)) && !mDelay)
		{
			input.up = true;
		}

		const int playerOffset = mPlayer == RIGHT_PLAYER ? 400 : 0;
		mousexpos = mousexpos < 1 ? 1 : mousexpos;
		mousexpos = mousexpos > 393 ? 393 : mousexpos;

		float blobpos = match->getBlobPosition(mPlayer).x;
		if (mPlayer == RIGHT_PLAYER)
			blobpos = 400 - blobpos;

		
		if (blobpos + BLOBBY_SPEED < mousexpos)
			input.right = true;
		if (blobpos - BLOBBY_SPEED > mousexpos)
			input.left = true;
		
		SDL_WarpMouse(mousexpos + playerOffset, 300);
		RenderManager::getSingleton().setMouseMarker(mousexpos + playerOffset);
	}	
};

class KeyboardInputDevice : public InputDevice
{
private:
	SDLKey mLeftKey;
	SDLKey mRightKey;
	SDLKey mJumpKey;
public:
	virtual ~KeyboardInputDevice(){};
	KeyboardInputDevice(SDLKey leftKey, SDLKey rightKey, SDLKey jumpKey)
		: InputDevice()
	{
		mLeftKey = leftKey;
		mRightKey = rightKey;
		mJumpKey = jumpKey;	
	}
	
	void transferInput(PlayerInput& input)
	{
		Uint8* keyState = SDL_GetKeyState(0);	
		input = PlayerInput(keyState[mLeftKey], keyState[mRightKey], keyState[mJumpKey]);
	}
};

class JoystickInputDevice : public InputDevice
{
private:
	bool getAction(const JoystickAction& action)
	{
		switch (action.type)
		{
			case JoystickAction::AXIS:
				if (SDL_JoystickGetAxis(action.joy,
					-action.number - 1) < 10)
					return true;
				if (SDL_JoystickGetAxis(action.joy,
					action.number - 1) > 10)
					return true;
				break;
			case JoystickAction::BUTTON:
				if (SDL_JoystickGetButton(action.joy,
							action.number))
					return true;
				break;
		}
		return false;
	}
	
	JoystickAction mLeftAction;
	JoystickAction mRightAction;
	JoystickAction mJumpAction;
public:
	~JoystickInputDevice() {};
	JoystickInputDevice(JoystickAction laction, JoystickAction raction,
			JoystickAction jaction)
		: mLeftAction(laction), mRightAction(raction),
			mJumpAction(jaction)
	{
	}

	void transferInput(PlayerInput& input)
	{
		input.left = getAction(mLeftAction);
		input.right = getAction(mRightAction);
		input.up = getAction(mJumpAction);
	}
};

