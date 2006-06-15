#pragma once

#include "InputManager.h"

class LocalInputSource : public InputSource
{
private:
	int mPlayer;
public:
	LocalInputSource(int player)
		: mPlayer(player) {}
	virtual PlayerInput getInput()
	{
		return InputManager::getSingleton()->getGameInput(mPlayer);
	}
};

