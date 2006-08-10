#pragma once

#include "InputManager.h"
#include "RenderManager.h"

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
	
	~LocalInputSource()
	{
            RenderManager::getSingleton().setMouseMarker(-6);           
                       }
};

