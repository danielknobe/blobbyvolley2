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
#include "InputManager.h"

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
		~KeyboardInputDevice() override = default;
		KeyboardInputDevice(InputManager* inputMgr, SDL_Keycode leftKey, SDL_Keycode rightKey, SDL_Keycode jumpKey);
		PlayerInputAbs transferInput() override;
};

// ********************************************************************************************************
// 		Class Implementation
// ********************************************************************************************************

// -------------------------------------------------------------------------------------------------
//		Creator Function
// -------------------------------------------------------------------------------------------------

std::unique_ptr<InputDevice> createKeyboardInput(InputManager* inputMgr, SDL_Keycode left, SDL_Keycode right, SDL_Keycode jump)
{
	return std::make_unique<KeyboardInputDevice>(inputMgr, left, right, jump);
}

// -------------------------------------------------------------------------------------------------
// 		Keyboard Input Device
// -------------------------------------------------------------------------------------------------

KeyboardInputDevice::KeyboardInputDevice(InputManager* inputMgr, SDL_Keycode leftKey, SDL_Keycode rightKey, SDL_Keycode jumpKey)
	: InputDevice(inputMgr), mLeftKey(leftKey), mRightKey(rightKey), mJumpKey(jumpKey)
{
}

PlayerInputAbs KeyboardInputDevice::transferInput()
{
	return {mInputManager->isKeyPressed(mLeftKey),
		    mInputManager->isKeyPressed(mRightKey),
		    mInputManager->isKeyPressed(mJumpKey)};
}


