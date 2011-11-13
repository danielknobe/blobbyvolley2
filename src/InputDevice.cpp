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

#include "InputDevice.h"

#include <iostream>
#include <sstream>

#include "DuelMatch.h"
#include "RenderManager.h"

// -------------------------------------------------------------------------------------------------
// 		MOUSE INPUT
// -------------------------------------------------------------------------------------------------
MouseInputDevice::MouseInputDevice(PlayerSide player, int jumpbutton)
	: InputDevice()
{
	mJumpButton = jumpbutton;
	mPlayer = player;
	if (SDL_GetMouseState(NULL, NULL))
		mDelay = true;
	else
		mDelay = false;
}

void MouseInputDevice::transferInput(PlayerInput& input)
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
	if (blobpos + BLOBBY_SPEED * 2 <= mMarkerX)
		input.right = true;
	else
	if (blobpos - BLOBBY_SPEED * 2 >= mMarkerX)
		input.left = true;

	RenderManager::getSingleton().setMouseMarker(mMarkerX);
}

// -------------------------------------------------------------------------------------------------
// 		KEYBOARD INPUT
// -------------------------------------------------------------------------------------------------

KeyboardInputDevice::KeyboardInputDevice(SDLKey leftKey, SDLKey rightKey, SDLKey jumpKey)
	: InputDevice()
{
	mLeftKey = leftKey;
	mRightKey = rightKey;
	mJumpKey = jumpKey;	
}

void KeyboardInputDevice::transferInput(PlayerInput& input)
{
	Uint8* keyState = SDL_GetKeyState(0);	
	input = PlayerInput(keyState[mLeftKey], keyState[mRightKey], keyState[mJumpKey]);
}

// -------------------------------------------------------------------------------------------------
// 		JOYSTICK INPUT
// -------------------------------------------------------------------------------------------------

// Joystick Pool
JoystickPool* JoystickPool::mSingleton = 0; //static

JoystickPool& JoystickPool::getSingleton()
	{
		if (mSingleton == 0)
			mSingleton = new JoystickPool();
		return *mSingleton;
	}
	
SDL_Joystick* JoystickPool::getJoystick(int id)
{
	SDL_Joystick* joy =  mJoyMap[id];
	if (!joy)
	
		std::cerr << "Warning: could not find joystick number "
			<< id << "!" << std::endl;
	return joy;
}
	
int JoystickPool::probeJoysticks()
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

void JoystickPool::closeJoysticks()
{
	for (JoyMap::iterator iter = mJoyMap.begin();
		iter != mJoyMap.end(); ++iter)
	{
		SDL_JoystickClose((*iter).second);
	}
}
	
// Joystick Action
	
JoystickAction::JoystickAction(std::string string)
{
	type = AXIS;
	number = 0;
	joy = 0;
	joyid = 0;
	try
	{
		const char* str = string.c_str();
		if (strstr(str, "button"))
		{
			type = BUTTON;
			if (sscanf(str, "joy_%d_button_%d", &joyid, &number) < 2)
				throw string;
		}
		else if (strstr(str, "axis"))
		{
			if (sscanf(str, "joy_%d_axis_%d", &joyid, &number) < 2)
				throw string;
		}

		joy = JoystickPool::getSingleton().getJoystick(joyid);
	}
	catch (std::string e)
	{
		std::cerr << "Parse error in joystick config: " << e << std::endl;
	}
}

JoystickAction::JoystickAction(const JoystickAction& action)
{
	type = action.type;
	joy = JoystickPool::getSingleton().getJoystick(action.joyid);
	joyid = action.joyid;
	number = action.number;
}

JoystickAction::~JoystickAction()
{
}

std::string JoystickAction::toString()
{
	const char* typestr = "unknown";
	if (type == AXIS)
		typestr = "axis";
	if (type == BUTTON)
		typestr = "button";
		
	std::stringstream buf;
	buf << "joy_" << joyid << "_" << typestr << "_" << number;
	return buf.str();
}

// Joystick Input Device
JoystickInputDevice::JoystickInputDevice(JoystickAction laction, JoystickAction raction,
		JoystickAction jaction)
	: mLeftAction(laction), mRightAction(raction),
		mJumpAction(jaction)
{
}

void JoystickInputDevice::transferInput(PlayerInput& input)
{
	input.left = getAction(mLeftAction);
	input.right = getAction(mRightAction);
	input.up = getAction(mJumpAction);
}

bool JoystickInputDevice::getAction(const JoystickAction& action)
{
	if (action.joy != 0)
	switch (action.type)
	{
		case JoystickAction::AXIS:
			if (action.number < 0)
			{
				if (SDL_JoystickGetAxis(action.joy,
					-action.number - 1) < -15000)
					return true;
			}
			else if (action.number > 0)
			{
				if (SDL_JoystickGetAxis(action.joy,
					action.number - 1) > 15000)
					return true;
			}
			break;
		case JoystickAction::BUTTON:
			if (SDL_JoystickGetButton(action.joy,
						action.number))
				return true;
			break;
	}
	return false;
}
