#pragma once

#include "NetworkPacket.h"

class NetworkServer
{
	friend class NetworkManager;
private:
	
	NetworkServer();
public:
	bool connectionReady();

	PlayerInput getLatestInput();
	void setNewState(const PhysicState& pstate, PlayerInput input);
};

