#include "blood.h"
#include "RenderManager.h"
#include "UserConfig.h"
#include <stdlib.h>

BloodManager* BloodManager::mSingleton = NULL;

Blood::Blood(Vector2 position, Vector2 direction)
{
	mPos = position;
	mDir = direction;
	mLastFrame = SDL_GetTicks();
}

void Blood::step()
{
	const float GRAVITY = 3;
	int diff = SDL_GetTicks() - mLastFrame;
	RenderManager::getSingleton().drawImage("gfx/blood.bmp", mPos);
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
}

void BloodManager::spillBlood(Vector2 pos, float intensity)
{
	for (int c = 0; c <= int(intensity*50); c++)
	{
		int x = random(-30*intensity, 30*intensity);
		int y = random(-50*intensity, 3);
		mParticles.push_front(Blood(pos, Vector2(x, y)));
	}
}

int BloodManager::random(int min, int max)
{
	return (int)((double(rand())/RAND_MAX)*(max-min)) + min;
}
