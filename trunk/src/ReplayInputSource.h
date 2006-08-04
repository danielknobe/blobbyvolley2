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

