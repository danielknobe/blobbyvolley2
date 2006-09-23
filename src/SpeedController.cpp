#include "SpeedController.h"

#include <SDL/SDL.h>

SpeedController* SpeedController::mMainInstance = NULL;

SpeedController::SpeedController(float gameFPS, float realFPS)
{
	mGameFPS = gameFPS;
	mRealFPS = realFPS;
	mFramedrop = false;
}

SpeedController::~SpeedController()
{
}

void SpeedController::setGameSpeed(float fps)
{
	mGameFPS = fps;
}

void SpeedController::setRealSpeed(float fps)
{
	mRealFPS = fps;
}

bool SpeedController::doFramedrop()
{
	return mFramedrop;
}

void SpeedController::update()
{
	mFramedrop = false;
	int rateTicks = 1000 / mGameFPS;
	static int lastTicks = SDL_GetTicks();
	static int lastDrawnFrame = lastTicks;
	int FPS = 1000;
	int ticksDiff = (SDL_GetTicks()-lastTicks);
	if (ticksDiff > 0)
		FPS = 1000 / ticksDiff;

	if (FPS > mGameFPS)
	{
		if ((rateTicks-ticksDiff) > 0)
			SDL_Delay(rateTicks-ticksDiff);
	}
	else if (FPS < mGameFPS)
		if ((lastDrawnFrame+(1000/5)) > SDL_GetTicks()) //should guaranty that at least 5 frames will be drawn per second
			mFramedrop = true;

	lastTicks = SDL_GetTicks();
	if (!mFramedrop)
		lastDrawnFrame = lastTicks;
}
