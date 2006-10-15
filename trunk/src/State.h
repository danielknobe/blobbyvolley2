#pragma once

#include "Global.h"
#include "UserConfig.h"

class DuelMatch;
class InputSource;
class ReplayRecorder;

class State
{
private:
	
protected:
	State();
	static State* mCurrentState;
	
public:
	virtual ~State() {}
	virtual void step() = 0;
	static State* getCurrentState();
};

class MainMenuState : public State
{
private:
public:
	MainMenuState();
	virtual ~MainMenuState();
	virtual void step();
};

class LocalGameState : public State
{
private:
	InputSource* mLeftInput;
	InputSource* mRightInput;
	
	Color mLeftColor;
	Color mRightColor;
	bool mLeftOscillate;
	bool mRightOscillate;
	bool mPaused;
	bool mSaveReplay;
	bool mWinner;
	std::string mFilename;
	
	DuelMatch* mMatch;
	ReplayRecorder* mRecorder;
public:
	LocalGameState();
	virtual ~LocalGameState();
	virtual void step();
	
};

class ReplayMenuState : public State
{
public:
	ReplayMenuState();
	virtual void step();
private:
	void loadCurrentReplay();
	DuelMatch* mReplayMatch;
	ReplayRecorder* mReplayRecorder;

	std::vector<std::string> mReplayFiles;
	int mSelectedReplay;
	bool mReplaying;
	
	int mPlayButton;
	int mCancelButton;
	int mDeleteButton;

	bool mLeftOscillate;
	bool mRightOscillate;
};
