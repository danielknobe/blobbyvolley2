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

// this global allows the host game thread to be killed
extern std::atomic<bool> gKillHostThread;
extern std::shared_ptr<std::thread> gHostedServerThread;

/* implementation */

void deinit()
{
	SDL_Quit();
}

void setupPHYSFS()
{
	FileSystem& fs = FileSystem::getSingleton();

	// We run these tests out of the build tree, so `data` is where our assets reside.
	std::string baseSearchPath("data");

	// set write dir
	std::string writeDir = fs.getPrefDir();
	fs.setWriteDir(writeDir);
	fs.probeDir("gfx");
	fs.probeDir("sounds");
	fs.probeDir("scripts");
	fs.probeDir("backgrounds");
	fs.probeDir("rules");
	fs.probeDir("replays");

	// set read dir (local files next to application)
	fs.addToSearchPath(baseSearchPath);
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

	try
	{
		UserConfig gameConfig;
		gameConfig.loadFile("config.xml");

		SpeedController scontroller(gameConfig.getFloat("gamefps"));
		SpeedController::setMainInstance(&scontroller);
		scontroller.setDrawFPS(gameConfig.getBool("showfps"));

		int running = 1;

		DEBUG_STATUS("starting mainloop");

		BlobbyApp app( std::make_unique<MainMenuState>(), gameConfig );

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

		// save the replay and back to main menu
		add_key_event(SDLK_ESCAPE);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_RETURN);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_RETURN);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_RETURN);

		// go to replay
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_RETURN);

		// start replay
		add_key_event(SDLK_DOWN);
		add_key_event(SDLK_RETURN);

		// let the replay run, but shorter than the game before
		add_key_event(SDLK_LEFT, 100);

		// end replay and game
		add_key_event(SDLK_ESCAPE);
		add_key_event(SDLK_ESCAPE);
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

			app.getInputManager().updateInput();
			running = app.getInputManager().running();
			app.getIMGUI().begin();
			app.step();
			auto* rmanager = &app.getRenderManager(); //RenderManager may change

			if (!scontroller.doFramedrop())
			{
				app.getIMGUI().end(*rmanager);
				rmanager->getBlood().step(*rmanager);
				rmanager->refresh();
			}
			scontroller.update();
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		SDL_Quit();
		exit (EXIT_FAILURE);
	}
	deinit();
	exit(EXIT_SUCCESS);
}
