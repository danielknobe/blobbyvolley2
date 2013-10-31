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

/**
 * @file SpeedController.h
 * @brief Contains a class which determine the framerate
 */

#pragma once

#include "BlobbyDebug.h"

/// \brief class controlling game speed
/// \details This class can control the game speed and the displayed FPS.
/// It is updated once a frame and waits the necessary time.
/// A distinction is made between game FPS and real FPS.
/// Game FPS is the number of game loop iterations per second,
/// real FPS is the number of screen updates per second. The real
/// FPS is reached with framedropping
/// The class can report how much time is actually waited. If this value
/// is close to zero, the real speed can be altered.


class SpeedController : public ObjectCounter<SpeedController>
{
	public:
		SpeedController(float gameFPS);
		~SpeedController();

		void setGameSpeed(float fps);
		float getGameSpeed() const{return mGameFPS;}

	/// This reports whether a framedrop is necessary to hold the real FPS
		bool doFramedrop() const;

	/// gives the caller the fps of the drawn frames:
		int getFPS() const { return mFPS; }
		void setDrawFPS(bool draw) { mDrawFPS = draw; }  //help methods
		bool getDrawFPS() const { return mDrawFPS; }

	/// This updates everything and waits the necessary time
		void update();

		static void setMainInstance(SpeedController* inst) { mMainInstance = inst; }
		static SpeedController* getMainInstance() { return mMainInstance; }
	private:
		float mGameFPS;
		int mFPS;
		int mFPSCounter;
		bool mFramedrop;
		bool mDrawFPS;
		static SpeedController* mMainInstance;
		int mOldTicks;

		// internal data
		unsigned int mBeginSecond;
		int mCounter;
};


