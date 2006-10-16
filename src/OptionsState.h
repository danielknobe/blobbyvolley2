#pragma once

#include "Global.h"
#include "UserConfig.h"
#include "State.h"

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
	bool mSaveConfig;	
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

class MiscOptionsState : public State
{
public:
	MiscOptionsState();
	virtual ~MiscOptionsState();
	virtual void step();
private:
	UserConfig mOptionConfig;
	std::vector<std::string> mBackgrounds;
	int mBackground;
	float mVolume;
	bool mMute;
	int mGameFPS;
	bool mShowFPS;
	bool mSaveConfig;	
};
