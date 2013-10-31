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

#pragma once

#include <string>

/*! \class Clock
	\brief Game Timing Management
	\details This class represents a clock. It can be started, paused, resetted,
			and it is possible to get the time in a string for in-game representation
*/
class Clock
{
	public:
		/// default c'tor
		Clock();
		
		/// starts/unpauses the clock
		void start();
		/// pauses the clock
		void stop();
				
		/// resets the clock. after this, the clock is paused.
		void reset();
		
		/// gets whether the clock is currently running
		bool isRunning() const;
		
		/// this function has to be called each frame. It calculates
		///	the passed time;
		void step();
		
		/// gets the time in seconds as an integer
		int getTime() const;
		
		/// set the time to a specified value
		/// \param newTime: new time in seconds
		void setTime(int newTime);
		
		/// returns the time as a string
		std::string getTimeString() const;
		
	private:
		/// is the clock currently running?
		bool mRunning;
		
		/// recorded time in seconds
		time_t mGameTime;
		
		/// last time that step was called. 
		/// needed to calculate the time difference.
		time_t mLastTime;
		
};
