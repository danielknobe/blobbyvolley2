#pragma once

#include "InputSource.h"
#include "UserConfig.h"

class InputManager
{
private:
	static InputManager* mSingleton;

	PlayerInput mInput[2];
	bool mRunning;
	UserConfig mConfigManager;
	
	InputManager();
public:
	static InputManager* createInputManager();
	static InputManager* getSingleton();
	
	// Configmethods
	void writeConfigInFile();
	void loadConfigFromFile();
	void getCurrentConfig();
	void setCurrentConfig();
	
	bool running();
	PlayerInput getGameInput(int player);
	void updateInput();
};

