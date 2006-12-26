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

#include "SpeedController.h"

#include <SDL/SDL.h>

SpeedController* SpeedController::mMainInstance = NULL;

SpeedController::SpeedController(float gameFPS)
{
	mGameFPS = gameFPS;
	mFramedrop = false;
	mDrawFPS = true;
	mFPSCounter = 0;
	mOldTicks = SDL_GetTicks();
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
	int ticksDiff = (SDL_GetTicks()-lastTicks);
	static int beginSecond = lastTicks;
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
	if (!mFramedrop)
		lastDrawnFrame = lastTicks;
}
