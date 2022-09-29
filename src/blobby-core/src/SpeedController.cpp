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
#include <thread>

/* implementation */
/// this is required to reduce rounding errors. now we have a resolution of
/// 1�s. This is much better than SDL_Delay can handle, but we prevent 
/// accumulation errors.
const int PRECISION_FACTOR = 1000;

SpeedController* SpeedController::mMainInstance = nullptr;

SpeedController::SpeedController(float gameFPS)
{
	mTargetFPS = gameFPS;
	mFramedrop = false;
	mDrawFPS = true;
	mFPSCounter = 0;
	mOldTicks = clock_t::now();
	mFPS = 0;
	mBeginSecond = mOldTicks;
	mCounter = 0;
}

SpeedController::~SpeedController() = default;

void SpeedController::setTargetFPS(float fps)
{
	if (fps < 5)
		fps = 5;
	mTargetFPS = fps;
	
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
	std::chrono::microseconds rate_ticks{std::max( static_cast<int>(1000000 / mTargetFPS), 1)};

	if ( mCounter == mTargetFPS)
	{
		auto delta = clock_t::now() - mBeginSecond;
		auto wait = std::chrono::seconds{1} - delta;
		if (wait.count() > 0)
		{
			std::this_thread::sleep_for(wait);
		}
	}
	if (mBeginSecond + std::chrono::seconds{1} <= clock_t::now())
	{
		mBeginSecond = clock_t::now();
		mCounter = 0;
	}

	auto now = clock_t::now();
	const auto delta = now - mBeginSecond;
	if ( delta <= mCounter * rate_ticks)
	{
		auto wait = ((mCounter+1)*rate_ticks) - delta;
		if (wait.count() > 0)
			std::this_thread::sleep_for(wait);
	}
	
	// do we need framedrop?
	// if passed time > time when we should have drawn next frame
	// maybe we should limit the number of consecutive framedrops?
	// for now: we can't do a framedrop if we did a framedrop last frame
	if ( delta > rate_ticks * (mCounter + 1) && !mFramedrop)
	{
		mFramedrop = true;
	} else
		mFramedrop = false;

	mCounter++;

	//calculate the FPS of drawn frames:
	if (mDrawFPS)
	{
		if (now >= mOldTicks + std::chrono::seconds{1})
		{
			mOldTicks = now;
			mFPS = mFPSCounter;
			mFPSCounter = 0;
		}

		if (!mFramedrop)
			mFPSCounter++;
	}
}

float SpeedController::getTargetFPS() const
{
	return mTargetFPS;
}


