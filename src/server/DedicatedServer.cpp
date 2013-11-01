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

#include "DedicatedServer.h"

#include <algorithm>

#include <boost/make_shared.hpp>

#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"

#include "NetworkMessage.h"
#include "NetworkGame.h"
#include "GenericIO.h"

#ifndef WIN32
#include <sys/syslog.h>
#endif

extern int SWLS_PacketCount;
extern int SWLS_Connections;
extern int SWLS_Games;
extern int SWLS_GameSteps;

void syslog(int pri, const char* format, ...);

DedicatedServer::DedicatedServer(const ServerInfo& info, const std::string& rulefile, int max_clients)
: mConnectedClients(0)
, mServer(new RakServer())
, mRulesFile(rulefile)
, mAcceptNewPlayers(true)
, mServerInfo(info)
{

	if (!mServer->Start(max_clients, 1, mServerInfo.port))
	{
		syslog(LOG_ERR, "Couldn't bind to port %i, exiting", mServerInfo.port);
		throw(2);
	}

	auto gamelogic = createGameLogic(rulefile, nullptr);

	// set rules data in ServerInfo
	/// \todo this code should be places in ServerInfo
	std::strncpy(mServerInfo.rulestitle, gamelogic->getTitle().c_str(), sizeof(mServerInfo.rulestitle));
	mServerInfo.rulesauthor[sizeof(mServerInfo.rulestitle)-1] = 0;

	std::strncpy(mServerInfo.rulesauthor, gamelogic->getAuthor().c_str(), sizeof(mServerInfo.rulesauthor));
	mServerInfo.rulesauthor[sizeof(mServerInfo.rulesauthor)-1] = 0;
}

DedicatedServer::~DedicatedServer()
{
	mServer->Disconnect(50);
}

void DedicatedServer::processPackets()
{
	packet_ptr packet;
	while ((packet = mServer->Receive()))
	{
		SWLS_PacketCount++;

		switch(packet->data[0])
		{
			// connection status changes
			case ID_NEW_INCOMING_CONNECTION:
				mConnectedClients++;
				SWLS_Connections++;
				syslog(LOG_DEBUG, "New incoming connection, %d clients connected now", mConnectedClients);

				if ( !mAcceptNewPlayers )
				{
					RakNet::BitStream stream;
					stream.Write( (char)ID_NO_FREE_INCOMING_CONNECTIONS );
					mServer->Send( &stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);
					mServer->CloseConnection( packet->playerId, true );
				}

				break;
			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
			{
				mConnectedClients--;

				auto player = mPlayerMap.find(packet->playerId);
				// delete the disconnectiong player
				if( player != mPlayerMap.end() )
				{
					if( player->second->getGame() )
						player->second->getGame()->injectPacket( packet );

					// no longer count this player as connected
					mPlayerMap.erase( player );

					updateLobby();
				}
				syslog(LOG_DEBUG, "Connection closed, %d clients connected now", mConnectedClients);
				break;
			}

			// game progress packets

			case ID_INPUT_UPDATE:
			case ID_PAUSE:
			case ID_UNPAUSE:
			case ID_CHAT_MESSAGE:
			case ID_REPLAY:
			case ID_RULES:
			{
				auto player = mPlayerMap.find(packet->playerId);
				// delete the disconnectiong player
				if( player != mPlayerMap.end() && player->second->getGame() )
				{
					player->second->getGame() ->injectPacket( packet );
				} else {
					syslog(LOG_ERR, "received packet from player not in playerlist!");
				}

				break;
			}

			// player connects to server
			case ID_ENTER_SERVER:
			{
				RakNet::BitStream stream = packet->getStream();

				stream.IgnoreBytes(1);	//ID_ENTER_SERVER

				mPlayerMap[packet->playerId] = boost::make_shared<NetworkPlayer>(packet->playerId, stream);

				// answer by sending the status to all players
				updateLobby();
				break;
			}
			case ID_CHALLENGE:
			{
				/// \todo assert that the player send an ID_ENTER_SERVER before

				// which player is wanted as opponent
				auto stream = packet->getStream();

				// check packet size
				if( stream.GetNumberOfBytesUsed() != 9 )
				{
					syslog(LOG_NOTICE, "faulty ID_ENTER_PACKET received: Expected 9 bytes, got %d", stream.GetNumberOfBytesUsed());
				}

				PlayerID first = packet->playerId;
				PlayerID second = UNASSIGNED_PLAYER_ID;

				auto player = mPlayerMap.find(first);
				if( player == mPlayerMap.end())
				{
					// seems like we did not receive a ENTER_SERVER Packet before.
					syslog( LOG_ERR, "a player tried to enter a game, but has no player info" );
					break;
				}

				auto firstPlayer = player->second;

				auto reader = createGenericReader(&stream);
				unsigned char temp;
				reader->byte(temp);
				reader->generic<PlayerID>(second);

				bool started = false;

				// search if there is an open request
				for(auto it = mGameRequests.begin(); it != mGameRequests.end(); ++it)
				{
					// is this a game request of the player we want to play with, or if we want to play with everyone
					if( it->first == second || second == UNASSIGNED_PLAYER_ID)
					{
						// do they want to play with us or anyone
						if(it->second == first || it->second == UNASSIGNED_PLAYER_ID)
						{
							/// \todo check that these players are still connected and not already playing a game
							if( mPlayerMap.find(it->first) != mPlayerMap.end() && firstPlayer->getGame() == nullptr &&  mPlayerMap[it->first]->getGame() == nullptr )
							{
								// we can create a game now
								auto newgame = createGame( firstPlayer, mPlayerMap[it->first] );
								mGameList.push_back(newgame);

								// remove the game request
								mGameRequests.erase( it );
								started = true;
								break;	// we're done
							}
						}
					}

				}

				if (!started)
				{
					// no match could be found -> add to request list
					mGameRequests[first] = second;

					// send challenge packets
					if( second == UNASSIGNED_PLAYER_ID )
					{
						// challenge everybody
						for(auto it = mPlayerMap.begin(); it != mPlayerMap.end(); ++it)
						{
							if( it->second->getGame() == nullptr )
							{
								RakNet::BitStream stream;
								auto writer = createGenericWriter( &stream );
								writer->byte( ID_CHALLENGE );
								writer->generic<PlayerID> ( first );
								mServer->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0, it->second->getID(), false);
							}
						}
					}
					// challenge only one player
					else
					{
						RakNet::BitStream stream;
						auto writer = createGenericWriter( &stream );
						writer->byte( ID_CHALLENGE );
						writer->generic<PlayerID> ( first );
						mServer->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0, second, false);
					}
				}

				break;
			}
			case ID_BLOBBY_SERVER_PRESENT:
			{
				processBlobbyServerPresent( packet );
				break;
			}
			default:
				syslog(LOG_DEBUG, "Unknown packet %d received\n", int(packet->data[0]));
		}
	}
}


