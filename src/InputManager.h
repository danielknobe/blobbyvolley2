#pragma once

#include "InputSource.h"

class InputManager
{
private:
	static InputManager* mSingleton;

	PlayerInput mInput[2];
	bool mRunning;
	
	InputManager();
public:
	static InputManager* createInputManager();
	static InputManager* getSingleton();

	bool running();
	PlayerInput getGameInput(int player);
	void updateInput();
};

