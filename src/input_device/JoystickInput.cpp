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
#include "JoystickPool.h"
#include "InputManager.h"

// ********************************************************************************************************
// 		Interface Definition
// ********************************************************************************************************

/*! \class JoystickInputDevice
	\brief Ingame Joystick input
*/
class JoystickInputDevice : public InputDevice
{
	public:
		~JoystickInputDevice() override = default;
		JoystickInputDevice(InputManager* inputMgr, JoystickAction laction, JoystickAction raction,
				JoystickAction jaction);

		PlayerInputAbs transferInput() override;

	private:

		JoystickAction mLeftAction;
		JoystickAction mRightAction;
		JoystickAction mJumpAction;
};

// ********************************************************************************************************
// 		Class Implementation
// ********************************************************************************************************

// -------------------------------------------------------------------------------------------------
//		Creator Function
// -------------------------------------------------------------------------------------------------

std::unique_ptr<InputDevice> createJoystickInput(InputManager* inputMgr, JoystickAction left, JoystickAction right, JoystickAction jump)
{
	return std::unique_ptr<InputDevice>{new JoystickInputDevice(inputMgr, left, right, jump)};
}

// -------------------------------------------------------------------------------------------------
// 		Joystick Input Device
// -------------------------------------------------------------------------------------------------

JoystickInputDevice::JoystickInputDevice(InputManager* inputMgr, JoystickAction laction, JoystickAction raction, JoystickAction jaction)
	: InputDevice(inputMgr)
	, mLeftAction(laction)
	, mRightAction(raction)
	, mJumpAction(jaction)
{
}

PlayerInputAbs JoystickInputDevice::transferInput()
{
	return { mInputManager->isJoyActionActive(mLeftAction),
			 mInputManager->isJoyActionActive(mRightAction),
			 mInputManager->isJoyActionActive(mJumpAction) };
}

