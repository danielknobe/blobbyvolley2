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

#if BLOBBY_ON_DESKTOP
	#ifndef WIN32
		#include "config.h"
	#endif
#endif

#ifdef __SWITCH__
	#include <switch.h>
#endif

#include "RenderManager.h"
#include "SoundManager.h"
#include "InputManager.h"
#include "TextManager.h"
#include "UserConfig.h"
#include "IMGUI.h"
#include "SpeedController.h"
#include "Blood.h"
#include "FileSystem.h"
#include "state/State.h"
#include "BlobbyApp.h"

#if defined(WIN32)
#ifndef GAMEDATADIR
#define GAMEDATADIR "data"
#endif
#endif

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
	#if BLOBBY_ON_DESKTOP
		std::string baseSearchPath("data" + fs.getDirSeparator());
	#else
		std::string baseSearchPath(fs.getBaseDir());
	#endif

	/*
	set write dir
	*/
	std::string writeDir = fs.getPrefDir();
	fs.setWriteDir(writeDir);
	fs.probeDir("replays");
	fs.probeDir("gfx");
	fs.probeDir("sounds");
	fs.probeDir("scripts");
	fs.probeDir("backgrounds");
	fs.probeDir("rules");

	/*
	set read dir (local files next to application)
	*/
	fs.addToSearchPath(baseSearchPath);
	fs.addToSearchPath(baseSearchPath + "gfx.zip");
	fs.addToSearchPath(baseSearchPath + "sounds.zip");
	fs.addToSearchPath(baseSearchPath + "scripts.zip");
	fs.addToSearchPath(baseSearchPath + "backgrounds.zip");
	fs.addToSearchPath(baseSearchPath + "rules.zip");

	/*
	set read dir (unix-like when installed)
	*/
	#if BLOBBY_ON_DESKTOP && (!defined WIN32)
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/gfx.zip");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/sounds.zip");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/scripts.zip");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/backgrounds.zip");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/rules.zip");
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

		SpeedController scontroller(gameConfig.getFloat("gamefps"));
		SpeedController::setMainInstance(&scontroller);
		scontroller.setDrawFPS(gameConfig.getBool("showfps"));

		int running = 1;

		DEBUG_STATUS("starting mainloop");

		BlobbyApp app( std::unique_ptr<State>(new MainMenuState()), gameConfig );

		while (running)
		{
			app.getInputManager().updateInput();
			running = app.getInputManager().running();

			app.getIMGUI().begin();
			app.step();
			//draw FPS:
			static int lastfps = 0;
			static int lastlag = -1;
			if (scontroller.getDrawFPS())
			{
				// We need to ensure that the title bar is only set
				// when the framerate changed, because setting the
				// title can be quite resource intensive on some
				// windows manager, like for example metacity.
				// we only update lag information if lag changed at least by
				// 5 ms, for the same reason.
				int newfps = scontroller.getFPS();
				if (newfps != lastfps || std::abs(CURRENT_NETWORK_LAG - lastlag) > 4)
				{
					std::stringstream tmp;
					tmp << AppTitle << "  FPS: " << newfps;
					if( CURRENT_NETWORK_LAG != -1)
						tmp << "  LAG: " << CURRENT_NETWORK_LAG;
					app.getRenderManager().setTitle(tmp.str());
					lastlag = CURRENT_NETWORK_LAG;
				}
				lastfps = newfps;
			}
			// Dirty workarround for hiding fps in title
			if (!scontroller.getDrawFPS() && (lastfps != -1))
			{
				std::stringstream tmp;
				tmp << AppTitle;
				app.getRenderManager().setTitle(tmp.str());

				lastfps = -1;
			}

			if (!scontroller.doFramedrop())
			{
				app.getIMGUI().end(app.getRenderManager());
				app.getRenderManager().getBlood().step(app.getRenderManager());
				app.getRenderManager().refresh();
			}
			scontroller.update();
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
