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

class WinState : public State
{
private:
	
public:
	WinState(int winningPlayer);	
	virtual ~WinState() {}
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
	
	DuelMatch* mMatch;
	ReplayRecorder* mRecorder;

public:
	LocalGameState(GameMode mode);
	virtual ~LocalGameState();
	virtual void step();
	
};

class OptionState : public State
{
public:
	OptionState();
	virtual ~OptionState();
	virtual void step();

private:
	void rebuildGUI();

	UserConfig mOptionConfig;
	std::vector<std::string> mScriptNames;
	std::vector<int> mLeftPlayerButtons;
	std::vector<int> mRightPlayerButtons;
	int mPlayerOptions[MAX_PLAYERS];
	bool mReplayActivated;

	int mReplayButton;	
	int mOkButton;
	int mCancelButton;

	bool mSaveConfig;	
};

class ReplayMenuState : public State
{
public:
	ReplayMenuState();
	virtual ~ReplayMenuState();
	virtual void step();
	
	static std::string getRecordName();
	
private:
	void rebuildGUI();
	
	DuelMatch* mReplayMatch;
	ReplayRecorder* mReplayRecorder;

	std::vector<std::string> mReplayFiles;
	std::vector<int> mReplayButtons;
	int mSelectedReplay;
	bool mReplaying;
	
	int mPlayButton;
	int mCancelButton;
	int mDeleteButton;

	bool mLeftOscillate;
	bool mRightOscillate;
};
