#pragma once

#include "NetworkPaket.h"
#include "InputSource.h"

class NetworkServer : public InputSource
{
	friend class NetworkManager;
private:
	NetworkClient();
	
public:
	bool connectionReady();
	
	const PhysicState& getPhysicState();
	void sendInput(PlayerInput input);

	virtual PlayerInput getInput();
};
