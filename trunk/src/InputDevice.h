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

#pragma once

#include <SDL/SDL.h>
#include <map>
#include "RenderManager.h"
#include "DuelMatch.h"

class JoystickPool
{
public:
	static JoystickPool& getSingleton()
	{
		if (mSingleton == 0)
			mSingleton = new JoystickPool();
		return *mSingleton;
	}
	
	SDL_Joystick* getJoystick(int id)
	{
		SDL_Joystick* joy =  mJoyMap[id];
		if (!joy)
		
			std::cerr << "Warning: could not find joystick number "
				<< id << "!" << std::endl;
		return joy;
	}
	
	int probeJoysticks()
	{
		int id = 0;
		SDL_Joystick* lastjoy;
		while ((lastjoy = SDL_JoystickOpen(id)))
		{
			mJoyMap[id] = lastjoy;
			id++;
		}
		return id;
	}
	
	void closeJoysticks()
	{
		for (JoyMap::iterator iter = mJoyMap.begin();
			iter != mJoyMap.end(); ++iter)
		{
			SDL_JoystickClose((*iter).second);
		}
	}
	
private:
	typedef std::map<int, SDL_Joystick*> JoyMap;
	JoyMap mJoyMap;
	static JoystickPool* mSingleton;
};

struct JoystickAction
{
	enum Type
	{
		AXIS,
		BUTTON,
// 	We don't implement these exotic input methods here
//		HAT,
//		TRACKBALL
	};

	JoystickAction(std::string string);
	JoystickAction(int _joyid, Type _type, int _number)
		: type(_type), joy(0), joyid(_joyid),
			number(_number) {}
	~JoystickAction();
	JoystickAction(const JoystickAction& action);

	std::string toString();
	
	Type type;
	
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
		bool warp = SDL_GetAppState() & SDL_APPINPUTFOCUS;
		int mouseState = SDL_GetMouseState(&mMouseXPos, NULL);

		if (warp)
			SDL_WarpMouse(mMouseXPos, 310);
		
		if (mouseState == 0)
			mDelay = false;

		if((mouseState & SDL_BUTTON(mJumpButton)) && !mDelay)
			input.up = true;

		const int playerOffset = mPlayer == RIGHT_PLAYER ? 200 : -200;
		mMouseXPos = mMouseXPos < 201 ? 201 : mMouseXPos;
		if (mMouseXPos <= 201 && warp)
			SDL_WarpMouse(201, 310);

		mMouseXPos = mMouseXPos > 600 ? 600 : mMouseXPos;
		if (mMouseXPos >= 600 && warp)
			SDL_WarpMouse(600, 310);
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

