#include "JoystickPool.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <sstream>

#include <SDL2/SDL.h>

// Joystick Pool
JoystickPool* JoystickPool::mSingleton = 0; //static

JoystickPool& JoystickPool::getSingleton()
{
	if (mSingleton == 0)
		mSingleton = new JoystickPool();

	return *mSingleton;
}

SDL_Joystick* JoystickPool::getJoystick(int id)
{
	SDL_Joystick* joy =  mJoyMap[id];

	if (!joy)
		std::cerr << "Warning: could not find joystick number "
			<< id << "!" << std::endl;

	return joy;
}

void JoystickPool::probeJoysticks()
{
	int numJoysticks = SDL_NumJoysticks();
	SDL_Joystick* lastjoy;
	for(int i = 0; i < numJoysticks; i++)
	{
		lastjoy = SDL_JoystickOpen(i);

		if (lastjoy == NULL)
			continue;

		mJoyMap[SDL_JoystickInstanceID(lastjoy)] = lastjoy;
	}
}

void JoystickPool::closeJoysticks()
{
	for (JoyMap::iterator iter = mJoyMap.begin();
		iter != mJoyMap.end(); ++iter)
	{
		SDL_JoystickClose((*iter).second);
	}
}

// Joystick Action

JoystickAction::JoystickAction(std::string string)
{
	type = AXIS;
	number = 0;
	joy = 0;
	joyid = 0;
	try
	{
		const char* str = string.c_str();
		if (std::strstr(str, "button"))
		{
			type = BUTTON;
			if (sscanf(str, "joy_%d_button_%d", &joyid, &number) < 2)
				throw string;
		}
		else if (std::strstr(str, "axis"))
		{
			if (sscanf(str, "joy_%d_axis_%d", &joyid, &number) < 2)
				throw string;
		}

		joy = JoystickPool::getSingleton().getJoystick(joyid);
	}
	catch (const std::string& e)
	{
		std::cerr << "Parse error in joystick config: " << e << std::endl;
	}
}

JoystickAction::JoystickAction(const JoystickAction& action)
{
	type = action.type;
	joy = JoystickPool::getSingleton().getJoystick(action.joyid);
	joyid = action.joyid;
	number = action.number;
}

JoystickAction::~JoystickAction()
{
}

std::string JoystickAction::toString()
{
	const char* typestr = "unknown";
	if (type == AXIS)
		typestr = "axis";

	if (type == BUTTON)
		typestr = "button";

	std::stringstream buf;
	buf << "joy_" << joyid << "_" << typestr << "_" << number;
	return buf.str();
}

