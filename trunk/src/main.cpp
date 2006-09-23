#include <SDL/SDL.h>

#ifdef WIN32
#undef main
#endif 

#include <physfs.h>

#include "RenderManager.h"
#include "SoundManager.h"
#include "InputManager.h"
#include "UserConfig.h"
#include "IMGUI.h"
#include "State.h"
#include "SpeedController.h"

#include <cstring>

#if defined(WIN32)
#ifndef GAMEDATADIR
#define GAMEDATADIR "data"
#endif
#endif

void probeDir(const std::string& dirname)
{
	if (PHYSFS_isDirectory(dirname.c_str()) == 0)
	{
		if (PHYSFS_exists(dirname.c_str()))
		{
			PHYSFS_delete(dirname.c_str());
		}
		if (PHYSFS_mkdir(dirname.c_str()))
		{
			std::cout << PHYSFS_getWriteDir() <<
				dirname << " created" << std::endl;
		}
		else
		{
			std::cout << "Warning: Creation of" << 
				PHYSFS_getWriteDir() << dirname <<
				" failed!" << std::endl;
		}
	}
}

void setupPHYSFS()
{
	std::string separator = PHYSFS_getDirSeparator();
	// Game should be playable out of the source package on all 
	// platforms
	PHYSFS_addToSearchPath("data", 1);
	PHYSFS_addToSearchPath("data/gfx.zip", 1);
	PHYSFS_addToSearchPath("data/sounds.zip", 1);
	PHYSFS_addToSearchPath("data/scripts.zip", 1);
#if defined(WIN32)
	// Just write in installation directory
	PHYSFS_setWriteDir("data");
#else
	// Create a search path in the home directory and ensure that
	// all paths exist and are actually directories
	std::string userdir = PHYSFS_getUserDir();
	std::string userAppend = ".blobby";
	std::string homedir = userdir + userAppend;
	PHYSFS_addToSearchPath(userdir.c_str(), 0);
	PHYSFS_setWriteDir(userdir.c_str());
	probeDir(userAppend);
	probeDir(userAppend + separator + "replays");
	probeDir(userAppend + separator + "gfx");
	probeDir(userAppend + separator + "sounds");
	probeDir(userAppend + separator + "scripts");
	PHYSFS_removeFromSearchPath(userdir.c_str());
	PHYSFS_setWriteDir(homedir.c_str());
	PHYSFS_addToSearchPath(homedir.c_str(), 0);
#if defined(GAMEDATADIR)
	// A global installation path makes only sense on non-Windows
	// platforms
	std::string basedir = GAMEDATADIR;
	PHYSFS_addToSearchPath(basedir.c_str(), 1);
	PHYSFS_addToSearchPath((basedir + separator + "gfx.zip").c_str(),
									1);
	PHYSFS_addToSearchPath((basedir + separator + "sounds.zip").c_str(),
									1);
	PHYSFS_addToSearchPath((basedir + separator + "scripts.zip").c_str(),
									1);
#endif
#endif
}

int main(int argc, char* argv[])
{
	PHYSFS_init(argv[0]);
	setupPHYSFS();
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
	srand(SDL_GetTicks());
	// Default is OpenGL and false
	// choose renderer
	RenderManager *rmanager;

	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");
	if(gameConfig.getString("device") == "SDL")
		rmanager = RenderManager::createRenderManagerSDL();
	else if (gameConfig.getString("device") == "GP2X")
		rmanager = RenderManager::createRenderManagerGP2X();
	else if (gameConfig.getString("device") == "OpenGL")
		rmanager = RenderManager::createRenderManagerGL2D();
	else
	{
		std::cerr << "Warning: Unknown renderer selected!";
		std::cerr << "Falling back to OpenGL" << std::endl;
		rmanager = RenderManager::createRenderManagerGL2D();
	}

	// fullscreen?
	if(gameConfig.getString("fullscreen") == "true")
		rmanager->init(800, 600, true);
	else
		rmanager->init(800, 600, false);
	// Only for Alpha2 Release!!!

	SpeedController scontroller(gameConfig.getFloat("gamefps"),
					gameConfig.getFloat("realfps"));
	SpeedController::setMainInstance(&scontroller);

	SoundManager* smanager = SoundManager::createSoundManager();
	smanager->init();
	smanager->setVolume(gameConfig.getFloat("global_volume"));
	smanager->playSound("sounds/bums.wav", 0.0);
	smanager->playSound("sounds/pfiff.wav", 0.0);

	InputManager* inputmgr = InputManager::createInputManager();
		
	int running = 1;
	
	while (running)
	{
		inputmgr->updateInput();
		running = inputmgr->running();

		// This is true by default for compatibility, GUI states may
		// disable it if necessary
		rmanager->drawGame(true);
		IMGUI::getSingleton().begin();
		State::getCurrentState()->step();
		rmanager = &RenderManager::getSingleton(); //RenderManager may change

		if (!scontroller.doFramedrop())
		{
			rmanager->draw();
			IMGUI::getSingleton().end();
			rmanager->refresh();
		}
		scontroller.update();
	}
	rmanager->deinit();
	smanager->deinit();
	
	SDL_Quit();
	PHYSFS_deinit();

}

