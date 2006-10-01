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
	PlayerSide mPlayer;
public:
	WinState(PlayerSide winningPlayer);	
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
	bool mPaused;
	bool mSaveReplay;
	std::string mFilename;
	
	DuelMatch* mMatch;
	ReplayRecorder* mRecorder;
	
public:
	LocalGameState();
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
	UserConfig mOptionConfig;
	std::vector<std::string> mScriptNames;
	int mPlayerOptions[MAX_PLAYERS];
	bool mShowFPS;
	bool mSaveConfig;	
};

class ReplayMenuState : public State
{
public:
	ReplayMenuState();
	virtual ~ReplayMenuState();
	virtual void step();
	
private:
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

class GraphicOptionsState : public State
{
public:
	GraphicOptionsState();
	virtual ~GraphicOptionsState();
	virtual void step();
private:
	UserConfig mOptionConfig;
	bool mSaveConfig;
	bool mFullscreen;
	std::string mRenderer;
	int mR1, mG1, mB1, mR2, mG2, mB2;
	bool mLeftMorphing, mRightMorphing;
};

class InputOptionsState : public State
{
public:
	InputOptionsState();
	virtual ~InputOptionsState();
	virtual void step();
private:
	UserConfig mOptionConfig;
	bool mSaveConfig;
	std::string oldString;
	int oldInteger;
	//left data:
	std::string mLeftBlobbyDevice;
	int mLeftBlobbyMouseJumpbutton;
	std::string mLeftBlobbyKeyboardLeft;
	std::string mLeftBlobbyKeyboardRight;
	std::string mLeftBlobbyKeyboardJump;
	std::string mLeftBlobbyJoystickLeft;
	std::string mLeftBlobbyJoystickRight;
	std::string mLeftBlobbyJoystickJump;
	//right data:
	std::string mRightBlobbyDevice;
	int mRightBlobbyMouseJumpbutton;
	std::string mRightBlobbyKeyboardLeft;
	std::string mRightBlobbyKeyboardRight;
	std::string mRightBlobbyKeyboardJump;
	std::string mRightBlobbyJoystickLeft;
	std::string mRightBlobbyJoystickRight;
	std::string mRightBlobbyJoystickJump;
};