void DedicatedServer::updateGames()
{
	// make sure all ingame packets are processed.

	/// \todo we iterate over all games twice! We should find a way to organize things better.
	for(auto it = mPlayerMap.begin(); it != mPlayerMap.end(); ++it)
	{
		auto game = it->second->getGame();
		if(game)
		{
			game->processPackets();
		}
	}

	for (auto iter = mGameList.begin(); iter != mGameList.end(); ++iter )
	{
		SWLS_GameSteps++;

		(*iter)->step();
		if (!(*iter)->isGameValid())
		{
			iter = mGameList.erase(iter);
			// workarround to prevent increment of
			// past-end-iterator
			if(iter == mGameList.end())
				break;
		}
	}


}

void DedicatedServer::updateLobby()
{
	// remove all invalid game requests
	for( auto it = mGameRequests.begin(); it != mGameRequests.end(); /* no increment, because we have to do that manually in case we erase*/ )
	{
		PlayerID first = it->first;
		PlayerID second = it->second;

		auto firstPlayer = mPlayerMap.find(first);
		// if the first player is no longer available, everything is fine
		if( firstPlayer == mPlayerMap.end() || firstPlayer->second->getGame() != nullptr)
		{
			// left server or is already playing -> remove game requests
			it = mGameRequests.erase(it);
			continue;
		}

		if( second != UNASSIGNED_PLAYER_ID )
		{
			auto secondPlayer = mPlayerMap.find(second);
			if( secondPlayer == mPlayerMap.end() || secondPlayer->second->getGame() != nullptr)
			{
				// left server or is already playing -> remove game requests
				it = mGameRequests.erase(it);

				// if the second player starts a game, or disconnected, no need to keep the first player waiting
				/// \todo this could be done a lot more elegant
				mServer->CloseConnection(firstPlayer->first, true);
				continue;
			}
		}

		// if all still valid, increment iterator
		++it;
	}

	broadcastServerStatus();
}

bool DedicatedServer::hasActiveGame() const
{
	return !mGameList.empty();
}

