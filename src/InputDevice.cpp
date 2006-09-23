#include "InputDevice.h"

JoystickPool* JoystickPool::mSingleton = 0; //static

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

		joy = JoystickPool::getSingleton().getJoystick(joyid);
	}
	catch (std::string e)
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
	char buf[64];
	char* typestr = "unknown";
	if (type == AXIS)
		typestr = "axis";
	if (type == BUTTON)
		typestr = "button";

	snprintf(buf, 64, "joy_%d_%s_%d", joyid, typestr, number);
	return buf;
}

bool JoystickInputDevice::getAction(const JoystickAction& action)
{
	if (action.joy != 0)
	switch (action.type)
	{
		case JoystickAction::AXIS:
			if (action.number < 0)
			{
				if (SDL_JoystickGetAxis(action.joy,
					-action.number - 1) < -15000)
					return true;
			}
			else if (action.number > 0)
			{
				if (SDL_JoystickGetAxis(action.joy,
					action.number - 1) > 15000)
					return true;
			}
			break;
		case JoystickAction::BUTTON:
			if (SDL_JoystickGetButton(action.joy,
						action.number))
				return true;
			break;
	}
	return false;
}

