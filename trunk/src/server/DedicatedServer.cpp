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

#include <boost/make_shared.hpp>

#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"

#include "NetworkMessage.h"
#include "NetworkGame.h"
#include "GenericIO.h"


extern int SWLS_PacketCount;
extern int SWLS_Connections;
extern int SWLS_Games;
extern int SWLS_GameSteps;

void syslog(int pri, const char* format, ...);

DedicatedServer::DedicatedServer(const ServerInfo& info, const std::string& rulefile, int max_clients) : mConnectedClients(0), mServerInfo(info),
																											mRulesFile(rulefile), mServer( new RakServer() )
{
	if (!mServer->Start(max_clients, 1, mServerInfo.port))
	{
		syslog(LOG_ERR, "Couldn't bind to port %i, exiting", mServerInfo.port);
		throw(2);
	}
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
				break;
			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
			{
				bool cond1 = mWaitingPlayer.valid();
				bool cond2 = mWaitingPlayer.getID() == packet->playerId;
				// if first player disconncted, reset
				if (cond1 && cond2)
					mWaitingPlayer = NetworkPlayer();

				// delete the disconnectiong player
				if ( mPlayerGameMap.find(packet->playerId) != mPlayerGameMap.end() )
				{
					// inject the packet into the game, so the other player can get a notification
					mPlayerGameMap[packet->playerId]->injectPacket(packet);

					// then delete the player
					mPlayerGameMap.erase(packet->playerId);
				}

				// no longer count this player as connected
				mPlayerMap.erase(packet->playerId);

				mConnectedClients--;

				updateLobby();

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
				if (mPlayerGameMap.find(packet->playerId) != mPlayerGameMap.end())
				{
					mPlayerGameMap[packet->playerId]->injectPacket(packet);

				} else {
					syslog(LOG_ERR, "received packet from player not in playerlist!");
				}

				break;

			// player connects to server
			case ID_ENTER_SERVER:
			{
				RakNet::BitStream stream = packet->getStream();

				stream.IgnoreBytes(1);	//ID_ENTER_SERVER

				mPlayerMap[packet->playerId] = NetworkPlayer(packet->playerId, stream);

				// answer by sending the status
				updateLobby();
				break;
/*
				if (!mWaitingPlayer.valid())
				{
					/// \todo does the copy-ctor what i assume it does? deep copy?
					mWaitingPlayer = NetworkPlayer(packet->playerId, stream);
				}
				else // We have two players now
				{
					NetworkPlayer secondPlayer = NetworkPlayer(packet->playerId, stream);

					auto newgame = createGame( mWaitingPlayer, secondPlayer );
					mPlayerGameMap[mWaitingPlayer.getID()] = newgame;
					mPlayerGameMap[secondPlayer.getID()] = newgame;
					mGameList.push_back(newgame);
					mWaitingPlayer = NetworkPlayer();
				}
				break;
*/
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
	for (auto iter = mGameList.begin(); iter != mGameList.end(); ++iter )
	{
		SWLS_GameSteps++;
		if (!(*iter)->step())
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
	broadcastServerStatus();
}

bool DedicatedServer::hasActiveGame() const
{
	return !mGameList.empty();
}

bool DedicatedServer::hasWaitingPlayer() const
{
	return mWaitingPlayer.valid();
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
		if (!mWaitingPlayer.valid())
		{
			mServerInfo.setWaitingPlayer("none");
		}
		else
		{
			mServerInfo.setWaitingPlayer(mWaitingPlayer.getName());
		}

		stream2.Write((unsigned char)ID_BLOBBY_SERVER_PRESENT);
		mServerInfo.writeToBitstream(stream2);
		mServer->Send(&stream2, HIGH_PRIORITY, RELIABLE_ORDERED, 0,	packet->playerId, false);
	}
}

boost::shared_ptr<NetworkGame> DedicatedServer::createGame(NetworkPlayer first, NetworkPlayer second)
{
	PlayerSide switchSide = NO_PLAYER;

	auto leftPlayer = first;
	auto rightPlayer = second;

	// put first player on his desired side in game
	if(RIGHT_PLAYER == first.getDesiredSide())
	{
		std::swap(leftPlayer, rightPlayer);
	}

	// if both players want the same side, one of them is going to get inverted game data
	if (first.getDesiredSide() == second.getDesiredSide())
	{
		// if both wanted to play on the left, the right player is the inverted one, if both wanted right, the left
		if (second.getDesiredSide() == LEFT_PLAYER)
			switchSide = RIGHT_PLAYER;
		if (second.getDesiredSide() == RIGHT_PLAYER)
			switchSide = LEFT_PLAYER;
	}

	auto newgame = boost::make_shared<NetworkGame>(*mServer.get(), leftPlayer.getID(), rightPlayer.getID(), leftPlayer.getName(), rightPlayer.getName(),
													leftPlayer.getColor(), rightPlayer.getColor(), switchSide, mRulesFile);

	SWLS_Games++;

	#ifdef DEBUG
	std::cout 	<< "NEW GAME CREATED:\t"<<leftPlayer.getID().binaryAddress << " : " << leftPlayer.getID().port << "\n"
				<< "\t\t\t" << rightPlayer.getID().binaryAddress << " : " << rightPlayer.getID().port << "\n";
	#endif

	return newgame;
}

void DedicatedServer::broadcastServerStatus()
{
	RakNet::BitStream stream;

	auto out = createGenericWriter(&stream);
	out->byte((unsigned char)ID_SERVER_STATUS);

	std::vector<std::string> playernames;
	for( auto it = mPlayerMap.begin(); it != mPlayerMap.end(); ++it)
	{
		playernames.push_back( it->second.getName() );
	}
	out->generic<std::vector<std::string>>( playernames );

	for( auto it = mPlayerMap.begin(); it != mPlayerMap.end(); ++it)
	{
		mServer->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0, it->first, false);
	}
}
