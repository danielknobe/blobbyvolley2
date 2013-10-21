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

#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>

#include "Global.h"
#include "raknet/NetworkTypes.h"
#include "raknet/BitStream.h"
#include "SpeedController.h"
#include "InputSource.h"
#include "RakNetPacket.h"
#include "DuelMatch.h"
#include "BlobbyDebug.h"

class RakServer;
class ReplayRecorder;

typedef std::list<packet_ptr> PacketQueue;

class NetworkGame : public ObjectCounter<NetworkGame>
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
				PlayerSide switchedSide,
				std::string rules);

		~NetworkGame();

		void injectPacket(const packet_ptr& packet);

		// This function processes the queued network packets,
		// makes a physic step, checks the rules and broadcasts
		// the current state and outstanding messages to the clients.
		// It returns whether there are still clients connected.
		bool step();

	private:
		void broadcastBitstream(RakNet::BitStream* stream, RakNet::BitStream* switchedstream);
		void broadcastBitstream(RakNet::BitStream* stream);
		void broadcastPhysicState();
		bool isGameStarted() { return mRulesSent[LEFT_PLAYER] && mRulesSent[RIGHT_PLAYER]; }

		RakServer& mServer;
		PlayerID mLeftPlayer;
		PlayerID mRightPlayer;
		PlayerSide mSwitchedSide;

		PacketQueue mPacketQueue;

		DuelMatch* mMatch;
		boost::shared_ptr<InputSource> mLeftInput;
		boost::shared_ptr<InputSource> mRightInput;
		PlayerSide mWinningPlayer;

		boost::scoped_ptr<ReplayRecorder> mRecorder;

		bool mPausing;

		float mGameSpeed;
		SpeedController* mGameFPSController;

		bool mRulesSent[MAX_PLAYERS];
		int mRulesLength;
		boost::shared_array<char> mRulesString;
};

