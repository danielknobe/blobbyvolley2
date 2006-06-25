#include <cassert>
#include <SDL/SDL.h>
#include "InputManager.h"

InputManager* InputManager::mSingleton = 0;

InputManager::InputManager()
{
	assert (mSingleton == 0);
	mSingleton = this;
	mRunning = true;
#if defined(__arm__) && defined(linux)
	SDL_JoystickOpen(0);
#endif
}

InputManager* InputManager::getSingleton()
{
	assert(mSingleton);
	return mSingleton;
}

PlayerInput InputManager::getGameInput(int player)
{
	assert (player >= 0 && player < 2);
	return mInput[player];
}

InputManager* InputManager::createInputManager()
{
	return new InputManager();
}

void InputManager::updateInput()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	switch (event.type)
	{
		case SDL_QUIT:
			mRunning = 0;
			break;
		case SDL_JOYBUTTONDOWN:
			mRunning = 0;
		break;
	}
	
	Uint8* keyState = SDL_GetKeyState(0);
	mInput[0] = PlayerInput(keyState[SDLK_a], keyState[SDLK_d], 
		keyState[SDLK_w]);
	mInput[1] = PlayerInput(keyState[SDLK_LEFT],
			keyState[SDLK_RIGHT], keyState[SDLK_UP]);
}

bool InputManager::running()
{
	return mRunning;
}

// Configmethods
void InputManager::writeConfigInFile()
{
	mConfigManager.loadFile("InputConfig.ini");
	mConfigManager.setInteger("asdasd",67);
	mConfigManager.saveFile("InputConfig.ini");
}

void InputManager::loadConfigFromFile()
{
	
}

void InputManager::getCurrentConfig()
{
	
}

void InputManager::setCurrentConfig()
{
	
}
