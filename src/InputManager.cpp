﻿/*=============================================================================
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
#include "InputManager.h"

/* includes */
#include <cassert>
#include <iostream>

#include <SDL.h>

#include "BlobbyApp.h"
#include "UserConfig.h"
#include "RenderManager.h"
#include "InputDevice.h"
#include "input_device/JoystickPool.h"

/* implementation */

const unsigned int DOUBLE_CLICK_TIME = 200;

InputManager::InputManager(BlobbyApp* app) : mApp(app)
{
	// joystick
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_JoystickEventState(SDL_ENABLE);
	mJoystickPool = std::unique_ptr<JoystickPool>{new JoystickPool()};
	mJoystickPool->probeJoysticks();

	mRunning = true;
	mWindowFocus = true;

	/// \todo init properly?
	mLastTextKey = "";
	mLastClickTime = 0;
	mMouseCaptured = false;
}

InputManager::~InputManager() = default;

std::unique_ptr<InputDevice> InputManager::beginGame(PlayerSide side)
{
	// Move Mouse to center of window
	int windowX = 0;
	int windowY = 0;
	SDL_GetWindowSize(mApp->getWindow(), &windowX, &windowY);
	SDL_WarpMouseInWindow(mApp->getWindow(), windowX / 2, windowY / 2);

	std::string prefix;
	if (side == LEFT_PLAYER)
		prefix = "left_blobby_";

	if (side == RIGHT_PLAYER)
		prefix = "right_blobby_";

	UserConfig config;
	///  \todo we need only read only access here!
	config.loadFile("inputconfig.xml");
	// determine which device is to be used
	std::string device = config.getString(prefix + "device");

	// load config for mouse
	if (device == "mouse")
	{
		int jumpbutton = config.getInteger(prefix + "mouse_jumpbutton");
		float sensitivity = config.getFloat(prefix + "mouse_sensitivity");
		return createMouseInput(this, side, jumpbutton, sensitivity);
	}
	// load config for keyboard

	else if (device == "keyboard")
	{
		SDL_Keycode lkey = SDL_GetKeyFromName((config.getString(prefix + "keyboard_left")).c_str());
		SDL_Keycode rkey = SDL_GetKeyFromName((config.getString(prefix + "keyboard_right")).c_str());
		SDL_Keycode jkey = SDL_GetKeyFromName((config.getString(prefix + "keyboard_jump")).c_str());
		return createKeyboardInput(this, lkey, rkey, jkey);
	}
	// load config for joystick
	else if (device == "joystick")
	{
		JoystickAction laction(config.getString(prefix + "joystick_left"));
		JoystickAction raction(config.getString(prefix + "joystick_right"));
		JoystickAction jaction(config.getString(prefix + "joystick_jump"));
		return createJoystickInput(this, laction, raction, jaction);
	}
	// load config for touch
	else if (device == "touch")
	{
		return createTouchInput(this, side, config.getInteger("blobby_touch_type"));
	}
	else
		std::cerr << "Error: unknown input device: " << device << std::endl;

	return nullptr;
}

void InputManager::updateInput()
{
	mUp = false;
	mDown = false;
	mLeft = false;
	mRight = false;
	mSelect = false;
	mExit = false;
	mClick = false;
	mMouseWheelUp = false;
	mMouseWheelDown = false;
	mUnclick = false;
	mLastMouseButton = -1;

	mLastActionKey = SDLK_UNKNOWN;

	mLastTextKey = "";
	mLastJoyAction = "";
	// Init GUI Events for buffered Input
	SDL_PumpEvents();
	SDL_Event event;
	SDL_JoystickUpdate();

	// process all SDL events
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				mRunning = false;
				break;
			case SDL_JOYDEVICEADDED:
				mJoystickPool->openJoystick(event.jdevice.which);
				break;
			case SDL_JOYDEVICEREMOVED:
				mJoystickPool->closeJoystick(event.jdevice.which);
				break;
			case SDL_KEYDOWN:
				mLastActionKey = event.key.keysym.sym;
				switch (event.key.keysym.sym)
				{
					case SDLK_UP:
						mUp = true;
						break;

					case SDLK_DOWN:
						mDown = true;
						break;

					case SDLK_LEFT:
						mLeft = true;
						break;

					case SDLK_RIGHT:
						mRight = true;
						break;

					case SDLK_RETURN:
						mLastTextKey = "return";
					case SDLK_SPACE:
						mSelect = true;
						break;

					case SDLK_ESCAPE:
					case SDLK_AC_BACK:
						mExit = true;
						break;
					case SDLK_BACKSPACE:
						mLastTextKey = "backspace";
						break;
					case SDLK_DELETE:
						mLastTextKey = "del";
						break;
					case SDLK_v:
						// CTRL + v -> Paste
						if ((event.key.keysym.mod & KMOD_CTRL) != 0)
						{
							mLastTextKey = "paste";
						}
						break;

					default:
						break;
				}
				break;

			case SDL_TEXTINPUT:
				mLastTextKey = std::string(event.text.text);
				break;
			// Workaround because SDL has a bug in Version 2.0.1,
			// so that we can't use mouse here
#if BLOBBY_ON_DESKTOP
			case SDL_MOUSEBUTTONDOWN:
				mLastMouseButton = event.button.button;
				switch (event.button.button)
				{
					case SDL_BUTTON_LEFT:
						mClick = true;

						if(SDL_GetTicks() - mLastClickTime < DOUBLE_CLICK_TIME )
						{
							mDoubleClick = true;
						}

						mLastClickTime = SDL_GetTicks();
						break;
				}
				break;
#elif BLOBBY_ON_MOBILE
			case SDL_FINGERDOWN:
				mClick = true;

				if(SDL_GetTicks() - mLastClickTime < DOUBLE_CLICK_TIME )
				{
						mDoubleClick = true;
				}

				mLastClickTime = SDL_GetTicks();
				break;
#endif
			case SDL_MOUSEWHEEL:
				if (event.wheel.y < 0) {
					mMouseWheelDown = true;
				}
				else
				{
					mMouseWheelUp = true;
				}
				break;

			case SDL_MOUSEBUTTONUP:
				mUnclick = true;
				break;

			case SDL_JOYAXISMOTION:
			case SDL_JOYBUTTONDOWN:
			{
				JoystickAction joyAction(event.jbutton.which,
					JoystickAction::NONE, event.jbutton.button);

				// Handle Axis
				if (event.type == SDL_JOYAXISMOTION)
				{
					if (abs(event.jaxis.value) > 10000)
					{
						if (event.jaxis.value > 0)
							joyAction.number = event.jaxis.axis + 1;

						if (event.jaxis.value < 0)
							joyAction.number = -(event.jaxis.axis + 1);

						joyAction.type = JoystickAction::AXIS;
						mLastJoyAction = joyAction.toString();
					}
				}
				// Handle Buttons
				else if (event.type == SDL_JOYBUTTONDOWN)
				{
					joyAction.type = JoystickAction::BUTTON;
					mLastJoyAction = joyAction.toString();
				}

				switch (joyAction.toKeyAction())
				{
					case KeyAction::UP:
						mUp = true;
						break;

					case KeyAction::DOWN:
						mDown = true;
						break;

					case KeyAction::LEFT:
						mLeft = true;
						break;

					case KeyAction::RIGHT:
						mRight = true;
						break;

					case KeyAction::SELECT:
						mLastTextKey = "return";
						mSelect = true;
						break;
					case KeyAction::BACK:
						mExit = true;
						break;
					default:
						break;
				}
				break;
			}

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_FOCUS_LOST:
						mWindowFocus = false;
						break;

					case SDL_WINDOWEVENT_FOCUS_GAINED:
						mWindowFocus = true;
						break;
				}
		}
	}
}


