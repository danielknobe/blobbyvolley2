#pragma once

#include "NetworkPacket.h"
#include "InputSource.h"

class NetworkServer : public InputSource
{
	friend class NetworkManager;
private:
	
	NetworkServer();
public:
	bool connectionReady();

	virtual PlayerInput getInput();
	void setNewState(const PhysicState& pstate, PlayerInput input);
};

