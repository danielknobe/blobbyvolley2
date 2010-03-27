/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#pragma once

#include <list>

#include "Global.h"
#include "PhysicWorld.h"
#include "GameLogic.h"
#include "raknet/NetworkTypes.h"
#include "UserConfig.h"
#include "SpeedController.h"
#include "RakNetPacket.h"

class RakServer;

typedef std::list<packet_ptr> PacketQueue;

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
			std::string leftPlayerName, std::string rightPlayerName,
			Color leftColor, Color rightColor, 
			PlayerSide switchedSide = NO_PLAYER);

	~NetworkGame();

	void injectPacket(const packet_ptr& packet);

	// This function processes the queued network packets,
	// makes a physic step, checks the rules and broadcasts
	// the current state and outstanding messages to the clients.
	// It returns whether there are still clients connected.
	bool step();

private:
	void broadcastBitstream(RakNet::BitStream* stream, RakNet::BitStream* switchedstream);
	void broadcastPhysicState();

	RakServer& mServer;
	PlayerID mLeftPlayer;
	PlayerID mRightPlayer;
	PlayerSide mSwitchedSide;
	std::string mLeftPlayerName;
	std::string mRightPlayerName;

	PacketQueue mPacketQueue;

	PhysicWorld mPhysicWorld;
	GameLogic mLogic;
	PlayerSide mWinningPlayer;


	bool mPausing;

	float mGameSpeed;
	SpeedController* mGameFPSController;
};

