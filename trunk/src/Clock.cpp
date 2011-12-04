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

#include "Clock.h"

#include <ctime>
#include <sstream>

Clock::Clock():mRunning(false), mGameTime(0), mLastTime(0)
{
}

void Clock::reset()
{
	// set all variables to their default values
	mRunning = false;
	mGameTime = 0;
	mLastTime = 0;
}

void Clock::start()
{
	mLastTime = std::time(0);
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

int Clock::getTime() const 
{
	return mGameTime;
}
void Clock::setTime(int newTime) 
{
	mGameTime = newTime;
}

std::string Clock::getTimeString() const
{
	/// \todo maybe it makes sense to cache this value. we call this function ~75times a seconds
	///			when the string changes only once. guest it does not make that much of a difference, but still...
	// calculate seconds, minutes and hours as integers
	int seconds = mGameTime % 60;
	int minutes = ((mGameTime - seconds)/60) % 60;
	int hours = ((mGameTime - 60 * minutes - seconds) / 3600) % 60;
	
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

void Clock::step()
{
	if(mRunning)
	{
		time_t newTime = std::time(0);
		if(newTime > mLastTime)
		{
			mGameTime += newTime - mLastTime;
		}
		mLastTime = newTime;
	}
}
