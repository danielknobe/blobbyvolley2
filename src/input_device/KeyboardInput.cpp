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

/* other includes */

// ********************************************************************************************************
// 		Interface Definition
// ********************************************************************************************************

/*! \class KeyboardInputDevice
	\brief Ingame keyboard input
*/
class KeyboardInputDevice : public InputDevice
{
	private:
		SDL_Keycode mLeftKey;
		SDL_Keycode mRightKey;
		SDL_Keycode mJumpKey;
	public:
		virtual ~KeyboardInputDevice(){};
		KeyboardInputDevice(SDL_Keycode leftKey, SDL_Keycode rightKey, SDL_Keycode jumpKey);
		virtual PlayerInputAbs transferInput();
};

// ********************************************************************************************************
// 		Class Implementation
// ********************************************************************************************************

// -------------------------------------------------------------------------------------------------
//		Creator Function
// -------------------------------------------------------------------------------------------------

InputDevice* createKeyboardInput(SDL_Keycode left, SDL_Keycode right, SDL_Keycode jump)
{
	return new KeyboardInputDevice(left, right, jump);
}

// -------------------------------------------------------------------------------------------------
// 		Keyboard Input Device
// -------------------------------------------------------------------------------------------------

KeyboardInputDevice::KeyboardInputDevice(SDL_Keycode leftKey, SDL_Keycode rightKey, SDL_Keycode jumpKey)
	: InputDevice(), mLeftKey(leftKey), mRightKey(rightKey), mJumpKey(jumpKey)
{
}

PlayerInputAbs KeyboardInputDevice::transferInput()
{
	const Uint8* keyState = SDL_GetKeyboardState(0);

	return PlayerInputAbs(keyState[SDL_GetScancodeFromKey(mLeftKey)], keyState[SDL_GetScancodeFromKey(mRightKey)], keyState[SDL_GetScancodeFromKey(mJumpKey)]);
}


