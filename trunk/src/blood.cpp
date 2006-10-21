#include "blood.h"
#include "RenderManager.h"
#include "UserConfig.h"
#include <stdlib.h>

BloodManager* BloodManager::mSingleton = NULL;

Blood::Blood(Vector2 position, Vector2 direction)
{
	mPos = position;
	mDir = direction;
}

void Blood::step()
{
	const float GRAVITY = 2.98;
	static int lastFrame = SDL_GetTicks();
	int diff = SDL_GetTicks() - lastFrame;
	RenderManager::getSingleton().drawImage("gfx/blood.bmp", mPos);
	mDir.y += GRAVITY;
	mPos.x += mDir.x;
	mPos.y += mDir.y;
	lastFrame = SDL_GetTicks();
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
		if ((partPos.x < 0) || (partPos.x > 800) || (partPos.y < 0) || (partPos.y > 600))
			mParticles.erase(it2);
	}
}

void BloodManager::spillBlood(Vector2 pos)
{
	for (int c = 0; c <= random(5, 50); c++)
	{
		int x = random(-20, 20);
		int y = random(-50, 3);
		mParticles.push_front(Blood(pos, Vector2(x, y)));
	}
}

int BloodManager::random(int min, int max)
{
	return (int)((double(rand())/RAND_MAX)*(max-min)) + min;
}
