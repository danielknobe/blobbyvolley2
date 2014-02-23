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

#include <map>

#include <SDL2/SDL_events.h>

#include "BlobbyDebug.h"

class JoystickPool : public ObjectCounter<JoystickPool>
{
	public:
		static JoystickPool& getSingleton();

		SDL_Joystick* getJoystick(int id);

		void probeJoysticks();
		void closeJoysticks();

	private:
		typedef std::map<int, SDL_Joystick*> JoyMap;
		JoyMap mJoyMap;
		static JoystickPool* mSingleton;
};

struct JoystickAction : public ObjectCounter<JoystickAction>
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
