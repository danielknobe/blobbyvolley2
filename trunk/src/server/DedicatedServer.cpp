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

#include <set>
#include <algorithm>
#include <iostream>

#include <boost/make_shared.hpp>

#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"

#include "NetworkMessage.h"
#include "NetworkGame.h"
#include "GenericIO.h"

#ifndef WIN32
#ifndef __ANDROID__
#include <sys/syslog.h>
#endif
#endif

extern int SWLS_PacketCount;
extern int SWLS_Connections;
extern int SWLS_Games;

void syslog(int pri, const char* format, ...);

DedicatedServer::DedicatedServer(const ServerInfo& info,
								const std::vector<std::string>& rulefiles,
								const std::vector<float>& gamespeeds,
								int max_clients, bool local_server)
: mConnectedClients(0)
, mServer(new RakServer())
, mAcceptNewPlayers(true)
, mPlayerHosted( local_server )
, mServerInfo(info)
{
	if (!mServer->Start(max_clients, 1, mServerInfo.port))
	{
		syslog(LOG_ERR, "Couldn't bind to port %i, exiting", mServerInfo.port);
		throw(2);
	}

	/// \todo this code should be places in ServerInfo
	mMatchMaker.setSendFunction([&](const RakNet::BitStream& stream, PlayerID target){ mServer->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0, target, false); });
	mMatchMaker.setCreateGame([&](boost::shared_ptr<NetworkPlayer> left, boost::shared_ptr<NetworkPlayer> right,
								PlayerSide switchSide, std::string rules, int stw, float sp){
							createGame(left, right, switchSide, rules, stw, sp); });

	// add gamespeeds
	for( auto& s : gamespeeds )
		mMatchMaker.addGameSpeedOption( s );

	// add rules
	for( const auto& f : rulefiles )
		mMatchMaker.addRuleOption( f );

	mServer->setUpdateCallback([this](){ queuePackets(); });
}

DedicatedServer::~DedicatedServer()
{
	mServer->Disconnect(50);
}

