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

#include <map>
#include <SDL/SDL_events.h>
#include "Global.h"
#include "InputSource.h"

class JoystickPool
{
public:
	static JoystickPool& getSingleton();
	
	SDL_Joystick* getJoystick(int id);
	
	int probeJoysticks();
	void closeJoysticks();
	
private:
	typedef std::map<int, SDL_Joystick*> JoyMap;
	JoyMap mJoyMap;
	static JoystickPool* mSingleton;
};

struct JoystickAction
{
	enum Type
	{
		AXIS,
		BUTTON,
// 	We don't implement these exotic input methods here
//		HAT,
//		TRACKBALL
	};

	JoystickAction(std::string string);
	JoystickAction(int _joyid, Type _type, int _number)
		: type(_type), joy(0), joyid(_joyid),
			number(_number) {}
	~JoystickAction();
	JoystickAction(const JoystickAction& action);

	std::string toString();
	
	Type type;
	
	SDL_Joystick* joy;
	int joyid;
	
	// Note: Axis are stored as the SDL axis +1, so we can used
	// the signedness as direction indication
	int number;
};

class InputDevice
{
public:
	InputDevice() {}
	virtual ~InputDevice() {}

	virtual void transferInput(PlayerInput& mInput) = 0;
};

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
	void transferInput(PlayerInput& input);
};

class KeyboardInputDevice : public InputDevice
{
private:
	SDLKey mLeftKey;
	SDLKey mRightKey;
	SDLKey mJumpKey;
public:
	virtual ~KeyboardInputDevice(){};
	KeyboardInputDevice(SDLKey leftKey, SDLKey rightKey, SDLKey jumpKey);
	void transferInput(PlayerInput& input);
};

class JoystickInputDevice : public InputDevice
{
private:
	bool getAction(const JoystickAction& action);

	JoystickAction mLeftAction;
	JoystickAction mRightAction;
	JoystickAction mJumpAction;
public:
	~JoystickInputDevice() {};
	JoystickInputDevice(JoystickAction laction, JoystickAction raction,
			JoystickAction jaction);

	void transferInput(PlayerInput& input);
};

