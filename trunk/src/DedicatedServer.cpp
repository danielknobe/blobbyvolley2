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
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/wait.h>

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

static bool g_run_in_foreground = false;
static bool g_print_syslog_to_stderr = false;
static bool g_workaround_memleaks = false;

void printHelp()
{
	std::cout << "Usage: blobby-server [OPTION...]" << std::endl;
	std::cout << "  -m, --memleak-hack        Workaround memory leaks by restarting regularly" << std::endl;
	std::cout << "  -n, --no-daemon           Don´t run as background process" << std::endl;
	std::cout << "  -p, --print-msgs          Print messages to stderr" << std::endl;
	std::cout << "  -h, --help                This message" << std::endl;
}

void process_arguments(int argc, char** argv)
{
	if (argc > 1)
	{
		for (int i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], "--memleak-hack") == 0 || strcmp(argv[i], "-m") == 0)
			{
				g_workaround_memleaks = true;
				continue;
			}
			if (strcmp(argv[i], "--no-daemon") == 0 || strcmp(argv[i], "-n") == 0)
			{
				g_run_in_foreground = true;
				continue;
			}
			if (strcmp(argv[i], "--print-msgs") == 0 || strcmp(argv[i], "-p") == 0)
			{
				g_print_syslog_to_stderr = true;
				continue;
			}
			if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
			{
				printHelp();
				exit(3);
			}
			std::cout << "Unknown option \"" << argv[i] << "\"" << std::endl;
			printHelp();
			exit(1);
		}
	}
}

void fork_to_background()
{
	pid_t f_return = fork();
	if (f_return == -1)
	{
		perror("fork");
		exit(1);
	}
	if (f_return != 0)
	{
		std::cout << "Running in background as PID " << f_return << std::endl;
		exit(0);
	}

}

void wait_and_restart_child()
{
	pid_t leaking_server;
	while ((leaking_server = fork()) > 0)
	{
		int status;

		// Wait for server to quit and refork
		waitpid(leaking_server, &status, 0);
		// Error will propably occur again
		if (WEXITSTATUS(status) != 0)
		{
			exit(WEXITSTATUS(status));
		}
	}

	if (leaking_server == -1)
	{
		perror("fork");
		exit(1);
	}

}

void setup_physfs(char* argv0)
{
	PHYSFS_init(argv0);
	PHYSFS_addToSearchPath("data", 1);
	std::string userdir = PHYSFS_getUserDir();
	std::string userAppend = ".blobby";
	std::string homedir = userdir + userAppend;
	PHYSFS_addToSearchPath(homedir.c_str(), 0);
	PHYSFS_setWriteDir(homedir.c_str());

}

int main(int argc, char** argv)
{
	process_arguments(argc, argv);
	
	if (!g_run_in_foreground)
	{
		fork_to_background();
	}

	if (g_workaround_memleaks)
	{
		wait_and_restart_child();
	}
	

	int startTime = SDL_GetTicks();

	int syslog_options = LOG_CONS | LOG_PID | (g_print_syslog_to_stderr ? LOG_PERROR : 0);

	openlog("blobby-server", syslog_options, LOG_DAEMON);
	setup_physfs(argv[0]);

	GameList gamelist;
	PlayerMap playermap;
	RakServer server;
	UserConfig config;

	PlayerID firstPlayer;
	PlayerSide firstPlayerSide = NO_PLAYER;
	std::string firstPlayerName;

	config.loadFile("server.xml");

	int port = config.getInteger("port");
	float speed = config.getFloat("speed");
	int clients = 0;

	ServerInfo myinfo(config);

	if (!server.Start(200, 0, 0, port))
	{
		syslog(LOG_ERR, "Couldn´t bind to port %i, exiting", port);
		return 2;
	}

	SpeedController scontroller(speed);

	syslog(LOG_NOTICE, "Blobby Volley 2 dedicated server version %i.%i started", BLOBBY_VERSION_MINOR, BLOBBY_VERSION_MAJOR);

	while (1)
	{
		Packet* packet;
		while ((packet = server.Receive()))
		{
			switch(packet->data[0])
			{
				case ID_NEW_INCOMING_CONNECTION:
					clients++;
					syslog(LOG_DEBUG, "New incoming connection, %d clients connected now", clients);
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
					syslog(LOG_DEBUG, "Connection closed, %d clients connected now", clients);
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
							switchSide);
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
					syslog(LOG_DEBUG, "Unknown packet %d recieved\n", int(packet->data[0]));
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
		scontroller.update();

		if (g_workaround_memleaks)
		{
			// Workaround for memory leak
			// Restart the server after 1 hour if no player is
			// connected
			if ((SDL_GetTicks() - startTime) > 60 * 60 * 1000)
			{
				if (gamelist.empty() && firstPlayerSide == NO_PLAYER)
				{
					exit(0);
				}
			}
		}
		
		usleep(1);
	}
	syslog(LOG_NOTICE, "Blobby Volley 2 dedicated server shutting down");
	closelog();
}
