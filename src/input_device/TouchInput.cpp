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
#include <SDL_touch.h>
#include "InputManager.h"

// ********************************************************************************************************
// 		Interface Definition
// ********************************************************************************************************

/*! \class TouchInputDevice
	\brief Ingame touch control
*/
class TouchInputDevice : public InputDevice
{
	public:
		~TouchInputDevice() override = default;
		TouchInputDevice(InputManager* inputMgr, PlayerSide player, int type);
		PlayerInputAbs transferInput() override;

	private:
		PlayerSide mPlayer;
		int mMarkerX;
		int mTouchXPos;
		int mTouchType;
};

// ********************************************************************************************************
// 		Class Implementation
// ********************************************************************************************************

// -------------------------------------------------------------------------------------------------
//		Creator Function
// -------------------------------------------------------------------------------------------------

std::unique_ptr<InputDevice> createTouchInput(InputManager* inputMgr, PlayerSide player, int type)
{
	return std::unique_ptr<InputDevice>{new TouchInputDevice(inputMgr, player, type)};
}

// -------------------------------------------------------------------------------------------------
// 		Keyboard Input Device
// -------------------------------------------------------------------------------------------------
TouchInputDevice::TouchInputDevice(InputManager* inputMgr, PlayerSide player, int type)
	: InputDevice(inputMgr)
{
	mPlayer = player;
	mTouchXPos = 400;
	mTouchType = type;
}

PlayerInputAbs TouchInputDevice::transferInput()
{
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

			if (finger == nullptr)
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

		mMarkerX = mTouchXPos + playerOffset;
		input.setTarget( mMarkerX, mPlayer );

		mInputManager->setMouseMarker(mMarkerX);
		break;
	}


	// *********************************************************************************
	// *playerside is divided in left/right move button, opponentside is the jumpbutton*
	// *********************************************************************************
	case 1:
	{
		for (int i = 0; i < SDL_GetNumTouchFingers(device); i++)
		{
			SDL_Finger *finger = SDL_GetTouchFinger(device, i);

			if (finger == nullptr)
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
