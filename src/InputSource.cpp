#include "InputSource.h"

std::string& operator<< (std::string& out, const PlayerInput& input)
{
	char buf[4];
	buf[0] = input.left ? 't' : 'f';
	buf[1] = input.right ? 't' : 'f';
	buf[2] = input.up ? 't' : 'f';
	buf[3] = '\0';
	out.append(std::string(buf));
	return out;
}

PlayerInput::PlayerInput(const std::string& string)
{
	left = string.at(0) == 't';
	right = string.at(1) == 't';
	up = string.at(2) == 't';
}
