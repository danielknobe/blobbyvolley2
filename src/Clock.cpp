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
#include "Clock.h"

/* includes */
#include <sstream>

/* implementation */

namespace {
	template<class T>
	Clock::seconds to_seconds(T input) {
		return std::chrono::duration_cast<Clock::seconds>(input);
	}
}

void Clock::reset()
{
	// set all variables to their default values
	mRunning = false;
	mGameRunning = duration_t(0);
	mLastTime = clock_t::now();
}

void Clock::start()
{
	mLastTime = clock_t::now();
	mRunning = true;
}

void Clock::stop()
{
	mRunning = false;
}

bool Clock::isRunning() const
{
	return mRunning;
}

Clock::seconds Clock::getTime() const
{
	return to_seconds(mGameRunning);
}

void Clock::setTime(milliseconds newTime)
{
	updateGameTime(newTime);
}

std::string Clock::makeTimeString(seconds time)
{
	int time_sec = time.count();
	int seconds = time_sec % 60;
	int minutes = ((time_sec - seconds) / 60) % 60;
	int hours = ((time_sec - 60 * minutes - seconds) / 3600) % 60;

	// now convert to string via stringstream
	std::stringstream stream;

	// only write hours if already player more than 1h
	if(hours > 0)
		stream << hours << ":";

	// write minutes
	// leading 0 if minutes < 10
	if(minutes < 10)
		stream << "0";

	stream << minutes << ":";

	// write seconds
	// leading 0 if seconds < 10
	if(seconds < 10)
		stream << "0";

	stream << seconds;

	// convert stringstream to string and return
	return stream.str();
}

const std::string& Clock::getTimeString() const
{
	return mCurrentTimeString;
}

void Clock::step()
{
	if(mRunning)
	{
		auto newTime = clock_t::now();
		updateGameTime(mGameRunning + (newTime - mLastTime));
		mLastTime = newTime;
	}
}

void Clock::updateGameTime(duration_t new_time) {
	auto old_sec = to_seconds( mGameRunning );
	auto new_sec = to_seconds( new_time);
	mGameRunning = new_time;

	// update time string only when the seconds change.
	if(old_sec != new_sec) {
		mCurrentTimeString = makeTimeString(new_sec);
	}
}