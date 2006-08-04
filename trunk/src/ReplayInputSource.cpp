#include "ReplayInputSource.h"

ReplayInputSource::ReplayInputSource()
{
}

ReplayInputSource::~ReplayInputSource()
{
	delete mRealSource;
}


PlayerInput ReplayInputSource::getInput()
{
	if (mRecorder->doGetInput())
	{
		return mRecorder->getInput(mSide);
	}
	else
	{
		PlayerInput input(false, false, false);
		if (mRealSource)
			input = mRealSource->getInput();
		if (mRecorder->doPushInput())
			mRecorder->pushInput(input, mSide);
		return input;
	}
}

