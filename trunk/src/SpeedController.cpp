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

#include <algorithm>	// for std::max
#include "SpeedController.h"

#include <SDL/SDL.h>

/// this is required to reduce rounding errors. now we have a resolution of
/// 1�s. This is much better than SDL_Delay can handle, but we prevent 
/// accumulation errors.
const int PRECISION_FACTOR = 1000;

SpeedController* SpeedController::mMainInstance = NULL;

SpeedController::SpeedController(float gameFPS)
{
	mGameFPS = gameFPS;
	mFramedrop = false;
	mDrawFPS = true;
	mFPSCounter = 0;
	mOldTicks = SDL_GetTicks();
	mFPS = 0;
}

SpeedController::~SpeedController()
{
}

void SpeedController::setGameSpeed(float fps)
{
	if (fps < 5)
		fps = 5;
	mGameFPS = fps;
}

bool SpeedController::doFramedrop() const
{
	return mFramedrop;
}

void SpeedController::update()
{
	mFramedrop = false;
	int rateTicks = std::max( static_cast<int>(PRECISION_FACTOR * 1000 / mGameFPS), 1);
	
	static int lastTicks = SDL_GetTicks();
	static unsigned int beginSecond = lastTicks;
	static int counter = 0;

	if (counter == mGameFPS)
	{
		const int delta = SDL_GetTicks() - beginSecond;
		int wait = 1000 - delta;
		if (wait > 0)
			SDL_Delay(wait);
	}
	if (beginSecond + 1000 <= SDL_GetTicks())
	{
		beginSecond = SDL_GetTicks();
		counter = 0;
	}

	const int delta = SDL_GetTicks() - beginSecond;
	if ( (PRECISION_FACTOR * delta) / rateTicks <= counter)
	{
		int wait = ((counter+1)*rateTicks/PRECISION_FACTOR) - delta;
		if (wait > 0)
			SDL_Delay(wait);
	}
	
	// do we need framedrop?
	// if passed time > time when we should have drawn next frame
	// maybe we should limit the number of consecutive framedrops?
	if ( delta * PRECISION_FACTOR > rateTicks * (counter + 1) )
	{
		mFramedrop = true;
	}

	counter++;

	//calculate the FPS of drawn frames:
	if (mDrawFPS)
	{
		if (lastTicks >= mOldTicks + 1000)
		{
			mOldTicks = lastTicks;
			mFPS = mFPSCounter;
			mFPSCounter = 0;
		}

		if (!mFramedrop)
			mFPSCounter++;
	}

	//update for next call:
	lastTicks = SDL_GetTicks();
}


