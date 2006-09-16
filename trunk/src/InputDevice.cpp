#include "InputDevice.h"

JoystickAction::JoystickAction(std::string string)
{
	type = AXIS;
	number = 0;
	joy = 0;
	joyid = 0;
	try
	{
		const char* str = string.c_str();
		if (strstr(str, "button"))
		{
			type = BUTTON;
			if (sscanf(str, "joy_%d_button_%d", &joyid, &number) < 2)
				throw string;
		}
		else if (strstr(str, "axis"))
		{
			if (sscanf(str, "joy_%d_axis_%d", &joyid, &number) < 2)
				throw string;
		}

		joy = SDL_JoystickOpen(joyid);
	}
	catch (std::string e)
	{
		std::cerr << "Parse error in joystick config: " << e << std::endl;
	}
}

JoystickAction::JoystickAction(const JoystickAction& action)
{
	type = action.type;
	joy = action.joy;
	number = action.number;
	// We open a dummy joystick so that SDL's reference counting works
	SDL_JoystickOpen(joyid);
}

JoystickAction::~JoystickAction()
{
	SDL_JoystickClose(joy);
}

std::string JoystickAction::toString()
{
	char buf[64];
	char* typestr = "unknown";
	if (type == AXIS)
		typestr = "axis";
	if (type == BUTTON)
		typestr = "button";

	snprintf(buf, 64, "joy_%d_%s_%d", joyid, typestr, number);
	return buf;
}
