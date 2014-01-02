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
#include <iostream>

#include <SDL2/SDL.h>

#ifndef __APPLE__
#ifndef __ANDROID__
#include "config.h"
#endif
#endif

#ifdef __APPLE__
	#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		#include <physfs.h>
	#else
		#include "config.h"
	#endif
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

#if defined(WIN32)
#ifndef GAMEDATADIR
#define GAMEDATADIR "data"
#endif
#endif

#ifdef WIN32
#include <windows.h>
#undef main
#endif

/* implementation */

void deinit()
{
	RenderManager::getSingleton().deinit();
	SoundManager::getSingleton().deinit();
	/// \todo this is more a hack than a real solution
	/// 		how do we make sure our current state
	///			is certainly destructed properly?
	delete State::getCurrentState();
	SDL_Quit();
}

void setupPHYSFS()
{
	FileSystem& fs = FileSystem::getSingleton();
	std::string separator = fs.getDirSeparator();
	// Game should be playable out of the source package on all
	// relevant platforms.
	std::string baseSearchPath("data" + separator);
	// Android and iOS are needing a special path
	#ifdef __ANDROID__
		baseSearchPath = SDL_AndroidGetExternalStoragePath() + separator;
	#endif
	#ifdef __APPLE__
		#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
			baseSearchPath = PHYSFS_getBaseDir();
		#endif
	#endif

	fs.addToSearchPath(baseSearchPath);
	fs.addToSearchPath(baseSearchPath + "gfx.zip");
	fs.addToSearchPath(baseSearchPath + "sounds.zip");
	fs.addToSearchPath(baseSearchPath + "scripts.zip");
	fs.addToSearchPath(baseSearchPath + "backgrounds.zip");
	fs.addToSearchPath(baseSearchPath + "rules.zip");

	#if defined(WIN32)
		// Just write in installation directory
		fs.setWriteDir("data");

		// handle the case when it is installed
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/gfx.zip");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/sounds.zip");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/scripts.zip");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/backgrounds.zip");
		fs.addToSearchPath(BLOBBY_INSTALL_PREFIX  "/share/blobby/rules.zip");
	#else
		#ifndef __ANDROID__
			// Create a search path in the home directory and ensure that
			// all paths exist and are actually directories
			std::string userdir = fs.getUserDir();
			std::string userAppend = ".blobby";
			std::string homedir = userdir + userAppend;
			/// \todo please review this code and determine if we really need to add userdir to serach path
			/// only to remove it later
			fs.setWriteDir(userdir);
			fs.probeDir(userAppend);
			/// \todo why do we need separator here?
			fs.probeDir(userAppend + separator + "replays");
			fs.probeDir(userAppend + separator + "gfx");
			fs.probeDir(userAppend + separator + "sounds");
			fs.probeDir(userAppend + separator + "scripts");
			fs.probeDir(userAppend + separator + "backgrounds");
			fs.probeDir(userAppend + separator + "rules");
			fs.removeFromSearchPath(userdir);
			// here we set the write dir anew!
			fs.setWriteDir(homedir);
			#if defined(GAMEDATADIR)
			{
				// A global installation path makes only sense on non-Windows
				// platforms
				std::string basedir = GAMEDATADIR;
				fs.addToSearchPath(basedir);
				fs.addToSearchPath(basedir + separator + "gfx.zip");
				fs.addToSearchPath(basedir + separator + "sounds.zip");
				fs.addToSearchPath(basedir + separator + "scripts.zip");
				fs.addToSearchPath(basedir + separator + "backgrounds.zip");
				fs.addToSearchPath(basedir + separator + "rules.zip");
			}
			#endif
		#else
			fs.setWriteDir(baseSearchPath);
			fs.probeDir("replays");
			fs.probeDir("gfx");
			fs.probeDir("sounds");
			fs.probeDir("scripts");
			fs.probeDir("backgrounds");
			fs.probeDir("rules");
		#endif

	#endif
}

#ifdef __ANDROID__
	#undef main
	extern "C"
	int SDL_main(int argc, char* argv[])
#elif __APPLE__
	#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		int main(int argc, char* argv[])
	#else
		#undef main
		extern "C"
		int SDL_main(int argc, char* argv[])
	#endif
#else
	#undef main
	extern "C"
	int main(int argc, char* argv[])
