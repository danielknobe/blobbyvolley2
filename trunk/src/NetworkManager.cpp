//#include <SDL/SDL_net.h>
#include <cassert>
#include <iostream>

#include "NetworkManager.h"

NetworkManager* NetworkManager::mSingleton = 0;

NetworkManager* NetworkManager::NetworkManager::getSingleton()
{
	assert(mSingleton);
	return mSingleton;
}

NetworkManager* NetworkManager::createNetworkManager()
{
	return new NetworkManager();
}

NetworkManager::NetworkManager()
{
	mSingleton = this;
/*
	if (SDLNet_Init() != 0)
	{
		std::cerr << "SDLNet_Init: " << SDLNet_GetError() << std::endl;
		exit (1);
	}
*/
}

NetworkManager::~NetworkManager()
{
//	SDLNet_Quit();
}

