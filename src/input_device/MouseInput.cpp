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
#include "RenderManager.h"
#include "InputManager.h"

// ********************************************************************************************************
// 		Interface Definition
// ********************************************************************************************************

/*! \class MouseInputDevice
	\brief Ingame mouse control
*/
class MouseInputDevice : public InputDevice
{
	private:
		PlayerSide mPlayer;
		int mJumpButton;
		int mMarkerX;
		bool mDelay; // The pressed button of the mainmenu must be ignored

	public:
		virtual ~MouseInputDevice(){};
		MouseInputDevice(PlayerSide player, int jumpbutton);
		virtual PlayerInputAbs transferInput();
};

// ********************************************************************************************************
// 		Class Implementation
// ********************************************************************************************************

// -------------------------------------------------------------------------------------------------
//		Creator Function
// -------------------------------------------------------------------------------------------------

InputDevice* createMouseInput(PlayerSide player, int jumpbutton)
{
	return new MouseInputDevice(player, jumpbutton);
}

// -------------------------------------------------------------------------------------------------
// 		Keyboard Input Device
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

PlayerInputAbs MouseInputDevice::transferInput()
{
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

	mMarkerX = mMouseXPos + playerOffset;
	input.setTarget( mMarkerX, mPlayer );

	RenderManager::getSingleton().setMouseMarker(mMarkerX);

	return input;
}
