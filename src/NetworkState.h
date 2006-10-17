#pragma once

#include "State.h"
#include "PhysicWorld.h"

class RakClient;
class DuelMatch;

class NetworkGameState : public State
{
public:
	NetworkGameState(const std::string& servername, Uint16 port);
	virtual ~NetworkGameState();
	virtual void step();
private:
	enum
	{
		CONNECTING,
		WAITING_FOR_OPPONENT,
		OPPONENT_DISCONNECTED,
		DISCONNECTED,
		CONNECTION_FAILED,
		PLAYING,
		PLAYER_WON,
		PAUSING
	} mNetworkState;


	InputSource* mLocalInput;
	PhysicWorld mPhysicWorld;
	int mLeftScore;
	int mRightScore;
	PlayerSide mServingPlayer;
	
	RakClient* mClient;
	PlayerSide mOwnSide;
	PlayerSide mWinningPlayer;
	
	DuelMatch* mFakeMatch; 	// This hack is necessary to let MouseInputDevice
				// access the necessary game variables
};

