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
	{
		std::cerr << "Warning: could not find joystick number " << id << "!" << std::endl;
		mJoyMap.erase(id);
	}

	return joy;
}

void JoystickPool::probeJoysticks()
{
	/*
	int numJoysticks = SDL_NumJoysticks();
	SDL_Joystick* lastjoy;
	for(int i = 0; i < numJoysticks; i++)
	{
		lastjoy = SDL_JoystickOpen(i);

		if (lastjoy == NULL)
			continue;

		mJoyMap[SDL_JoystickInstanceID(lastjoy)] = lastjoy;
	}
	*/
}

void JoystickPool::openJoystick(const int joyIndex)
{
	SDL_Joystick* joystickInstance = SDL_JoystickOpen(joyIndex);
	if (joystickInstance != NULL)
	{
		mJoyMap[SDL_JoystickInstanceID(joystickInstance)] = joystickInstance;
	}
}

void JoystickPool::closeJoystick(const int joyId)
{
	SDL_Joystick* joystickInstance = mJoyMap[joyId];
	if (joystickInstance != NULL)
	{
		SDL_JoystickClose(joystickInstance);
	}
	mJoyMap.erase(joyId);
}

void JoystickPool::closeJoysticks()
{
	for (JoyMap::iterator iter = mJoyMap.begin();
		iter != mJoyMap.end(); ++iter)
	{
		SDL_JoystickClose((*iter).second);
		mJoyMap.erase((*iter).first);
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
	{
		typestr = "axis";
	}

	if (type == BUTTON)
	{
		typestr = "button";
	}

	std::stringstream buf;
	buf << "joy_" << joyid << "_" << typestr << "_" << number;
	return buf.str();
}

KeyAction JoystickAction::toKeyAction()
{
	// This is an place holder
	// Just to get simple gamepad support for the GUI
	// TODO: Fix that! Use GUID -> ButtonMapping to get this think cleaned up
	if (type == AXIS)
	{
		// X-Achse?!
		if (abs(number) == 1)
		{

			if (number < 0)
			{
				return KeyAction::LEFT;
			}
			else
			{
				return KeyAction::RIGHT;
			}
		}

		// Y-Achse?!
		if (abs(number) == 2)
		{

			if (number < 0)
			{
				return KeyAction::UP;
			}
			else
			{
				return KeyAction::DOWN;
			}
		}
	}
	
	if (type == BUTTON)
	{
		if(number == 0)
		{
			return KeyAction::SELECT;
		}
		if(number == 1)
		{
			// TODO: This is stupid, we must check if this button is used ingame so that
			// the jumpbutton does not pause the game
			//return JoystickEvent::BACK;
			// We don't allow back at the moment -> game is not leavable
			return KeyAction::NONE;
		}
	}
	return KeyAction::NONE;
}

