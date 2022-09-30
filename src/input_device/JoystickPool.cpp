#include "JoystickPool.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <sstream>

#include <SDL.h>

#include <InputManager.h>

// Joystick Pool
JoystickPool::~JoystickPool()
{
	for (auto& iter : mJoyMap)
	{
		SDL_JoystickClose(iter.second);
	}
}


SDL_Joystick* JoystickPool::getJoystick(int id)
{
	SDL_Joystick* joy =  mJoyMap[id];

	if (joy == nullptr)
	{
		std::cerr << "Warning: could not find joystick number " << id << "!" << std::endl;
		mJoyMap.erase(id);
	}

	return joy;
}

void JoystickPool::probeJoysticks()
{
#ifdef __SWITCH__
	int numJoysticks = SDL_NumJoysticks();
	SDL_Joystick* lastjoy;
	for(int i = 0; i < numJoysticks; i++)
	{
		lastjoy = SDL_JoystickOpen(i);

		if (lastjoy == NULL)
			continue;

		mJoyMap[SDL_JoystickInstanceID(lastjoy)] = lastjoy;
	}
#endif
}

void JoystickPool::openJoystick(const int joyIndex)
{
	SDL_Joystick* joystickInstance = SDL_JoystickOpen(joyIndex);
	if (joystickInstance != nullptr)
	{
		mJoyMap[SDL_JoystickInstanceID(joystickInstance)] = joystickInstance;
	}
}

void JoystickPool::closeJoystick(const int joyId)
{
	SDL_Joystick* joystickInstance = mJoyMap[joyId];
	if (joystickInstance != nullptr)
	{
		SDL_JoystickClose(joystickInstance);
	}
	mJoyMap.erase(joyId);
}

// Joystick Action

JoystickAction::JoystickAction(std::string string)
{
	type = AXIS;
	number = 0;
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
	}
	catch (const std::string& e)
	{
		std::cerr << "Parse error in joystick config: " << e << std::endl;
	}
}

JoystickAction::~JoystickAction() = default;

std::string JoystickAction::toString() const
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

KeyAction JoystickAction::toKeyAction() const
{
	// This is an place holder
	// Just to get simple gamepad support for the GUI
	// TODO: Fix that! Use GUID -> ButtonMapping to get this think cleaned up
#ifndef __SWITCH__
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
#endif
	
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
#ifdef __SWITCH__
		if(number == 12 || number == 16) 
		{
			return KeyAction::LEFT;
		}
		if(number == 13 || number == 17) 
		{
			return KeyAction::UP;
		}
		if(number == 14 || number == 18) 
		{
			return KeyAction::RIGHT;
		}
		if(number == 15 || number == 19) 
		{
			return KeyAction::DOWN;
		}
		if(number == 10)
		{
			return KeyAction::BACK;
		}
#endif
	}
	return KeyAction::NONE;
}

