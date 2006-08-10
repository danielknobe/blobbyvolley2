#include <SDL/SDL.h>

#ifdef WIN32
#undef main
#endif 

#include <physfs.h>

#include "RenderManager.h"
#include "SoundManager.h"
#include "InputManager.h"
#include "UserConfig.h"
#include "GUIManager.h"
#include "State.h"
#include "SpeedController.h"

int main(int argc, char* argv[])
{
	PHYSFS_init(argv[0]);
	PHYSFS_addToSearchPath("data", 0);
	PHYSFS_setWriteDir("data");
	
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
	GUIManager::createGUIManager();

	// fullscreen?
	if(gameConfig.getString("fullscreen") == "true")
		rmanager->init(800, 600, true);
	else
		rmanager->init(800, 600, false);
	// Only for Alpha2 Release!!!

	SpeedController scontroller(gameConfig.getFloat("gamefps"),
					gameConfig.getFloat("realfps"));

	SoundManager* smanager = SoundManager::createSoundManager();
	smanager->init();
	smanager->playSound("sounds/bums.wav", 0.0);
	smanager->playSound("sounds/pfiff.wav", 0.0);

	InputManager* inputmgr = InputManager::createInputManager();
		
	int running = 1;
	
	while (running)
	{
		inputmgr->updateInput();
		running = inputmgr->running();
		
		GUIManager::getSingleton()->processInput();
		State::getCurrentState()->step();

		if (!scontroller.doFramedrop())
		{
			rmanager->draw();
			GUIManager::getSingleton()->render();
			rmanager->refresh();
		}
		scontroller.update();
	}
	rmanager->deinit();
	smanager->deinit();
	
	SDL_Quit();
	PHYSFS_deinit();

}
