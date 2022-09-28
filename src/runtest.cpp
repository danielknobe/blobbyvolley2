/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2022 Daniel Knobe (daniel-knobe@web.de)
Copyright (C) 2022 Erik Schultheis (erik-schultheis@freenet.de)

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

/*! \file runtest.cpp
 *  \brief Simplified version of blobby main to run a sequence of events for testing.
 */

/* includes */
#include <atomic>
#include <thread>
#include <iostream>

#include <SDL.h>

#include "Global.h"

#include "render/RenderManager.h"
#include "SoundManager.h"
#include "input/InputManager.h"
#include "TextManager.h"
#include "UserConfig.h"
#include "IMGUI.h"
#include "SpeedController.h"
#include "render/Blood.h"
#include "io/FileSystem.h"
#include "state/State.h"
#include "TargetDevice.h"


// this global allows the host game thread to be killed
extern std::atomic<bool> gKillHostThread;
extern std::shared_ptr<std::thread> gHostedServerThread;

/* implementation */

void deinit()
{
	RenderManager::getSingleton().deinit();
	SoundManager::getSingleton().deinit();
	State::deinit();
	SDL_Quit();
}

void setupPHYSFS()
{
	FileSystem& fs = FileSystem::getSingleton();
	const std::string separator = fs.getDirSeparator();
	// Game should be playable out of the source package on all
	// relevant platforms.
	std::string baseSearchPath("data" + separator);

	fs.addToSearchPath(baseSearchPath);
	fs.addToSearchPath(baseSearchPath + "gfx.zip");
	fs.addToSearchPath(baseSearchPath + "sounds.zip");
	fs.addToSearchPath(baseSearchPath + "scripts.zip");
	fs.addToSearchPath(baseSearchPath + "backgrounds.zip");
	fs.addToSearchPath(baseSearchPath + "rules.zip");

	// Create a search path in the home directory and ensure that
	// all paths exist and are actually directories
	// Linux
	std::string userdir = fs.getUserDir();

	std::string userAppend = ".blobby";
	std::string homedir = userdir + userAppend;
	/// \todo please review this code and determine if we really need to add userdir to search path
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
}

extern "C"
int main(int argc, char* argv[])
{
	DEBUG_STATUS("started main");

	FileSystem filesys(argv[0]);
	setupPHYSFS();

	DEBUG_STATUS("physfs initialised");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);


	DEBUG_STATUS("SDL initialised");

	atexit(SDL_Quit);
	atexit([](){gKillHostThread=true; if(gHostedServerThread) gHostedServerThread->join();});
	srand(SDL_GetTicks());
	// Default is OpenGL and false
	// choose renderer
	RenderManager *rmanager = nullptr;
	SoundManager *smanager = nullptr;

	try
	{
		UserConfig gameConfig;
		gameConfig.loadFile("config.xml");

		IMGUI::getSingleton().setTextMgr(gameConfig.getString("language"));

		if(gameConfig.getString("device") == "SDL")
			rmanager = RenderManager::createRenderManagerSDL();
			/*else if (gameConfig.getString("device") == "GP2X")
				rmanager = RenderManager::createRenderManagerGP2X();*/
		else if (gameConfig.getString("device") == "OpenGL")
			rmanager = RenderManager::createRenderManagerGL2D();
		else
		{
			std::cerr << "Warning: Unknown renderer selected!";
			std::cerr << "Falling back to SDL" << std::endl;
			rmanager = RenderManager::createRenderManagerSDL();
		}

		// fullscreen?
		if(gameConfig.getString("fullscreen") == "true")
			rmanager->init(BASE_RESOLUTION_X, BASE_RESOLUTION_Y, true);
		else
			rmanager->init(BASE_RESOLUTION_X, BASE_RESOLUTION_Y, false);

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

		// Default Usage:
		std::vector<SDL_Event> simulation_queue;
		auto add_key_event = [&simulation_queue](SDL_Keycode key, int repeat=1) {
			SDL_Event sdl_event;
			sdl_event.type = SDL_KEYDOWN;
			sdl_event.key.keysym.sym = key;
			for(int i = 0; i < repeat; ++i)
			{
				simulation_queue.push_back( sdl_event );
			}
		};

		// simple test event sequence
		// go to options menu
		add_key_event(SDLK_DOWN, 4);
		add_key_event(SDLK_RETURN);

		// change players to bots -- use one without debug
		// output for clear reports
		add_key_event(SDLK_DOWN, 3);
		add_key_event(SDLK_RIGHT, 2);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_RIGHT, 2);

		// Go to graphics menu
		add_key_event(SDLK_DOWN, 4);
		add_key_event(SDLK_RETURN);

		// activate OpenGL renderer
		add_key_event(SDLK_DOWN, 3);
		add_key_event(SDLK_RETURN);
		add_key_event(SDLK_DOWN, 14);
		add_key_event(SDLK_RETURN);

		// got to graphics menu again
		add_key_event(SDLK_DOWN, 8);
		add_key_event(SDLK_RETURN);

		// activate SDL renderer
		add_key_event(SDLK_DOWN, 4);
		add_key_event(SDLK_RETURN);
		add_key_event(SDLK_DOWN, 13);
		add_key_event(SDLK_RETURN);

		// back to menu and start a game
		add_key_event(SDLK_DOWN, 11);
		add_key_event(SDLK_RETURN);
		add_key_event(SDLK_DOWN, 3);
		add_key_event(SDLK_RETURN);

		// let the bots play: effectively NOPS
		add_key_event(SDLK_LEFT, 200);

		add_key_event(SDLK_ESCAPE);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_RETURN);
		add_key_event(SDLK_ESCAPE);

		int step = 0;
		while (running)
		{
			if(step % 10 == 0) {
				int fake_event = step / 10;
				if(fake_event < simulation_queue.size()) {
					SDL_PushEvent( &simulation_queue[fake_event] );
				}
			}
			++step;

			inputmgr->updateInput();
			running = inputmgr->running();
			IMGUI::getSingleton().begin();
			State::step();
			rmanager = &RenderManager::getSingleton(); //RenderManager may change

			if (!scontroller.doFramedrop())
			{
				rmanager->draw();
				IMGUI::getSingleton().end(*rmanager);
				rmanager->getBlood().step(*rmanager);
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
