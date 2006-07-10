#pragma once

#include "InputSource.h"
#include "UserConfig.h"

struct InputKeyMap
{
        const char *keyname;
        SDLKey key;
};

class InputManager
{
private:
	static InputManager* mSingleton;
	
	static InputKeyMap mKeyMap[];	// Type for String <-convert-> SDLKey

	SDLKey mLeftBlobbyLeftMove;
	SDLKey mLeftBlobbyRightMove;
	SDLKey mLeftBlobbyJump;
	SDLKey mRightBlobbyLeftMove;
	SDLKey mRightBlobbyRightMove;
	SDLKey mRightBlobbyJump;
	
	PlayerInput mInput[2];
	bool mRunning;
	UserConfig mConfigManager;
	
	SDL_Joystick* mJoystick;
	
	InputManager();
public:
	static InputManager* createInputManager();
	static InputManager* getSingleton();
	
	bool running();
	PlayerInput getGameInput(int player);
	void updateInput();

	// Configmethods
	
	// EXPERIMENTAL
	std::string keyToString(const SDLKey key);
	SDLKey stringToKey(const std::string& keyname);

	void configFileToCurrentConfig();
	void currentConfigToConfigFile();





};

