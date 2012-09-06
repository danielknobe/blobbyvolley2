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

#pragma once

#include <boost/scoped_ptr.hpp>

#include "State.h"
#include "Vector.h"

class DuelMatch;
class ReplayPlayer;

/*! \class ReplayState
	\brief State playing a replay
*/
class ReplayState : public State
{
public:
	ReplayState();
	~ReplayState();
	virtual void step();
	virtual const char* getStateName() const;
	void loadReplay(const std::string& replay);
	
private:
	
	boost::scoped_ptr<DuelMatch> mReplayMatch;
	boost::scoped_ptr<ReplayPlayer> mReplayPlayer;

	bool mChecksumError;
	bool mVersionError;
	
	Vector2 mLastMousePosition;
	int mMouseShowTimer;

	Player mLeftPlayer;
	Player mRightPlayer;
	
	// controls
	int mPositionJump;
	bool mPaused;
	
	// replay speed control
	int mSpeedValue;
	int mSpeedTimer;
};
