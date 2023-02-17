/*=============================================================================
Blobby Volley 2
Copyright (C) 2023 Daniel Knobe (daniel-knobe@web.de)
Copyright (C) 2023 Erik Schultheis (erik-schultheis@freenet.de)

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
#include "RateController.h"

/* includes */
#include <cassert>

/* implementation */
using namespace std::chrono;

RateController::RateController() : mFrameDuration(0) {

}

void RateController::start(int frames_per_second) {
	mFrameDuration = duration_cast<nanoseconds>(seconds(1)) / frames_per_second;
	mLastTicks = clock_t::now();
}

bool RateController::handle_next_frame() {
	assert(mFrameDuration.count() != 0);
	if(wants_next_frame()) {
		mLastTicks += mFrameDuration;
		return true;
	}
	return false;
}

bool RateController::wants_next_frame() const
{
	assert(mFrameDuration.count() != 0);
	return clock_t::now() > mLastTicks + mFrameDuration;
}
