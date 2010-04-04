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

Clock::Clock():mRunning(false), mGameTime(0), mLastTime(0){
}

void Clock::reset(){
	mRunning = false;
	mGameTime = 0;
	mLastTime = 0;
}

void Clock::start(){
	mLastTime = std::time(0);
	mRunning = true;
}

std::string Clock::getTimeString() const{
	int seconds = mGameTime % 60;
	int minutes = ((mGameTime - seconds)/60) % 60;
	int hours = ((mGameTime - 60 * minutes - seconds) / 3600) % 60;
	std::stringstream stream;
	if(hours > 0)
		stream << hours << ":";
	if(minutes < 10)
		stream <<"0";
	stream << minutes <<":";
	if(seconds < 10)
		stream <<"0";
	stream << seconds;
	
	return stream.str();
}

void Clock::step(){
	if(mRunning){
		time_t newTime = std::time(0);
		if(newTime > mLastTime){
			mGameTime += newTime - mLastTime;
		}
		mLastTime = newTime;
	}
}