int DedicatedServer::getWaitingPlayers() const
{
	return mPlayerMap.size() - 2 * mGameList.size();
}

void DedicatedServer::allowNewPlayers( bool allow )
{
	mAcceptNewPlayers = allow;
}

// special packet processing
void DedicatedServer::processBlobbyServerPresent( const packet_ptr& packet)
{
	RakNet::BitStream stream = packet->getStream();

	// If the client knows nothing about versioning, the version is 0.0
	int major = 0;
	int minor = 0;
	bool wrongPackageSize = true;

	// current client has bitSize 72

	if( stream.GetNumberOfBitsUsed() == 72)
	{
		stream.IgnoreBytes(1);	//ID_BLOBBY_SERVER_PRESENT
		stream.Read(major);
		stream.Read(minor);
		wrongPackageSize = false;
	}

	RakNet::BitStream stream2;

	if (wrongPackageSize)
	{
		std::cerr << "outdated client tried to connect! Unable to determine client version due to packet size mismatch : " << stream.GetNumberOfBitsUsed() << "\n" ;
		stream2.Write((unsigned char)ID_VERSION_MISMATCH);
		stream2.Write((int)BLOBBY_VERSION_MAJOR);
		stream2.Write((int)BLOBBY_VERSION_MINOR);
		mServer->Send(&stream2, LOW_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);
	}
	else if (major < BLOBBY_VERSION_MAJOR
		|| (major == BLOBBY_VERSION_MAJOR && minor < BLOBBY_VERSION_MINOR))
	// Check if the packet contains matching version numbers
	{
		stream2.Write((unsigned char)ID_VERSION_MISMATCH);
		stream2.Write((int)BLOBBY_VERSION_MAJOR);
		stream2.Write((int)BLOBBY_VERSION_MINOR);
		mServer->Send(&stream2, LOW_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);
	}
	else
	{
		mServerInfo.activegames = mGameList.size();
		mServerInfo.waitingplayers = mPlayerMap.size() - 2 * mServerInfo.activegames;

		stream2.Write((unsigned char)ID_BLOBBY_SERVER_PRESENT);
		mServerInfo.writeToBitstream(stream2);

		mServer->Send(&stream2, HIGH_PRIORITY, RELIABLE_ORDERED, 0,	packet->playerId, false);
	}
}

boost::shared_ptr<NetworkGame> DedicatedServer::createGame(boost::shared_ptr<NetworkPlayer> first, boost::shared_ptr<NetworkPlayer> second)
{
	PlayerSide switchSide = NO_PLAYER;

	auto leftPlayer = first;
	auto rightPlayer = second;

	// put first player on his desired side in game
	if(RIGHT_PLAYER == first->getDesiredSide())
	{
		std::swap(leftPlayer, rightPlayer);
	}

	// if both players want the same side, one of them is going to get inverted game data
	if (first->getDesiredSide() == second->getDesiredSide())
	{
		// if both wanted to play on the left, the right player is the inverted one, if both wanted right, the left
		if (second->getDesiredSide() == LEFT_PLAYER)
			switchSide = RIGHT_PLAYER;
		if (second->getDesiredSide() == RIGHT_PLAYER)
			switchSide = LEFT_PLAYER;
	}

	auto newgame = boost::make_shared<NetworkGame>(*mServer.get(), leftPlayer, rightPlayer, switchSide, mRulesFile);

	SWLS_Games++;

	first->setGame( newgame );
	second->setGame( newgame );

	/// \todo add some logging?

	return newgame;
}

void DedicatedServer::broadcastServerStatus()
{
	RakNet::BitStream stream;

	auto out = createGenericWriter(&stream);
	out->byte((unsigned char)ID_SERVER_STATUS);




	std::vector<std::string> playernames;
	std::vector<PlayerID> playerIDs;
	for( auto it = mPlayerMap.begin(); it != mPlayerMap.end(); ++it)
	{
		// only send players that are waiting
		if( it->second->getGame() == nullptr )
		{
			playernames.push_back( it->second->getName() );
			playerIDs.push_back( it->second->getID() );
		}
	}
	out->generic<std::vector<std::string>>( playernames );
	out->generic<std::vector<PlayerID>>( playerIDs );

	out->uint32(mGameList.size());

	for( auto it = mPlayerMap.begin(); it != mPlayerMap.end(); ++it)
	{
		if( it->second->getGame() == nullptr)
		{
			mServer->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0, it->first, false);
		}
	}
}

