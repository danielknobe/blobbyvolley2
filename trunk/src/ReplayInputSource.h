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

#include "Global.h"
#include "InputSource.h"
#include "ReplayRecorder.h"

// This is a InputSource which allows recording and playing of Replays
// Recording is done with ReplayInputSource hooking between the application
// and another input source, so it can record all other input sources.
// The given sources are deleted on destruction

class ReplayInputSource : public InputSource
{
//	friend class ReplayRecorder;
	friend InputSource* ReplayRecorder::
		createReplayInputSource(PlayerSide side, InputSource* source);
public:
	~ReplayInputSource();
	
	virtual PlayerInput getInput();

private:
	ReplayInputSource();
	
	InputSource* mRealSource;
	ReplayRecorder* mRecorder;
	PlayerSide mSide;
};

