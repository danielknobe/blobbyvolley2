/*=============================================================================
Blobby Volley 2
Copyright (C) 2022 Daniel Knobe (daniel-knobe@web.de)
Copyright (C) 2022 Erik Schultheis (erik-schultheis@freenet.de)

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
#include "BlobbyApp.h"

/* includes */
#include <string>
#include <cassert>

#include "state/State.h"
#include "BlobbyDebug.h"

/* implementation */
void BlobbyApp::switchToState(std::unique_ptr<State> newState)
{
	assert(newState != nullptr);
	mStateToSwitchTo = std::move(newState);
}

State& BlobbyApp::getCurrentState() const
{
	return *mCurrentState;
}

const char* BlobbyApp::getCurrenStateName() const
{
	return mCurrentState->getStateName();
}

void BlobbyApp::step()
{
	// perform a state step
	mCurrentState->step_impl();

	// check if we should switch to a new state
	if( mStateToSwitchTo != nullptr )
	{
		// maybe this debug statuses will help pinpointing crashes faster
		DEBUG_STATUS( std::string("switching to state ") + mStateToSwitchTo->getStateName());

		// if yes, set that new state
		// use swap so the states do not get destroyed here
		mCurrentState = std::move( mStateToSwitchTo );
	}
}

BlobbyApp::BlobbyApp(std::unique_ptr<State> initState) : mCurrentState(std::move(initState))
{

}
