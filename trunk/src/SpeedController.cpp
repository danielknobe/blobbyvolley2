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

#include <iostream>
#include "SpeedController.h"

#include <SDL/SDL.h>

SpeedController* SpeedController::mRenderFPSInstance = NULL;
SpeedController* SpeedController::mCurrentGameFPSInstance = NULL;
float SpeedController::mGameFPS = 60;

SpeedController::SpeedController(float FPS)
{
	mNeededFPS = FPS;	//used for compatibility with renderer's SpeedController
	//mFramedrop = false;
	mDrawFPS = false;
	mFPSCounter = 0;
	int time = SDL_GetTicks();
	mBeginSecond = mOldTicks = mLastTicks = mStartTicks = time;
	mFPS = 0;
	mCounter = 0;
}

SpeedController::~SpeedController()
{
}

void SpeedController::setSpeed(float fps)
{
	if (fps < 1)
		fps = 1;
	mNeededFPS = fps;
}

float SpeedController::getTimeDelta()
{
	return (75.0/1000.0)*(SDL_GetTicks()-mLastTicks);
}

bool SpeedController::beginFrame()
{
	int time = SDL_GetTicks();
	int rateTicks = 1000 / mNeededFPS;
	int ticksDiff = (time-mLastTicks);
	mStartTicks = time;

	if (time >= mBeginSecond + 1000)
	{
		mBeginSecond = time;
		mCounter = 0;
	}

	if (ticksDiff >= rateTicks && mCounter <= mNeededFPS && (time-mBeginSecond)/rateTicks>mCounter)
	{
		if (mDrawFPS)
			if (time >= mOldTicks + 1000)
			{
				mOldTicks = time;
				mFPS = mFPSCounter;
				mFPSCounter = 0;
			}
		return true;
	}
	else
		return false;

	/*

	const int delta = SDL_GetTicks() - beginSecond;
	if (delta / rateTicks <= counter)
	{
		int wait = ((counter+1)*rateTicks) - delta;
		if (wait > 0)
			SDL_Delay(wait);
	}
	else
	{
		if ((lastDrawnFrame+(1000/5)) > SDL_GetTicks()) //should guaranty that at least 5 frames will be drawn per second
		{
			//mFramedrop = true;		//deactivated for alpha 6 release
			//std::cout << "Framedrop" << std::endl;
		}
	}
	counter++;*/

	//calculate the FPS of drawn frames:

	//update for next call:
	//if (!mFramedrop)
		//lastDrawnFrame = lastTicks;
}

void SpeedController::endFrame()
{
	mCounter++;
	mLastTicks = mStartTicks;
	if (mDrawFPS)
		mFPSCounter++;
}

void SpeedController::endPause()
{
	mLastTicks = SDL_GetTicks();
}
