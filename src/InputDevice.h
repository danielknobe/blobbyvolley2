#pragma once

#include <SDL/SDL.h>
#include <map>
#include "RenderManager.h"
#include "DuelMatch.h"

class JoystickPool
{
	std::map<SDL_Joystick*, int> refCounter;
	static JoystickPool* mSingleton;
public:
	SDL_Joystick* openJoystick(int joyid)
	{
		SDL_Joystick* joy = SDL_JoystickOpen(joyid);
		refCounter[joy]++;
		return joy;
	}
	void closeJoystick(SDL_Joystick* joy)
	{
		if (refCounter[joy] < 1)
			throw std::string("failed to close joystick!");
		if (refCounter[joy] == 1)
		{
			refCounter[joy] = 0;
			SDL_JoystickClose(joy);
		}
		else
			refCounter[joy]--;
	}
	static JoystickPool& getSingleton()
	{
		if (mSingleton == NULL)
			mSingleton = new JoystickPool();	
		return *mSingleton;
	}
};

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
	bool close;
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
	int mMarkerX;
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
			return;
		
		int mMouseXPos;

		int mouseState = SDL_GetMouseState(&mMouseXPos, NULL);

		SDL_WarpMouse(mMouseXPos, 300);
		
		if (mouseState == 0)
			mDelay = false;

		if((mouseState & SDL_BUTTON(mJumpButton)) && !mDelay)
			input.up = true;

		const int playerOffset = mPlayer == RIGHT_PLAYER ? 200 : -200;

		mMouseXPos = mMouseXPos < 201 ? 201 : mMouseXPos;
		if (mMouseXPos <= 201)
			SDL_WarpMouse(201, 300);

		mMouseXPos = mMouseXPos > 600 ? 600 : mMouseXPos;
		if (mMouseXPos >= 600)
			SDL_WarpMouse(600, 300);

		float blobpos = match->getBlobPosition(mPlayer).x;

		mMarkerX = mMouseXPos + playerOffset;

		if (blobpos + BLOBBY_SPEED < mMarkerX)
			input.right = true;
		if (blobpos - BLOBBY_SPEED > mMarkerX)
			input.left = true;

		RenderManager::getSingleton().setMouseMarker(mMarkerX);
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
	bool getAction(const JoystickAction& action);

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

