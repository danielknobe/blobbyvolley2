#include <SDL/SDL.h>

#ifdef WIN32
#undef main
#endif 

#include <physfs.h>

#include "RenderManager.h"
#include "SoundManager.h"
#include "PhysicWorld.h"
#include "InputManager.h"
#include "LocalInputSource.h"
#include "NetworkManager.h"
#include "UserConfig.h"
#include "GUIManager.h"
#include "State.h"

void correctFramerate()
{	int rate = 60;
	float rateTicks = 1000.0 / ((float)rate);
	static int frameCount = 0;
	static int lastTicks = SDL_GetTicks();
	
	int currentTicks;
	int targetTicks;
	++frameCount;
	currentTicks = SDL_GetTicks();
	targetTicks = (int)(((float)frameCount) * rateTicks) + lastTicks;
	if (currentTicks <= targetTicks)
		SDL_Delay(targetTicks - currentTicks);
	else
	{
		frameCount = 0;
		lastTicks = SDL_GetTicks();
	}
}

int main(int argc, char* argv[])
{
	PHYSFS_init(argv[0]);
	PHYSFS_addToSearchPath("data", 0);
	PHYSFS_setWriteDir("data");
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);

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
		rmanager = RenderManager::createRenderManagerGL2D();
	GUIManager::createGUIManager();

	// fullscreen?
	if(gameConfig.getString("fullscreen") == "true")
		rmanager->init(800, 600, true);
	else
		rmanager->init(800, 600, false);
	// Only for Alpha2 Release!!!


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

		rmanager->draw();
		GUIManager::getSingleton()->render();
		rmanager->refresh();
		correctFramerate();

	}
	rmanager->deinit();
	smanager->deinit();
	
	SDL_Quit();
	PHYSFS_deinit();

}
