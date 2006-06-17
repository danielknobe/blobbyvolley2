#pragma once

#include "Vector.h"
#include "InputSource.h"

struct NetworkStatePacket
{
	int id;
	PhysicState newState;
	int newInputCount;
	PlayerInput newInput[60];
};

struct NetworkInputPacket
{
	int id;
	PlayerInput lastInput;
};

