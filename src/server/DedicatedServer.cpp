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
#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"
#include "NetworkMessage.h"
#include "NetworkGame.h"

extern int SWLS_PacketCount;
extern int SWLS_Connections;
extern int SWLS_Games;
extern int SWLS_GameSteps;

void syslog(int pri, const char* format, ...);

DedicatedServer::DedicatedServer(const ServerInfo& info, const std::string& rulefile, int max_clients) : mConnectedClients(0), mServerInfo(info), mRulesFile(rulefile), mServer( new RakServer() )
{
	if (!mServer->Start(max_clients, 1, mServerInfo.port))
	{
		syslog(LOG_ERR, "Couldn't bind to port %i, exiting", mServerInfo.port);
		throw(2);
	}
}

DedicatedServer::~DedicatedServer()
{

}

void DedicatedServer::processPackets()
{
	packet_ptr packet;
	while ((packet = mServer->Receive()))
	{
		SWLS_PacketCount++;

		switch(packet->data[0])
		{
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
					/// \todo what are we doing here???
					/// seems not a good idea to let injectPacket remove the game from the game list...
					/// maybe we should add a centralized way to delete unused games  and players!
					// inject the packet into the game
					/// strange, injectPacket just pushes the packet into a queue. That cannot delete
					/// the game???
					mPlayerGameMap[packet->playerId]->injectPacket(packet);

					// if it was the last player, the game is removed from the game list.
					// thus, give the game a last chance to process the last
					// input

					// check, wether game was removed from game list (not a good idea!), in that case, process manually
					if( std::find(mGameList.begin(), mGameList.end(), mPlayerGameMap[packet->playerId]) == mGameList.end())
					{
						mPlayerGameMap[packet->playerId]->step();
					}

					// then delete the player
					mPlayerGameMap.erase(packet->playerId);
				}

				mConnectedClients--;
				syslog(LOG_DEBUG, "Connection closed, %d clients connected now", mConnectedClients);
				break;
			}
			case ID_INPUT_UPDATE:
			case ID_PAUSE:
			case ID_UNPAUSE:
			case ID_CHAT_MESSAGE:
			case ID_REPLAY:
			case ID_RULES:
				if (mPlayerGameMap.find(packet->playerId) != mPlayerGameMap.end()){
					mPlayerGameMap[packet->playerId]->injectPacket(packet);

					// check, wether game was delete from this, in this case, process manually
					/// \todo here again, injectPacket is not able to delete the game. So, what are we doing here?
					if( std::find(mGameList.begin(), mGameList.end(), mPlayerGameMap[packet->playerId]) == mGameList.end())
					{
						mPlayerGameMap[packet->playerId]->step();
					}

				} else {
					syslog(LOG_ERR, "player not found!");
					#ifdef DEBUG
					std::cout	<< " received game packet for no longer existing game! "
								<< (int)packet->data[0] << " - "
								<< packet->playerId.binaryAddress << " : " << packet->playerId.port
								<< "\n";
					// only quit in debug mode as this is not a problem endangering the stability
					// of the running server, but a situation that should never occur.
					return 3;
					#endif
				}

				break;
			case ID_ENTER_GAME:
			{
				RakNet::BitStream stream((char*)packet->data,
						packet->length, false);

				stream.IgnoreBytes(1);	//ID_ENTER_GAME

				if (!mWaitingPlayer.valid())
				{
					/// \todo does the copy-ctor what i assume it does? deep copy?
					mWaitingPlayer = NetworkPlayer(packet->playerId, stream);
				}
				else // We have two players now
				{
					NetworkPlayer secondPlayer = NetworkPlayer(packet->playerId, stream);
					/// \todo refactor this, this code is awful!
					///  one swap should be enough

					NetworkPlayer leftPlayer = mWaitingPlayer;
					NetworkPlayer rightPlayer = secondPlayer;
					PlayerSide switchSide = NO_PLAYER;

					if(RIGHT_PLAYER == mWaitingPlayer.getDesiredSide())
					{
						std::swap(leftPlayer, rightPlayer);
					}
					if (secondPlayer.getDesiredSide() == mWaitingPlayer.getDesiredSide())
					{
						if (secondPlayer.getDesiredSide() == LEFT_PLAYER)
							switchSide = RIGHT_PLAYER;
						if (secondPlayer.getDesiredSide() == RIGHT_PLAYER)
							switchSide = LEFT_PLAYER;
					}

					boost::shared_ptr<NetworkGame> newgame (new NetworkGame(
						*mServer.get(), leftPlayer.getID(), rightPlayer.getID(),
						leftPlayer.getName(), rightPlayer.getName(),
						leftPlayer.getColor(), rightPlayer.getColor(),
						switchSide, mRulesFile) );

					mPlayerGameMap[leftPlayer.getID()] = newgame;
					mPlayerGameMap[rightPlayer.getID()] = newgame;
					mGameList.push_back(newgame);
					SWLS_Games++;

					#ifdef DEBUG
					std::cout 	<< "NEW GAME CREATED:\t"<<leftPlayer.getID().binaryAddress << " : " << leftPlayer.getID().port << "\n"
								<< "\t\t\t" << rightPlayer.getID().binaryAddress << " : " << rightPlayer.getID().port << "\n";
					#endif

					mWaitingPlayer = NetworkPlayer();
				}
				break;
			}
			case ID_PONG:
				break;
			case ID_BLOBBY_SERVER_PRESENT:
			{
				RakNet::BitStream stream((char*)packet->data,
						packet->length, false);

				// If the client knows nothing about versioning, the version is 0.0
				int major = 0;
				int minor = 0;
				bool wrongPackageSize = true;

				// actuel client has bytesize 72

				if(packet->bitSize == 72)
				{
					stream.IgnoreBytes(1);	//ID_BLOBBY_SERVER_PRESENT
					stream.Read(major);
					stream.Read(minor);
					wrongPackageSize = false;
				}

				RakNet::BitStream stream2;

				if (wrongPackageSize)
				{
					printf("major: %d minor: %d\n", major, minor);
					stream2.Write((unsigned char)ID_VERSION_MISMATCH);
					stream2.Write((int)BLOBBY_VERSION_MAJOR);
					stream2.Write((int)BLOBBY_VERSION_MINOR);
					mServer->Send(&stream2, LOW_PRIORITY,
								RELIABLE_ORDERED, 0, packet->playerId,
								false);
				}
				else if (major < BLOBBY_VERSION_MAJOR
					|| (major == BLOBBY_VERSION_MAJOR && minor < BLOBBY_VERSION_MINOR))
				// Check if the packet contains matching version numbers
				{
					stream2.Write((unsigned char)ID_VERSION_MISMATCH);
					stream2.Write((int)BLOBBY_VERSION_MAJOR);
					stream2.Write((int)BLOBBY_VERSION_MINOR);
					mServer->Send(&stream2, LOW_PRIORITY,
						RELIABLE_ORDERED, 0, packet->playerId,
						false);
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
					mServer->Send(&stream2, HIGH_PRIORITY,
						RELIABLE_ORDERED, 0,
						packet->playerId, false);
				}
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

bool DedicatedServer::hasActiveGame() const
{
	return !mGameList.empty();
}

bool DedicatedServer::hasWaitingPlayer() const
{
	return mWaitingPlayer.valid();
}
