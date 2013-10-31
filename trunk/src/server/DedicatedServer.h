/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

#include <string>
#include <map>
#include <list>
#include <boost/scoped_ptr.hpp>

#include "NetworkPlayer.h"
#include "NetworkMessage.h"

class RakServer;

// function for logging to replacing syslog
enum {
	LOG_ERR,
	LOG_NOTICE,
	LOG_DEBUG
};

/*! \class DedicatedServer
	\brief manages a blobby server.
	\details Lets the players connect, matches games and processes them.
*/
class DedicatedServer
{
	public:
		/// create a dedicated server with the data specified in info, using the rules from rulesfile, allowing max_clients
		/// simulatanious connections
		/// \todo Maybe two classes for server info: lokal server info for an server, and remote for data sent to client
		DedicatedServer(const ServerInfo& info, const std::string& rulefile, int max_clients);
		~DedicatedServer();

		void processPackets();
		void updateGames();
		void updateLobby();

		bool hasActiveGame() const;
		int getWaitingPlayers() const;

		void allowNewPlayers( bool allow );

	private:
		// packet handling functions / utility functions
		void processBlobbyServerPresent( const packet_ptr& packet );
		// creates a new game with those players
		// does not add the game to the active game list
		boost::shared_ptr<NetworkGame> createGame(boost::shared_ptr<NetworkPlayer> first, boost::shared_ptr<NetworkPlayer> second);
		// broadcasts the current server  status to all waiting clients
		void broadcastServerStatus();

		// member variables
		// number of currently connected clients
		/// \todo is this already counted by raknet?
		unsigned int mConnectedClients;
		// raknet server used
		boost::scoped_ptr<RakServer> mServer;

		// path to rules file
		std::string mRulesFile;
		// true, if new players should be accepted
		bool mAcceptNewPlayers;
		// server info with server config
		ServerInfo mServerInfo;

		// containers for all games and mapping players to their games
		std::list< boost::shared_ptr<NetworkGame> > mGameList;
		std::map< PlayerID, boost::shared_ptr<NetworkPlayer>> mPlayerMap;
		std::map< PlayerID, PlayerID> mGameRequests;
};
