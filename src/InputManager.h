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

#include <memory>

#include <SDL_keycode.h>

#include "PlayerInput.h"
#include "Vector.h"
#include "BlobbyDebug.h"

enum class KeyAction
{
	UP,
	DOWN,
	LEFT,
	RIGHT,
	SELECT,
	BACK,
	NONE
};

class JoystickPool;
class InputDevice;
class BlobbyApp;
struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;
struct JoystickAction;

/// \brief class for managing input
class InputManager : public ObjectCounter<InputManager>
{
	public:
		InputManager(BlobbyApp* app);
		~InputManager();

		std::unique_ptr<InputDevice> beginGame(PlayerSide side);

		bool running() const;
		void setEndBlobby();	// call to trigger the event that ends blobby, i.e. running will return false after this call.
		void updateInput();

		void captureMouse( bool c );		// sets whether the mouse should be captured
		bool isMouseCaptured() const;

		// For GUI navigation (Gamepad, Joystick or Keyboard)
		bool up() const;
		bool down() const;
		bool left() const;
		bool right() const;
		bool select() const;
		bool exit() const; // extension for mouse included, so that right click = exit

		std::string getLastTextKey();
		std::string getLastActionKey() const;
		int getLastMouseButton() const { return mLastMouseButton; }
		std::string getLastJoyAction() const;

		// For GUI navigation (Mouse)
		Vector2 position();
		bool click() const;
		bool doubleClick() const;
		bool mouseWheelUp() const;
		bool mouseWheelDown() const;
		bool unclick() const;

		// In-game input visualization (Mouse)
		void setMouseMarker(int target);

		// In-game input
		bool isKeyPressed(SDL_Keycode code) const;
		bool isJoyActionActive(const JoystickAction& action) const;

		bool windowFocus() const;

		SDL_Joystick* getJoystickById(int joyId);
	private:
		// Keyboard
		//static InputKeyMap mKeyMap[];	// Type for String <-convert-> SDLKey

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

		bool mWindowFocus;

		int mMouseX;
		int mMouseY;
		float mFingerX;
		float mFingerY;
		int mLastClickTime;

		SDL_Keycode mLastActionKey;
		std::string mLastTextKey;

		int mLastMouseButton;
		std::string mLastJoyAction;

		bool mRunning;

		bool mMouseCaptured;

		BlobbyApp* mApp;
		std::unique_ptr<JoystickPool> mJoystickPool;
};
