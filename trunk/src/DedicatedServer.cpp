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
#include <iostream>
#include <physfs.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <ctime>

#ifndef WIN32
#include <syslog.h>
#include <sys/wait.h>
#else
#include <cstdarg>
#endif

#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"
// We need no stringcompressor only for the names

#include "DedicatedServer.h"
#include "InputSource.h"
#include "PhysicWorld.h"
#include "NetworkGame.h"
#include "UserConfig.h"
#include "NetworkMessage.h"
#include "SpeedController.h"
#include "RakNetPacket.h"

#ifdef WIN32
#undef main

// function for logging to replacing syslog
enum {
	LOG_ERR,
	LOG_NOTICE,
	LOG_DEBUG
};
void syslog(int pri, const char* format, ...);

#endif

static bool g_run_in_foreground = false;
static bool g_print_syslog_to_stderr = false;
static bool g_workaround_memleaks = false;

// ...
void printHelp();
void process_arguments(int argc, char** argv);
void fork_to_background();
void wait_and_restart_child();
void setup_physfs(char* argv0);

// functions for processing certain network packets
void createNewGame();

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

	#ifndef WIN32
	int syslog_options = LOG_CONS | LOG_PID | (g_print_syslog_to_stderr ? LOG_PERROR : 0);

	openlog("blobby-server", syslog_options, LOG_DAEMON);
	#endif
	
	setup_physfs(argv[0]);

	GameList gamelist;
	PlayerMap playermap;
	RakServer server;
	UserConfig config;

	PlayerID firstPlayer;
	PlayerSide firstPlayerSide = NO_PLAYER;
	std::string firstPlayerName;
	Color firstPlayerColor;

	config.loadFile("server.xml");

	int port = config.getInteger("port");
	float speed = config.getFloat("speed");
	int clients = 0;

	ServerInfo myinfo(config);

	if (!server.Start(150, 0, 1, port))
	{
		syslog(LOG_ERR, "Couldn´t bind to port %i, exiting", port);
		return 2;
	}

	SpeedController scontroller(speed);
	SpeedController::setMainInstance(&scontroller);

	syslog(LOG_NOTICE, "Blobby Volley 2 dedicated server version %i.%i started", BLOBBY_VERSION_MAJOR, BLOBBY_VERSION_MINOR);

	packet_ptr packet;

	while (1)
	{
		
		// -------------------------------------------------------------------------------
		// process all incoming packets , probably relay them to responsible network games
		// -------------------------------------------------------------------------------
		
		while ((packet = receivePacket(&server)))
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
					if ( playermap.find(packet->playerId) != playermap.end() ) {
						// inject the packet into the game
						playermap[packet->playerId]->injectPacket(packet);
						
						// then delete the player ...
						// if it was the last player, the game is delete, too.
						// thus, give the game a last chance to process the last
						// input
						
						// check, wether game was delete from this, in this case, process manually 
						if( std::find(gamelist.begin(), gamelist.end(), playermap[packet->playerId]) == gamelist.end())
						{
							playermap[packet->playerId]->step();
						}
						// then delete
						playermap.erase(packet->playerId);
					}
					
					clients--;
					syslog(LOG_DEBUG, "Connection closed, %d clients connected now", clients);
					break;
				}
				case ID_INPUT_UPDATE:
				case ID_PAUSE:
				case ID_UNPAUSE:
				case ID_CHAT_MESSAGE:
				case ID_REPLAY:
					if (playermap.find(packet->playerId) != playermap.end()){
						playermap[packet->playerId]->injectPacket(packet);
						
						// check, wether game was delete from this, in this case, process manually 
						if( std::find(gamelist.begin(), gamelist.end(), playermap[packet->playerId]) == gamelist.end())
						{
							playermap[packet->playerId]->step();
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

					int playerSide;
					stream.Read(playerSide);

					// Read the Playername
					char charName[16];
					stream.Read(charName, sizeof(charName));

					// ensures that charName is null terminated
					charName[sizeof(charName)-1] = '\0';
					
					// read colour data
					int color;
					stream.Read(color);


					std::string playerName(charName);
					PlayerSide newSide = (PlayerSide)playerSide;

					if (firstPlayerSide == NO_PLAYER)
					{
						firstPlayer = packet->playerId;
						firstPlayerSide = newSide;
						firstPlayerName = playerName;
						firstPlayerColor = color;
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
							
						Color leftColor =
							LEFT_PLAYER == firstPlayerSide ?
							firstPlayerColor : color;
						Color rightColor =
							RIGHT_PLAYER == firstPlayerSide ?
							firstPlayerColor : color;

						if (newSide == firstPlayerSide)
						{
							if (newSide == LEFT_PLAYER)
								switchSide = RIGHT_PLAYER;
							if (newSide == RIGHT_PLAYER)
								switchSide = LEFT_PLAYER;
						}
						boost::shared_ptr<NetworkGame> newgame (new NetworkGame(
							server, leftPlayer, rightPlayer,
							leftPlayerName, rightPlayerName,
							leftColor, rightColor,
							switchSide) );
						
						playermap[leftPlayer] = newgame;
						playermap[rightPlayer] = newgame;
						gamelist.push_back(newgame);
						
						#ifdef DEBUG
						std::cout 	<< "NEW GAME CREATED:\t"<<leftPlayer.binaryAddress << " : " << leftPlayer.port << "\n"
									<< "\t\t\t" << rightPlayer.binaryAddress << " : " << rightPlayer.port << "\n";
						#endif			

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
											stream2.Write((unsigned char)ID_UNKNOWN_CLIENT);
											server.Send(&stream2, LOW_PRIORITY,
														RELIABLE_ORDERED, 0, packet->playerId,
														false);
										}
										else if (major < BLOBBY_VERSION_MAJOR
						|| (major == BLOBBY_VERSION_MAJOR && minor < BLOBBY_VERSION_MINOR))
					// Check if the packet contains matching version numbers
					{
						stream2.Write((unsigned char)ID_OLD_CLIENT);
												server.Send(&stream2, LOW_PRIORITY,
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

						stream2.Write((unsigned char)ID_BLOBBY_SERVER_PRESENT);
						myinfo.writeToBitstream(stream2);
						server.Send(&stream2, HIGH_PRIORITY,
							RELIABLE_ORDERED, 0,
							packet->playerId, false);
					}
					break;
				}
				case ID_RECEIVED_STATIC_DATA:
					break;
				default:
					syslog(LOG_DEBUG, "Unknown packet %d received\n", int(packet->data[0]));
			}
		}

		// -------------------------------------------------------------------------------
		// now, step through all network games and process input - if a game ended, delete it
		// -------------------------------------------------------------------------------
		
		for (GameList::iterator iter = gamelist.begin(); gamelist.end() != iter; ++iter)
		{
			if (!(*iter)->step())
			{
				iter = gamelist.erase(iter);
				// workarround to prevent increment of
				// past-end-iterator
				if(iter == gamelist.end())
					break;
			}
		}
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
	}
	syslog(LOG_NOTICE, "Blobby Volley 2 dedicated server shutting down");
	#ifndef WIN32
	closelog();
	#endif
}

