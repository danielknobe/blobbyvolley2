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

/* includes */
#include <ctime>
#include <cstring>
#include <sstream>
#include <atomic>
#include <thread>
#include <iostream>

#include <SDL.h>

#include "Global.h"

#ifdef __SWITCH__
	#include <switch.h>
#endif

#include "RenderManager.h"
#include "SoundManager.h"
#include "InputManager.h"
#include "TextManager.h"
#include "UserConfig.h"
#include "IMGUI.h"
#include "Blood.h"
#include "FileSystem.h"
#include "state/State.h"
#include "BlobbyApp.h"
#include "RateController.h"

// this global allows the host game thread to be killed
extern std::atomic<bool> gKillHostThread;
extern std::shared_ptr<std::thread> gHostedServerThread;

/* implementation */

void deinit()
{
	SDL_Quit();

#if (defined __SWITCH__) && (defined DEBUG)
	socketExit();
#endif
}

void setupPHYSFS()
{
	FileSystem& fs = FileSystem::getSingleton();
	
	// Game should be playable out of the source package on all
	// relevant platforms.

	// List all the subdirectories / zip archives that we want to read from
	std::vector<std::string> subdirs = {"gfx", "sounds", "scripts", "backgrounds", "rules"};

	// set write dir
	std::string writeDir = fs.getPrefDir();
	fs.setWriteDir(writeDir);
	for(const auto& sub_dir : subdirs) {
		fs.probeDir(sub_dir);
	}
	fs.probeDir("replays");

	// set read dir (files in datafolder next to working dir)
	{
		std::string dataDir = "data";
		fs.addToSearchPath(dataDir);
		for(const auto& archive : subdirs) {
			fs.addToSearchPath(fs.join(dataDir, archive + ".zip"));
		}
	}

	// set read dir (files next to application)
	{
		std::string dataDir = fs.getBaseDir();
		fs.addToSearchPath(dataDir);
		for(const auto& archive : subdirs) {
			fs.addToSearchPath(fs.join(dataDir, archive + ".zip"));
		}
	}

	// set read dir (files inside MacOS bundle)
	#if (defined BUILD_MACOS_BUNDLE)
	{
		std::string dataDir = fs.join(fs.getBaseDir(), std::string("Contents") + 
		                                               fs.getDirSeparator() +
		                                               std::string("Resources"));
		fs.addToSearchPath(dataDir);
		for(const auto& archive : subdirs) {
			fs.addToSearchPath(fs.join(dataDir, archive + ".zip"));
		}
	}
	#endif

	// set read dir (unix-like when installed)
	#ifdef BLOBBY_DATA_DIR
		fs.addToSearchPath(BLOBBY_DATA_DIR);
		for(const auto& archive : subdirs) {
			fs.addToSearchPath(fs.join(BLOBBY_DATA_DIR, archive + ".zip"));
		}
	#endif
}

int main(int argc, char* argv[])
{
#if (defined __SWITCH__) && (defined DEBUG)
	socketInitializeDefault();   
	nxlinkStdio();
#endif

	DEBUG_STATUS("started main");

	FileSystem filesys(argv[0]);
	setupPHYSFS();

	DEBUG_STATUS("physfs initialised");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);


	DEBUG_STATUS("SDL initialised");

	atexit(SDL_Quit);
	atexit([](){gKillHostThread=true; if(gHostedServerThread) gHostedServerThread->join();});
	srand(SDL_GetTicks());

	// Test Version Startup Warning
	#ifdef TEST_VERSION
	struct tm* ptm;
	time_t time = std::time(0);
	ptm = gmtime ( &time );

	if( ptm->tm_year > (2015-1900) || ptm->tm_mon >= 12 )
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "TEST VERISON OUTDATED",
									(std::string("This is a test version of ") + AppTitle + " which expired on "
									"1.12.2015. Please visit https://blobbyvolley.de for a newer version").c_str(), 0);
		return -1;
	}

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "TEST VERISON WARNING",
								(std::string("This is a test version of ") + AppTitle + " for testing only.\n"
								"It might be unstable and/or incompatible to the current release. "
								"Use of this version is limited to 1.12.2015.\n"
								"Visit https://blobbyvolley.de for more information or bug reporting.").c_str(), 0);
	#endif

	try
	{
		UserConfig gameConfig;
		gameConfig.loadFile("config.xml");

		RateController controller;

		int running = 1;

		DEBUG_STATUS("starting mainloop");

		BlobbyApp app( std::unique_ptr<State>(new MainMenuState()), gameConfig );
		controller.start(144);

		while (running)
		{
			running = app.step();

			controller.handle_next_frame();
			// TODO make sure we do not drop too many consecutive frames
			if(!controller.wants_next_frame())
			{
				app.getIMGUI().end( app.getRenderManager());
				app.getRenderManager().getBlood().step( app.getRenderManager());
				app.getRenderManager().refresh();
			} else {
				std::cout << "FRAME DROP\n";
			}
			while(!controller.wants_next_frame()) {
				std::this_thread::yield();
			}
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", (std::string("An error occurred, blobby will close: ") + e.what()).c_str(), nullptr);
		SDL_Quit();
#if (defined __SWITCH__) && (defined DEBUG)
	socketExit();
#endif
		exit (EXIT_FAILURE);
	}
	deinit();
	exit(EXIT_SUCCESS);
}
