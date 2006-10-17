#pragma once

#include <queue>

#include "Global.h"
#include "PhysicWorld.h"
#include "raknet/NetworkTypes.h"

class RakServer;

typedef std::queue<Packet> PacketQueue;

class NetworkGame
{
public:
	// The given server is used to send messages to the client, received
	// messages have to bo injected manually in this class.
	// The PlayerID parameters are the IDs of the participating players.
	// The IDs are assumed to be on the same side as they are named.
	// If both players want to be on the same side, switchedSide
	// decides which player is switched.
	NetworkGame(RakServer& server,
			PlayerID leftPlayer, PlayerID rightPlayer,
			PlayerSide switchedSide = NO_PLAYER);

	~NetworkGame();
	
	void injectPacket(Packet* packet);

	// This function processes the queued network packets,
	// makes a physic step, checks the rules and broadcasts
	// the current state and outstanding messages to the clients.
	// It returns whether there are still clients connected.
	bool step();

private:
	void broadcastBitstream(RakNet::BitStream* stream, bool reliable);

	RakServer& mServer;
	PlayerID mLeftPlayer;
	PlayerID mRightPlayer;
	PlayerSide mSwitchedSide;

	PacketQueue mPacketQueue;

	PhysicWorld mPhysicWorld;
	int mLeftScore;
	int mRightScore;
	PlayerSide mServingPlayer;
	PlayerSide mWinningPlayer;
	
	int mLeftHitcount;
	int mRightHitcount;
	int mSquishLeft;
	int mSquishRight;
	
	bool mPausing;
};

