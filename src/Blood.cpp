/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#include "Blood.h"
#include "RenderManager.h"
#include "UserConfig.h"
#include <stdlib.h>

BloodManager* BloodManager::mSingleton = NULL;

Blood::Blood(Vector2 position, Vector2 direction, int player)
{
	mPos = position;
	mDir = direction;
	mPlayer = player;
	mLastFrame = SDL_GetTicks();
}

void Blood::step()
{
	const float GRAVITY = 3;
	int diff = SDL_GetTicks() - mLastFrame;
	//RenderManager::getSingleton().drawImage("gfx/blood.bmp", mPos);
	RenderManager::getSingleton().drawParticle(mPos, mPlayer);
	const int SPEED = 45; 
	//this calculation is NOT based on physical rules
	mDir.y += GRAVITY / SPEED * diff;
	mPos.x += mDir.x / SPEED * diff;
	mPos.y += mDir.y / SPEED * diff;
	mLastFrame = SDL_GetTicks();
}

BloodManager::BloodManager()
{
	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");
	mEnabled = gameConfig.getBool("blood");
}

void BloodManager::step()
{
	if (!mEnabled)
		return;
	RenderManager::getSingleton().startDrawParticles();
	std::list<Blood>::iterator it = mParticles.begin();
	while (it != mParticles.end())
	{
		std::list<Blood>::iterator it2 = it;
		it++;	
		it2->step();
		Vector2 partPos = it2->getPosition();
		if (partPos.y > 600)
			mParticles.erase(it2);
	}
	RenderManager::getSingleton().endDrawParticles();
}

void BloodManager::spillBlood(Vector2 pos, float intensity, int player)
{
	for (int c = 0; c <= int(intensity*50); c++)
	{
		int x = random(int(-30*intensity), int(30*intensity));
		int y = random(int(-50*intensity), 3);
		mParticles.push_front(Blood(pos, Vector2(x, y), player));
	}
}

int BloodManager::random(int min, int max)
{
	return (int)((double(rand())/RAND_MAX)*(max-min)) + min;
}
