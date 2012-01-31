/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

/* header include */
#include "SpeedController.h"

/* includes */
#include <algorithm>

#include <SDL/SDL.h>

/* implementation */
/// this is required to reduce rounding errors. now we have a resolution of
/// 1µs. This is much better than SDL_Delay can handle, but we prevent 
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
	mBeginSecond = mOldTicks;
	mCounter = 0;
}

SpeedController::~SpeedController()
{
}

void SpeedController::setGameSpeed(float fps)
{
	if (fps < 5)
		fps = 5;
	mGameFPS = fps;
	
	/// \todo maybe we should reset only if speed changed?
	mBeginSecond = mOldTicks;
	mCounter = 0;
}

bool SpeedController::doFramedrop() const
{
	return mFramedrop;
}

void SpeedController::update()
{
	int rateTicks = std::max( static_cast<int>(PRECISION_FACTOR * 1000 / mGameFPS), 1);
	
	static int lastTicks = SDL_GetTicks();

	if (mCounter == mGameFPS)
	{
		const int delta = SDL_GetTicks() - mBeginSecond;
		int wait = 1000 - delta;
		if (wait > 0)
			SDL_Delay(wait);
	}
	if (mBeginSecond + 1000 <= SDL_GetTicks())
	{
		mBeginSecond = SDL_GetTicks();
		mCounter = 0;
	}

	const int delta = SDL_GetTicks() - mBeginSecond;
	if ( (PRECISION_FACTOR * delta) / rateTicks <= mCounter)
	{
		int wait = ((mCounter+1)*rateTicks/PRECISION_FACTOR) - delta;
		if (wait > 0)
			SDL_Delay(wait);
	}
	
	// do we need framedrop?
	// if passed time > time when we should have drawn next frame
	// maybe we should limit the number of consecutive framedrops?
	// for now: we can't do a framedrop if we did a framedrop last frame
	if ( delta * PRECISION_FACTOR > rateTicks * (mCounter + 1) && !mFramedrop)
	{
		mFramedrop = true;
	} else
		mFramedrop = false;

	mCounter++;

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