#endif
{
	DEBUG_STATUS("started main");

	FileSystem filesys(argv[0]);
	setupPHYSFS();

	DEBUG_STATUS("physfs initialised");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);

	DEBUG_STATUS("SDL initialised");

	atexit(SDL_Quit);
	srand(SDL_GetTicks());
	// Default is OpenGL and false
	// choose renderer
	RenderManager *rmanager = 0;
	SoundManager *smanager = 0;


	// Test Version Startup Warning
	#ifdef TEST_VERSION
	struct tm* ptm;
	time_t time = std::time(0);
	ptm = gmtime ( &time );

	if( ptm->tm_year > (2013-1900) || ptm->tm_mon >= 12 ) {
		#ifdef WIN32
		MessageBox(0, (std::string("This is a test version of ") + AppTitle + " which expired on "
									"1.12.2013. Please visit blobby.sourceforge.net for a newer version").c_str(),
					"TEST VERISON OUTDATED",
					MB_OK);
		#endif
		return -1;
	}

	#ifdef WIN32
	MessageBox(0, (std::string("This is a test version of ") + AppTitle + " for testing only.\n"
								"It might be unstable and/or incompatible to the current release. "
								"Use of this version is limited to 1.12.2013.\nUntil then, "
								"the final version will most likely be released and you should update to that one.\n"
								"Visit blobby.sourceforge.net for more information or bug reporting.").c_str(),
				"TEST VERISON WARNING",
				MB_OK);
	#endif
	#endif

	try
	{
		UserConfig gameConfig;
		gameConfig.loadFile("config.xml");

		TextManager::createTextManager(gameConfig.getString("language"));

		if(gameConfig.getString("device") == "SDL")
			rmanager = RenderManager::createRenderManagerSDL();
		/*else if (gameConfig.getString("device") == "GP2X")
			rmanager = RenderManager::createRenderManagerGP2X();*/
#ifndef __ANDROID__
#ifndef __APPLE__
		else if (gameConfig.getString("device") == "OpenGL")
			rmanager = RenderManager::createRenderManagerGL2D();
		else
		{
			std::cerr << "Warning: Unknown renderer selected!";
			std::cerr << "Falling back to OpenGL" << std::endl;
			rmanager = RenderManager::createRenderManagerGL2D();
		}
#endif
#endif
		
#ifdef __APPLE__
#if !TARGET_OS_IPHONE || !TARGET_IPHONE_SIMULATOR
		else if (gameConfig.getString("device") == "OpenGL")
			rmanager = RenderManager::createRenderManagerGL2D();
		else
		{
			std::cerr << "Warning: Unknown renderer selected!";
			std::cerr << "Falling back to OpenGL" << std::endl;
			rmanager = RenderManager::createRenderManagerGL2D();
		}
#endif
#endif

		// fullscreen?
		if(gameConfig.getString("fullscreen") == "true")
			rmanager->init(800, 600, true);
		else
			rmanager->init(800, 600, false);

		if(gameConfig.getString("show_shadow") == "true")
			rmanager->showShadow(true);
		else
			rmanager->showShadow(false);

		SpeedController scontroller(gameConfig.getFloat("gamefps"));
		SpeedController::setMainInstance(&scontroller);
		scontroller.setDrawFPS(gameConfig.getBool("showfps"));

		smanager = SoundManager::createSoundManager();
		smanager->init();
		smanager->setVolume(gameConfig.getFloat("global_volume"));
		smanager->setMute(gameConfig.getBool("mute"));
		/// \todo play sound is misleading. what we actually want to do is load the sound
		smanager->playSound("sounds/bums.wav", 0.0);
		smanager->playSound("sounds/pfiff.wav", 0.0);

		std::string bg = std::string("backgrounds/") + gameConfig.getString("background");
		if ( FileSystem::getSingleton().exists(bg) )
			rmanager->setBackground(bg);

		InputManager* inputmgr = InputManager::createInputManager();
		int running = 1;

		DEBUG_STATUS("starting mainloop");

		while (running)
		{
			inputmgr->updateInput();
			running = inputmgr->running();

			IMGUI::getSingleton().begin();
			State::getCurrentState()->step();
			rmanager = &RenderManager::getSingleton(); //RenderManager may change
			//draw FPS:
			static int lastfps = 0;
			if (scontroller.getDrawFPS())
			{
				// We need to ensure that the title bar is only set
				// when the framerate changed, because setting the
				// title can ne quite resource intensive on some
				// windows manager, like for example metacity.
				int newfps = scontroller.getFPS();
				if (newfps != lastfps)
				{
					std::stringstream tmp;
					tmp << AppTitle << "	FPS: " << newfps;
					rmanager->setTitle(tmp.str());
				}
				lastfps = newfps;
			}
			// Dirty workarround for hiding fps in title
			if (!scontroller.getDrawFPS() && (lastfps != -1))
			{
				std::stringstream tmp;
				tmp << AppTitle;
				rmanager->setTitle(tmp.str());

				lastfps = -1;
			}

			if (!scontroller.doFramedrop())
			{
				rmanager->draw();
				IMGUI::getSingleton().end();
				BloodManager::getSingleton().step();
				rmanager->refresh();
			}
			scontroller.update();
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		if (rmanager)
			rmanager->deinit();
		if (smanager)
			smanager->deinit();
		SDL_Quit();
		exit (EXIT_FAILURE);
	}

	deinit();
	exit(EXIT_SUCCESS);
}
