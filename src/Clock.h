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
#include <chrono>

/*! \class Clock
	\brief Game Timing Management
	\details This class represents a clock. It can be started, paused, reset,
			and it is possible to get the time in a string for in-game representation
*/
class Clock
{
	public:
		using seconds = std::chrono::seconds;
		using milliseconds = std::chrono::milliseconds;

		/// default c'tor
		Clock() = default;
		
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
		seconds getTime() const;
		
		/// set the time to a specified value
		/// \param newTime: new time in seconds
		void setTime(seconds newTime);
		
		/// returns the time as a string
		const std::string& getTimeString() const;
		
	private:
		/// is the clock currently running?
		bool mRunning{false};
		
		/// recorded time in milliseconds. Do not write to this directly, but use
		/// `updateGameTime` to ensure synchronization with `mCurrentTimeString`
		milliseconds mGameRunning{0};
		
		/// last time that step was called. 
		/// needed to calculate the time difference.
		milliseconds mLastTime{0};

		/// Currently formatted time text
		std::string mCurrentTimeString;

		/// Sets the new game time, and updates `mCurrentTimeString` if appropriate.
		void updateGameTime(milliseconds newTime);

		/// Formats the given amount of seconds into a time string.
		static std::string makeTimeString(seconds time);
};