// GUI-Methods
bool InputManager::up() const
{
	return mUp;
}

bool InputManager::down() const
{
	return mDown;
}

bool InputManager::left() const
{
	return mLeft;
}

bool InputManager::right() const
{
	return mRight;
}

bool InputManager::select() const
{
	return mSelect;
}

bool InputManager::exit() const
{
	return mExit;
}

Vector2 InputManager::position()
{
	int windowX = 0;
	int windowY = 0;

	SDL_GetMouseState(&mMouseX, &mMouseY);

	SDL_GetWindowSize(mApp->getWindow(), &windowX, &windowY);
	mMouseX = (int)((((float)mMouseX) * ((float)BASE_RESOLUTION_X)) / windowX);
	mMouseY = (int)((((float)mMouseY) * ((float)BASE_RESOLUTION_Y)) / windowY);

	return Vector2(mMouseX, mMouseY);
}

bool InputManager::click() const
{
	return mClick;
}

bool InputManager::doubleClick() const
{
	return mDoubleClick;
}


bool InputManager::mouseWheelUp() const
{
	return mMouseWheelUp;
}

bool InputManager::mouseWheelDown() const
{
	return mMouseWheelDown;
}

bool InputManager::unclick() const
{
	return mUnclick;
}

bool InputManager::windowFocus() const
{
	return mWindowFocus;
}

bool InputManager::running() const
{
	return mRunning;
}

void InputManager::setEndBlobby()
{
	mRunning = false;
}

std::string InputManager::getLastTextKey()
{
	if (!mLastTextKey.empty())
	{
		return mLastTextKey;
	}
	else
		return "";
}

std::string InputManager::getLastActionKey() const
{
	return SDL_GetKeyName(mLastActionKey);
}

std::string InputManager::getLastJoyAction() const
{
	return mLastJoyAction;
}

void InputManager::captureMouse( bool c )
{
	mMouseCaptured = c;
}

bool InputManager::isMouseCaptured() const
{
	return mMouseCaptured;
}

SDL_Joystick* InputManager::getJoystickById(int joyId)
{
	return mJoystickPool->getJoystick(joyId);
}

void InputManager::setMouseMarker(int target)
{
	mApp->getRenderManager().setMouseMarker(target);
}

bool InputManager::isKeyPressed(SDL_Keycode code) const
{
	const Uint8* keyState = SDL_GetKeyboardState(nullptr);
	return keyState[SDL_GetScancodeFromKey(code)];
}

bool InputManager::isJoyActionActive(const JoystickAction& action) const
{
	SDL_Joystick* joystick = mJoystickPool->getJoystick(action.joyid);

	if (joystick != nullptr)
	{
		switch (action.type)
		{
			case JoystickAction::AXIS:
				if (action.number < 0)
				{
					return (SDL_JoystickGetAxis(joystick, -action.number - 1) < -15000);
				}
				else if (action.number > 0)
				{
					return (SDL_JoystickGetAxis(joystick, action.number - 1) > 15000);
				}
				break;

			case JoystickAction::BUTTON:
				return SDL_JoystickGetButton(joystick, action.number);
			default:
				break;
		}
	}
	return false;
}
