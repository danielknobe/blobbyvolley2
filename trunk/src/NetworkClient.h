#pragma once

#include "NetworkPaket.h"

class NetworkServer
{
	friend class NetworkManager;
private:
	NetworkClient();
	
public:
	bool connectionReady();
	
	const PhysicState& getPhysicState();
	void sendInput(PlayerInput input);

};
