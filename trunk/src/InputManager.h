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

#include "InputSource.h"
#include "InputDevice.h"
#include "Vector.h"

/*! \class InputKeyMap
	\brief mapping of keynames to SDLKey s
*/
struct InputKeyMap
{
	const char *keyname;
	SDLKey key;
};

/// \brief class for managing input
class InputManager
{
	public:
		static InputManager* createInputManager();
		static InputManager* getSingleton();
		~InputManager();

		void beginGame(PlayerSide side);
		void endGame();

		bool running() const;
		PlayerInput getGameInput(int player);
		void updateInput();

		// For GUI navigation (Gamepad, Joystick or Keyboard)
		bool up() const;
		bool down() const;
		bool left() const;
		bool right() const;
		bool select() const;
		bool exit() const; // extention for mouse included, so that right click = exit

		std::string getLastTextKey();
		std::string getLastActionKey();
		int getLastMouseButton() const { return mLastMouseButton; }
		std::string getLastJoyAction() const;

		// For GUI navigation (Mouse)
		Vector2 position();
		bool click() const;
		bool doubleClick() const;
		bool mouseWheelUp() const;
		bool mouseWheelDown() const;
		bool unclick() const;
		
		// config conversion methods
		std::string keyToString(const SDL_keysym& key);
		SDLKey stringToKey(const std::string& keyname);
	
	private:
		static InputManager* mSingleton;
		
		// Keyboard
		static InputKeyMap mKeyMap[];	// Type for String <-convert-> SDLKey

		// GUI storage (because we need event based input for the GUI)
		bool mUp;
		bool mDown;
		bool mLeft;
		bool mRight;
		bool mSelect;
		bool mExit;
		bool mClick;
		bool mDoubleClick;
		bool mMouseWheelUp;
		bool mMouseWheelDown;
		bool mUnclick;
		
		int mMouseX;
		int mMouseY;
		int mLastClickTime;
		
		SDL_keysym mLastInputKey;
		int mLastMouseButton; 
		std::string mLastJoyAction;

		PlayerInput mInput[MAX_PLAYERS];	
		InputDevice *mInputDevice[MAX_PLAYERS];	
		bool mRunning;
		
		InputManager();
		
};
