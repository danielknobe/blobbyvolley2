#include "SpeedController.h"

#include <SDL/SDL.h>
#include <limits.h>

SpeedController::SpeedController(float gameFPS, float realFPS)
{
	mGameFPS = gameFPS;
	mRealFPS = realFPS;
	mWaitingTime = 1.0;
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

float SpeedController::getWaitingTime()
{
	return mWaitingTime;
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
	int FPS = 1000;
	if ((SDL_GetTicks()-lastTicks) > 0)
		FPS = 1000 / (SDL_GetTicks()-lastTicks);

	if (FPS > mGameFPS)
		SDL_Delay(rateTicks-(SDL_GetTicks()-lastTicks));
	else if (FPS < mGameFPS)
		mFramedrop = true;

	lastTicks = SDL_GetTicks();

// This is the old correctFramerate-function, please replace it
        /*float rateTicks = 1000.0 / mGameFPS;
        static int frameCount = 0;
        static int lastTicks = SDL_GetTicks();

        int currentTicks;
        int targetTicks;
        ++frameCount;
        currentTicks = SDL_GetTicks();
        targetTicks = (int)(((float)frameCount) * rateTicks) + lastTicks;
        if (currentTicks <= targetTicks)
                SDL_Delay(targetTicks - currentTicks);
        else
        {
                frameCount = 0;
                lastTicks = SDL_GetTicks();
        }*/
}

