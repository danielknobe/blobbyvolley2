#pragma once

#include "State.h"
#include "PhysicWorld.h"
#include "NetworkMessage.h"

#include <vector>
#include <list>

class RakClient;
class DuelMatch;

class NetworkSearchState : public State
{
public:
	NetworkSearchState();
	virtual ~NetworkSearchState();

	virtual void step();
private:
	void broadcast();
	
	typedef std::list<RakClient*> ClientList;

	RakClient* mPingClient;

	std::vector<ServerInfo> mScannedServers;
	ClientList mQueryClients;

	int mSelectedServer;

	std::string mEnteredServer;
	int mServerBoxPosition;
};

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
		SERVER_FULL,
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

