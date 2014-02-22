/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

/* header include */
#include "InputDevice.h"
#include "InputManager.h"

/* includes */
#include <iostream>
#include <cstdio>
#include <sstream>

#include "DuelMatch.h"
#include "RenderManager.h"
#include "GameConstants.h"

/* implementation */

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

PlayerInputAbs MouseInputDevice::transferInput(const InputSource* source)
{
	// check if we have a running game,
	// otherwise leave directly
	const DuelMatch* match = source->getMatch();
	assert(match);

	PlayerInputAbs input = PlayerInputAbs();

	int mMouseXPos;

	SDL_Window* window = RenderManager::getSingleton().getWindow();
	bool warp = InputManager::getSingleton()->windowFocus(); //SDL_GetAppState() & SDL_APPINPUTFOCUS;
	int mouseState = SDL_GetMouseState(&mMouseXPos, NULL);

	if (warp)
		SDL_WarpMouseInWindow(window, mMouseXPos, 310);

	if (mouseState == 0)
		mDelay = false;

	if((mouseState & SDL_BUTTON(mJumpButton)) && !mDelay)
		input.setJump( true );

	const int playerOffset = mPlayer == RIGHT_PLAYER ? 200 : -200;
	mMouseXPos = mMouseXPos < 201 ? 201 : mMouseXPos;

	if (mMouseXPos <= 201 && warp)
		SDL_WarpMouseInWindow(window, 201, 310);

	mMouseXPos = mMouseXPos > 600 ? 600 : mMouseXPos;
	if (mMouseXPos >= 600 && warp)
		SDL_WarpMouseInWindow(window, 600, 310);

	// here we load the current position of the player.
	float blobpos = match->getBlobPosition(mPlayer).x;

	mMarkerX = mMouseXPos + playerOffset;
	if (blobpos + BLOBBY_SPEED * 2 <= mMarkerX)
		input.setRight(true);
	else if (blobpos - BLOBBY_SPEED * 2 >= mMarkerX)
		input.setLeft(true);

	RenderManager::getSingleton().setMouseMarker(mMarkerX);

	return input;
}

// -------------------------------------------------------------------------------------------------
// 		TOUCH INPUT
// -------------------------------------------------------------------------------------------------
TouchInputDevice::TouchInputDevice(PlayerSide player, int type)
	: InputDevice()
{
	mPlayer = player;
	mTouchXPos = 400;
	mTouchType = type;
}

PlayerInputAbs TouchInputDevice::transferInput(const InputSource* source)
{
	// check if we have a running game,
	// otherwise leave directly
	const DuelMatch* match = source->getMatch();
	assert(match);

	PlayerInputAbs input = PlayerInputAbs();

	// Get the primary touch device
	SDL_TouchID device = SDL_GetTouchDevice(0);

	switch (mTouchType)
	{
	// ******************************************************************
	// Blobby moves to playerside finger, opponentside is the jumpbutton*
	// ******************************************************************
	case 0:
	{
		for (int i = 0; i < SDL_GetNumTouchFingers(device); i++)
		{
			SDL_Finger *finger = SDL_GetTouchFinger(device, i);

			if (finger == NULL)
				continue;

			// Check the playerside
			if (mPlayer == LEFT_PLAYER)
			{
				// If finger has a valid blobby position, take it!
				if (finger->x < 0.6) {
					mTouchXPos = 200 + finger->x * 800;
				}

				// If finger has a valid jump position
				if (finger->x > 0.7) {
					input.setJump( true );
				}
			}
			else
			{
				// If finger has a valid blobby position, take it!
				if (finger->x > 0.4) {
					mTouchXPos = -200 + finger->x * 800;
				}

				// If finger has a valid jump position
				if (finger->x < 0.3) {
					input.setJump( true );
				}
			}
		}

		const int playerOffset = mPlayer == RIGHT_PLAYER ? 200 : -200;
		mTouchXPos = mTouchXPos < 201 ? 201 : mTouchXPos;
		mTouchXPos = mTouchXPos > 600 ? 600 : mTouchXPos;

		// here we load the current position of the player.
		float blobpos = match->getBlobPosition(mPlayer).x;

		mMarkerX = mTouchXPos + playerOffset;
		if (blobpos + BLOBBY_SPEED * 2 <= mMarkerX)
			input.setRight(true);
		else if (blobpos - BLOBBY_SPEED * 2 >= mMarkerX)
			input.setLeft(true);

		RenderManager::getSingleton().setMouseMarker(mMarkerX);
		break;
	}


	// *********************************************************************************
	// *playerside is devided in left/right move button, opponentside is the jumpbutton*
	// *********************************************************************************
	case 1:
	{
		for (int i = 0; i < SDL_GetNumTouchFingers(device); i++)
		{
			SDL_Finger *finger = SDL_GetTouchFinger(device, i);

			if (finger == NULL)
				continue;

			// Check the playerside
			if (mPlayer == LEFT_PLAYER)
			{
				// If finger is on the playerside
				if (finger->x < 0.5) {
					// Left arrow
					if (finger->x < 0.25)
					{
						input.setLeft(true);
					}
					else
					{
						input.setRight(true);
					}
				}

				// If finger has a valid jump position
				if (finger->x > 0.7) {
					input.setJump(true);
				}
			}
			else
			{
				// If finger is on the playerside
				if (finger->x > 0.5) {
					// Left arrow
					if (finger->x < 0.75)
					{
						input.setLeft( true );
					}
					else
					{
						input.setRight( true );
					}
				}

				// If finger has a valid jump position
				if (finger->x < 0.3) {
					input.setJump( true );
				}
			}
		}
		break;
	}
	default:
		// **************************************
		// *An error happens we do no input here*
		// **************************************
		break;
	}
	return input;
}

// -------------------------------------------------------------------------------------------------
// 		KEYBOARD INPUT
// -------------------------------------------------------------------------------------------------


KeyboardInputDevice::KeyboardInputDevice(SDL_Keycode leftKey, SDL_Keycode rightKey, SDL_Keycode jumpKey)
	: InputDevice()
{
	mLeftKey = leftKey;
	mRightKey = rightKey;
	mJumpKey = jumpKey;
}

PlayerInputAbs KeyboardInputDevice::transferInput(const InputSource* input)
{
	const Uint8* keyState = SDL_GetKeyboardState(0);

	return PlayerInputAbs(keyState[SDL_GetScancodeFromKey(mLeftKey)], keyState[SDL_GetScancodeFromKey(mRightKey)], keyState[SDL_GetScancodeFromKey(mJumpKey)]);
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

void JoystickPool::probeJoysticks()
{
	int numJoysticks = SDL_NumJoysticks();
	SDL_Joystick* lastjoy;
	for(int i = 0; i < numJoysticks; i++)
	{
		lastjoy = SDL_JoystickOpen(i);

		if (lastjoy == NULL)
			continue;

		mJoyMap[SDL_JoystickInstanceID(lastjoy)] = lastjoy;
	}
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
	catch (const std::string& e)
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

PlayerInputAbs JoystickInputDevice::transferInput(const InputSource* source)
{
	return PlayerInputAbs( getAction(mLeftAction),
						getAction(mRightAction),
						getAction(mJumpAction)
					);
}

bool JoystickInputDevice::getAction(const JoystickAction& action)
{
	if (action.joy != 0)
	{
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
	}
	return false;
}

