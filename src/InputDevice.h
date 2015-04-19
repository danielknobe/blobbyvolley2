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

#pragma once

#include <SDL2/SDL_events.h>
#include "PlayerInput.h"
#include "BlobbyDebug.h"

/*! \class InputDevice
	\brief Abstract base class for game input methods
*/
class InputDevice : public ObjectCounter<InputDevice>
{
	public:
		InputDevice() {}
		virtual ~InputDevice() {}

		virtual PlayerInputAbs transferInput() = 0;
};

struct JoystickAction;

InputDevice* createKeyboardInput(SDL_Keycode left, SDL_Keycode right, SDL_Keycode jump);
InputDevice* createJoystrickInput(JoystickAction left, JoystickAction right, JoystickAction jump);
InputDevice* createTouchInput(PlayerSide side, int type);
InputDevice* createMouseInput(PlayerSide player, int jumpbutton, float sensitivity);
