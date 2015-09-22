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

/* header include */
#include "DedicatedServer.h"

/* includes */
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <future>

#include <errno.h>
#include <unistd.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <SDL2/SDL_timer.h>

#include "DedicatedServer.h"
#include "SpeedController.h"
#include "FileSystem.h"
#include "UserConfig.h"
#include "Global.h"

// platform specific
#ifndef WIN32
#include <sys/syslog.h>
#include <sys/wait.h>
#else
#include <cstdarg>
#endif

#if __DESKTOP__
#ifndef WIN32
#include "config.h"
#endif
#endif



/* implementation */

#ifdef WIN32
#undef main

// function for logging to replacing syslog
void syslog(int pri, const char* format, ...);

#endif

static bool g_run_in_foreground = false;
static bool g_print_syslog_to_stderr = false;
static bool g_workaround_memleaks = false;
static std::string g_config_file = "server.xml";
static std::atomic<bool> g_run_server(true); // set this variable to false to stop the server

// ...
void printHelp();
void process_arguments(int argc, char** argv);
void fork_to_background();
void wait_and_restart_child();
void setup_physfs(char* argv0);

// server workload statistics
int SWLS_PacketCount = 0;
int SWLS_Connections = 0;
int SWLS_Games		 = 0;
int SWLS_GameSteps	 = 0;
int SWLS_RunningTime = 0;

const int UPDATE_FREQUENCY = 10;

void main_loop(DedicatedServer& server);

int main(int argc, char** argv)
{
	process_arguments(argc, argv);

	FileSystem fileSys(argv[0]);

	if (!g_run_in_foreground)
	{
		fork_to_background();
	}

	if (g_workaround_memleaks)
	{
		wait_and_restart_child();
	}

	#ifndef WIN32
	int syslog_options = LOG_CONS | LOG_PID | (g_print_syslog_to_stderr ? LOG_PERROR : 0);

	openlog("blobby-server", syslog_options, LOG_DAEMON);
	#endif

	setup_physfs(argv[0]);

	int maxClients = 100;
	std::string rulesFile = DEFAULT_RULES_FILE;
	std::string gameSpeeds = "75";

	UserConfig config;
	try
	{
		config.loadFile(g_config_file);
		maxClients = config.getInteger("maximum_clients");
		rulesFile  = config.getString("rules", DEFAULT_RULES_FILE);
		gameSpeeds = config.getString("speed", gameSpeeds);

		// bring that value into a sane range
		if(maxClients <= 0 || maxClients > 150)
			maxClients = 150;
	}
	catch (std::exception& e)
	{
		syslog(LOG_ERR, "server.xml not found. Falling back to default values.");
	}

	ServerInfo myinfo(config);
	std::vector<std::string> rule_vec;
	boost::algorithm::split(rule_vec, rulesFile, boost::algorithm::is_space(), boost::algorithm::token_compress_on);

	std::vector<std::string> speed_vec_str;
	boost::algorithm::split(speed_vec_str, gameSpeeds, boost::algorithm::is_space(), boost::algorithm::token_compress_on);

	std::vector<float> speed_vec;
	std::transform(speed_vec_str.begin(), speed_vec_str.end(), std::back_inserter(speed_vec), [](const std::string& v ){ return boost::lexical_cast<float>(v);});

	DedicatedServer server(myinfo, rule_vec, speed_vec, maxClients);

	syslog(LOG_NOTICE, "Blobby Volley 2 dedicated server version %i.%i started", BLOBBY_VERSION_MAJOR, BLOBBY_VERSION_MINOR);

	// main loop
	auto serverthread = std::async(std::launch::async, [&](){main_loop(server);});

	while(true)
	{
		std::string command;
		std::cin >> command;

		std::vector<std::string> cmd_vec;
		boost::algorithm::split(cmd_vec, command, boost::algorithm::is_space(), boost::algorithm::token_compress_on);


		if( cmd_vec[0] == "exit" )
		{
			/// \todo check for confirmation if there are still players connected!
			g_run_server = false;
			break;
		} else if ( cmd_vec[0] == "players" )
		{
			server.printAllPlayers(std::cout);
		} else if ( cmd_vec[0] == "games" )
		{
			server.printAllGames(std::cout);
		} else if ( cmd_vec[0] == "status" )
		{
			std::cout << "Blobby Server Status Report " << (SWLS_RunningTime / UPDATE_FREQUENCY / 60 / 60) << "h running \n";
			std::cout << " packet count: " << SWLS_PacketCount << "\n";
			std::cout << " accepted connections: " << SWLS_Connections << "\n";
			std::cout << " started games: " << SWLS_Games << "\n";
			std::cout << " game steps: " << SWLS_GameSteps << "\n";
		}

	}

	syslog(LOG_NOTICE, "Blobby Volley 2 dedicated server shutting down");
	#ifndef WIN32
	closelog();
	#endif
}

// -----------------------------------------------------------------------------------------
//    server main loop function
// ------------------------------
void main_loop( DedicatedServer& server)
{
	int startTime = SDL_GetTicks();
	SpeedController scontroller( UPDATE_FREQUENCY );

	while ( g_run_server )
	{
		// -------------------------------------------------------------------------------
		//  step through all network games and process input - if a game ended, delete it
		// -------------------------------------------------------------------------------

		SWLS_RunningTime++;

		if(SWLS_RunningTime % (UPDATE_FREQUENCY * 60 * 60 /*1h*/) == 0 )
		{
			std::cout << "Blobby Server Status Report " << (SWLS_RunningTime / UPDATE_FREQUENCY / 60 / 60) << "h running \n";
			std::cout << " packet count: " << SWLS_PacketCount << "\n";
			std::cout << " accepted connections: " << SWLS_Connections << "\n";
			std::cout << " started games: " << SWLS_Games << "\n";
			std::cout << " game steps: " << SWLS_GameSteps << "\n";
		}

		server.processPackets();
		server.updateGames();

		scontroller.update();

		if (g_workaround_memleaks)
		{
			// Workaround for memory leak
			// Restart the server after 1 hour if no player is
			// connected
			if ((SDL_GetTicks() - startTime) > 60 * 60 * 1000)
			{
				if (!server.hasActiveGame() && server.getWaitingPlayers() == 0)
				{
					exit(0);
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------------------

void printHelp()
{
	std::cout << "Usage: blobby-server [OPTION...]" << std::endl;
	std::cout << "  -m, --memleak-hack        Workaround memory leaks by restarting regularly" << std::endl;
	std::cout << "  -n, --no-daemon           Don't run as background process" << std::endl;
	std::cout << "  -p, --print-msgs          Print messages to stderr" << std::endl;
	std::cout << "  -c, --config-file <path>  Use custom config file instead of server.xml" << std::endl;
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
			if (strcmp(argv[i], "--config-file") == 0 || strcmp(argv[i], "-c") == 0)
			{
				++i;
				if (i >= argc)
				{
					std::cout << "\"config-file\" option needs an argument" << std::endl;
					printHelp();
					exit(1);
				}
				g_config_file = std::string("server/") + argv[i];
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
	FileSystem& fs = FileSystem::getSingleton();

	#if __DESKTOP__
	#ifndef WIN32
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/rules.zip");
	#endif
	#endif
	fs.addToSearchPath("data");
	fs.addToSearchPath("data" + fs.getDirSeparator() + "rules.zip");
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
