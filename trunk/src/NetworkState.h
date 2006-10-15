#pragma once

#include "State.h"

class NetworkGameState : public State
{
public:
	NetworkGameState(const std::string& servername, Uint16 port);
	virtual ~NetworkGameState();
	virtual void step();
private:
	InputSource* mLocalInput;
	InputSource* mRemoteInput;
	DuelMatch* mMatch;
	RakClient* mClient;
};

