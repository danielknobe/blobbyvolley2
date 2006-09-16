#pragma once

#include "InputManager.h"
#include "RenderManager.h"

class LocalInputSource : public InputSource
{
private:
	int mPlayer;
public:
	LocalInputSource(PlayerSide player)
		: mPlayer(player)
	{
		 InputManager::getSingleton()->beginGame(player);
	}
	virtual PlayerInput getInput()
	{
		return InputManager::getSingleton()->getGameInput(mPlayer);
	}

	~LocalInputSource()
	{
		RenderManager::getSingleton().setMouseMarker(-6);
		InputManager::getSingleton()->endGame();           
	}
};