void DedicatedServer::queuePackets()
{
	packet_ptr packet;
	while ((packet = mServer->Receive()))
	{
		SWLS_PacketCount++;

		switch(packet->data[0])
		{
			// connection status changes
			case ID_NEW_INCOMING_CONNECTION:
			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_ENTER_SERVER:
			case ID_LOBBY:
			case ID_BLOBBY_SERVER_PRESENT:
			{
				std::lock_guard<std::mutex> lock( mPacketQueueMutex );
				mPacketQueue.push_back( packet );
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
				// disallow player map changes while we sort out packets!
				std::lock_guard<std::mutex> lock( mPlayerMapMutex );

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
			default:
				syslog(LOG_DEBUG, "Unknown packet %d received\n", int(packet->data[0]));
		}
	}
}

void DedicatedServer::processPackets()
{
	while (!mPacketQueue.empty())
	{
		packet_ptr packet;
		{
			std::lock_guard<std::mutex> lock(mPacketQueueMutex);
			packet = mPacketQueue.front();
			mPacketQueue.pop_front();
		}
		SWLS_PacketCount++;

		switch(packet->data[0])
		{
			// connection status changes
			case ID_NEW_INCOMING_CONNECTION:
				mConnectedClients++;
				SWLS_Connections++;
				syslog(LOG_DEBUG, "New incoming connection from %s, %d clients connected now", packet->playerId.toString().c_str(), mConnectedClients);

				if ( !mAcceptNewPlayers )
				{
					RakNet::BitStream stream;
					stream.Write( (char)ID_NO_FREE_INCOMING_CONNECTIONS );
					mServer->Send( &stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);
					mServer->CloseConnection( packet->playerId, true );
					mConnectedClients--;
					syslog(LOG_DEBUG, "Connection not accepted, %d clients connected now", mConnectedClients);
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
					syslog(LOG_DEBUG, "Disconnected player %s", player->second->getName().c_str());
					if( player->second->getGame() )
						player->second->getGame()->injectPacket( packet );

					// no longer count this player as connected. protect this change with a mutex
					{
						std::lock_guard<std::mutex> lock( mPlayerMapMutex );
						mPlayerMap.erase( player );
					}
					mMatchMaker.removePlayer( player->first );
				}
				 else
				{
				}

				int pid = packet->data[0];
				syslog(LOG_DEBUG, "Connection %s closed via %d, %d clients connected now", packet->playerId.toString().c_str(),  pid, mConnectedClients);
				break;
			}
			// player connects to server
			case ID_ENTER_SERVER:
			{
				RakNet::BitStream stream = packet->getStream();

				stream.IgnoreBytes(1);	//ID_ENTER_SERVER

				auto newplayer = boost::make_shared<NetworkPlayer>(packet->playerId, stream);

				// add to player map. protect with mutex
				{
					std::lock_guard<std::mutex> lock( mPlayerMapMutex );
					mPlayerMap[packet->playerId] = newplayer;
				}
				mMatchMaker.addPlayer(packet->playerId, newplayer);
				syslog(LOG_DEBUG, "New player \"%s\" connected from %s ", newplayer->getName().c_str(), packet->playerId.toString().c_str());

				// if this is a locally hosted server, any player that connects automatically joins an
				// open game
				if( mPlayerHosted && mMatchMaker.getOpenGamesCount() != 0)
				{
					RakNet::BitStream stream;
					stream.Write((unsigned char)ID_LOBBY);
					stream.Write((unsigned char)LobbyPacketType::JOIN_GAME);
					stream.Write( mMatchMaker.getOpenGameIDs().front() );
					mMatchMaker.receiveLobbyPacket( packet->playerId, stream );
				}
				break;
			}
			case ID_LOBBY:
			{
				/// \todo assert that the player send an ID_ENTER_SERVER before

				// which player is wanted as opponent
				mMatchMaker.receiveLobbyPacket( packet->playerId, packet->getStream() );
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
	// update new game creation for locally hosted games.
	if( mPlayerHosted )
	{
		mMatchMaker.setAllowNewGames(mMatchMaker.getOpenGamesCount() == 0);
	}

	// this loop ensures that all games that have finished (eg because one
	// player left) still process network packets, to let the other player
	// finalize its interactions (sending replays etc).
	for(auto it = mPlayerMap.begin(); it != mPlayerMap.end(); ++it)
	{
		auto game = it->second->getGame();
		if(game && !game->isGameValid())
		{
			game->processPackets();
		}
	}

	// remove dead games from gamelist
	for (auto iter = mGameList.begin(); iter != mGameList.end();  )
	{
		if (!(*iter)->isGameValid())
		{
			syslog( LOG_DEBUG, "Removed game %s vs %s from gamelist",
					(*iter)->getPlayerID(LEFT_PLAYER).toString().c_str(),
					(*iter)->getPlayerID(RIGHT_PLAYER).toString().c_str()
					);
			iter = mGameList.erase(iter);
		}
		 else
		{
			++iter;
		}
	}
}

bool DedicatedServer::hasActiveGame() const
{
	return !mGameList.empty();
}

int DedicatedServer::getActiveGamesCount() const
{
	return mGameList.size();
}

int DedicatedServer::getWaitingPlayers() const
{
	return mPlayerMap.size() - 2 * mGameList.size();
}

const ServerInfo& DedicatedServer::getServerInfo() const
{
	return mServerInfo;
}

int DedicatedServer::getConnectedClients() const
{
	return mConnectedClients;
}

void DedicatedServer::allowNewPlayers( bool allow )
{
	mAcceptNewPlayers = allow;
}

// debug
void DedicatedServer::printAllPlayers(std::ostream& stream) const
{
	for( std::map< PlayerID, boost::shared_ptr<NetworkPlayer> >::const_iterator it = mPlayerMap.begin();
	     it != mPlayerMap.end();
	     ++it)
	{
		stream << it->second->getID().toString() << " \"" << it->second->getName() << "\" status: ";
		if( it->second->getGame() )
		{
			stream << "playing\n";
		} else
		{
			stream << "waiting\n";
		}
	}
}

void DedicatedServer::printAllGames(std::ostream& stream) const
{
	for( std::list< boost::shared_ptr<NetworkGame> >::const_iterator it = mGameList.begin();
	     it != mGameList.end();
	     ++it)
	{
		stream << (*it)->getPlayerID(LEFT_PLAYER).toString() << " vs " << (*it)->getPlayerID(RIGHT_PLAYER).toString() << "\n";
	}
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

void DedicatedServer::createGame(boost::shared_ptr<NetworkPlayer> left,
								boost::shared_ptr<NetworkPlayer> right,
								PlayerSide switchSide, std::string rules,
								int scoreToWin, float gamespeed)
{
	auto newgame = boost::make_shared<NetworkGame>(*mServer.get(), left, right,
								switchSide, rules, scoreToWin, gamespeed);
	left->setGame( newgame );
	right->setGame( newgame );

	SWLS_Games++;

	/// \todo add some logging?
	syslog(LOG_DEBUG, "Created game \"%s\" vs. \"%s\", rules:%s", left->getName().c_str(), right->getName().c_str(), rules.c_str());
	mGameList.push_back(newgame);
}

