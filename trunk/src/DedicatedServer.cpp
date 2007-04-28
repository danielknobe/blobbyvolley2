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

#include <stdlib.h>
#include <physfs.h>

#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"
#include "raknet/StringCompressor.h"

#include "DedicatedServer.h"
#include "InputSource.h"
#include "PhysicWorld.h"
#include "NetworkGame.h"
#include "UserConfig.h"
#include "NetworkMessage.h"
#include "SpeedController.h"

#ifdef WIN32
#undef main
#endif

int main(int argc, char** argv)
{
	PHYSFS_init(argv[0]);
	PHYSFS_addToSearchPath("data", 1);
	std::string userdir = PHYSFS_getUserDir();
	std::string userAppend = ".blobby";
	std::string homedir = userdir + userAppend;
	PHYSFS_addToSearchPath(userdir.c_str(), 0);
	PHYSFS_setWriteDir(userdir.c_str());

	GameList gamelist;
	PlayerMap playermap;
	RakServer server;
	UserConfig config;

	PlayerID firstPlayer;
	PlayerSide firstPlayerSide = NO_PLAYER;
	std::string firstPlayerName;
	float gameSpeed = 1.0;

	config.loadFile("server.xml");

	int port = config.getInteger("port");
	int clients = 0;

	float gameFPS = config.getFloat("gamefps");
	SpeedController::setGameFPS(gameFPS);

	ServerInfo myinfo(config);

	if (!server.Start(200, 0, 0, port))
	{
		std::cerr << "blobby-server: Couldn't bind to port " << port;
		std::cerr << " !" << std::endl;
	}

	while (1)
	{
		Packet* packet;
		while (packet = server.Receive())
		{
			switch(packet->data[0])
			{
				case ID_NEW_INCOMING_CONNECTION:
					clients++;
					printf("new connection incoming\n");
					printf("%d clients connected now\n", clients);
					break;
				case ID_CONNECTION_LOST:
				case ID_DISCONNECTION_NOTIFICATION:
				{
					bool cond1 = firstPlayerSide != NO_PLAYER;
					bool cond2 = firstPlayer == packet->playerId;
					if (cond1 && cond2)
						firstPlayerSide = NO_PLAYER;
					if (playermap[packet->playerId])
						playermap[packet->playerId]->injectPacket(packet);
					clients--;
					printf("connection close\n");
					printf("%d clients connected now\n", clients);
					break;
				}
				case ID_INPUT_UPDATE:
				case ID_PAUSE:
				case ID_UNPAUSE:
					if (playermap[packet->playerId])
						playermap[packet->playerId]->injectPacket(packet);
					break;
				case ID_ENTER_GAME:
				{
					int ival;
					RakNet::BitStream stream((char*)packet->data,
							packet->length, false);
					stream.Read(ival);
					stream.Read(ival);
					char charName[16];
					StringCompressor::Instance()->DecodeString(charName, 16, &stream);
					std::string playerName(charName);
					PlayerSide newSide = (PlayerSide)ival;

					if (firstPlayerSide == NO_PLAYER)
					{
						firstPlayer = packet->playerId;
						firstPlayerSide = newSide;
						firstPlayerName = playerName;

						stream.Read(gameSpeed);
						if (gameSpeed < 0.1)
							gameSpeed = 1.0;
					}
					else // We have two players now
					{
						PlayerID leftPlayer =
							LEFT_PLAYER == firstPlayerSide ?
							firstPlayer : packet->playerId;
						PlayerID rightPlayer =
							RIGHT_PLAYER == firstPlayerSide ?
							firstPlayer : packet->playerId;
						PlayerSide switchSide = NO_PLAYER;

						std::string leftPlayerName =
							LEFT_PLAYER == firstPlayerSide ?
							firstPlayerName : playerName;
						std::string rightPlayerName =
							RIGHT_PLAYER == firstPlayerSide ?
							firstPlayerName : playerName;

						if (newSide == firstPlayerSide)
						{
							if (newSide == LEFT_PLAYER)
								switchSide = RIGHT_PLAYER;
							if (newSide == RIGHT_PLAYER)
								switchSide = LEFT_PLAYER;
						}
						NetworkGame* newgame = new NetworkGame(
							server, leftPlayer, rightPlayer,
							leftPlayerName, rightPlayerName,
							gameSpeed, switchSide);
						playermap[leftPlayer] = newgame;
						playermap[rightPlayer] = newgame;
						gamelist.push_back(newgame);

						firstPlayerSide = NO_PLAYER;
					}
					break;
				}
				case ID_PONG:
					break;
				case ID_BLOBBY_SERVER_PRESENT:
				{
					RakNet::BitStream stream((char*)packet->data,
							packet->length, false);

					int ival;
					int major;
					int minor;

					stream.Read(ival);
					stream.Read(major);
					stream.Read(minor);
					if (packet->bitSize != 96)
					// We need special treatment when the client does
					// not know anything about versioning at all.
					{
						major = BLOBBY_VERSION_MAJOR;
						minor = BLOBBY_VERSION_MINOR;

						/*
						ServerInfo oldInfo;
						oldInfo.activegames = 0;
						strncpy(oldInfo.name,
							"Please update your Client!", 32);
						strncpy(oldInfo.waitingplayer, "", 64);
						strncpy(oldInfo.description,
							"Your client is to old to connect "
							"to this server. Get a new version"
							" at                  "
							"blobby.sourceforge.net"
							, 192);

						stream.Reset();
						stream.Write(ID_BLOBBY_SERVER_PRESENT);
						oldInfo.writeToBitstream(stream);
						server.Send(&stream, HIGH_PRIORITY,
							RELIABLE_ORDERED, 0,
							packet->playerId, false);
						*/

					}

					if (major < BLOBBY_VERSION_MAJOR
						|| (major == BLOBBY_VERSION_MINOR && minor < BLOBBY_VERSION_MINOR))
					// Check if the packet contains matching version numbers
					{
						stream.Reset();
						stream.Write(ID_OLD_CLIENT);
						server.Send(&stream, HIGH_PRIORITY,
							RELIABLE_ORDERED, 0, packet->playerId,
							false);
					}
					else if (major != BLOBBY_VERSION_MAJOR ||
							minor != BLOBBY_VERSION_MINOR)
					{
						printf("major: %d minor: %d\n", major, minor);
						stream.Reset();
						stream.Write(ID_UNKNOWN_CLIENT);
						server.Send(&stream, HIGH_PRIORITY,
							RELIABLE_ORDERED, 0, packet->playerId,
							false);
					}
					else
					{
						myinfo.activegames = gamelist.size();
						if (firstPlayerSide == NO_PLAYER)
						{
							strncpy(myinfo.waitingplayer, "none",
								sizeof(myinfo.waitingplayer) - 1);
						}
						else
						{
							strncpy(myinfo.waitingplayer, firstPlayerName.c_str(),
								sizeof(myinfo.waitingplayer) - 1);
						}

						stream.Reset();
						stream.Write(ID_BLOBBY_SERVER_PRESENT);
						myinfo.writeToBitstream(stream);
						server.Send(&stream, HIGH_PRIORITY,
							RELIABLE_ORDERED, 0,
							packet->playerId, false);
					}
					break;
				}
				case ID_RECEIVED_STATIC_DATA:
					break;
				default:
					printf("unknown packet %d recieved\n",
						int(packet->data[0]));
			}
		}
		for (GameList::iterator iter = gamelist.begin(); gamelist.end() != iter; ++iter)
		{
			if (!(*iter)->step())
			{
				PlayerMap::iterator piter = playermap.begin();
				while (piter != playermap.end())
				{
						if (piter->second == *iter)
							playermap.erase(piter);
						++piter;
				}
				delete *iter;
				iter = gamelist.erase(iter);
			}
		}
		server.DeallocatePacket(packet);
	}
}
