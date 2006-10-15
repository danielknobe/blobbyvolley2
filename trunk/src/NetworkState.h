#pragma once

#include "State.h"
#include "PhysicWorld.h"

class RakClient;

class NetworkGameState : public State
{
public:
	NetworkGameState(const std::string& servername, Uint16 port);
	virtual ~NetworkGameState();
	virtual void step();
private:
	InputSource* mLocalInput;
	PhysicWorld mPhysicWorld;
	int mLeftScore;
	int mRightScore;
	PlayerSide mServingPlayer;
	
	RakClient* mClient;
	PlayerSide mOwnSide;
	bool mInputEnabled;
	PlayerSide mWinningPlayer;
	bool mFailed;
};