// -----------------------------------------------------------------------------------------

void createNewGame()
{
	
}

// -----------------------------------------------------------------------------------------

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
	#ifndef WIN32
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
	#else
	std::cerr<<"fork is not available under windows\n";
	#endif
}


void wait_and_restart_child()
{
	#ifndef WIN32
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
	#else
	std::cerr<<"fork is not available under windows\n";
	#endif

}

void setup_physfs(char* argv0)
{
	PHYSFS_init(argv0);
	PHYSFS_addToSearchPath("data", 1);
	
	#if defined(WIN32)
	// Just write in installation directory
	PHYSFS_setWriteDir("data");
	#else
	std::string userdir = PHYSFS_getUserDir();
	std::string userAppend = ".blobby";
	std::string homedir = userdir + userAppend;
	PHYSFS_addToSearchPath(homedir.c_str(), 0);
	PHYSFS_setWriteDir(homedir.c_str());
	#endif

}


#ifdef WIN32
#undef main

void syslog(int pri, const char* format, ...) 
{
	// first, look where we want to send our message to
	FILE* target = stdout;
	switch(pri)
	{
		case LOG_ERR:
			target = stderr;
			break;
		case LOG_NOTICE:
		case LOG_DEBUG:
			target = stdout;
			break;
	}
	
	// create a string containing date and time
	std::time_t time_v = std::time(0);
	std::tm* time = localtime(&time_v);
	char buffer[128];
	std::strftime(buffer, sizeof(buffer), "%x - %X", time);
	
	// print it
	fprintf(target, "%s: ", buffer);
	
	// now relay the passed arguments and format string to vfprintf for output
	va_list args;
	va_start (args, format);
	vfprintf(target, format, args);
	va_end (args);
	
	// end finish with a newline
	fprintf(target, "\n");
}
#endif
