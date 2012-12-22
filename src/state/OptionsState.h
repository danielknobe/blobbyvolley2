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
#include "UserConfig.h"
#include "State.h"
#include <vector>

/*! \class OptionState
	\brief State for managing the main options menu
*/
class OptionState : public State
{
public:
	OptionState();
	virtual ~OptionState();
	virtual void step();
	virtual const char* getStateName() const;
	
private:
	/// writes current settings to disk
	void save();

	UserConfig mOptionConfig;
	std::vector<std::string> mScriptNames;
	int mPlayerOptions[MAX_PLAYERS];
	std::string mPlayerName[MAX_PLAYERS];
	unsigned mPlayerNamePosition[MAX_PLAYERS];
	unsigned mBotStrength[MAX_PLAYERS];
};

/*! \class GraphicOptionsState
	\brief State for managing the graphics options menu
*/
class GraphicOptionsState : public State
{
public:
	GraphicOptionsState();
	virtual ~GraphicOptionsState();
	virtual void step();
	virtual const char* getStateName() const;
private:
	/// writes current settings to disk
	void save();

	UserConfig mOptionConfig;
	bool mFullscreen;
	std::string mRenderer;
	int mR1, mG1, mB1, mR2, mG2, mB2;
	bool mLeftMorphing, mRightMorphing;
	bool mShowShadow;
};

/*! \class InputOptionsState
	\brief State for managing the input options menu
*/
class InputOptionsState : public State
{
public:
	InputOptionsState();
	virtual ~InputOptionsState();
	virtual void step();
	virtual const char* getStateName() const;
private:
	/// writes current settings to disk
	void save();

	UserConfig mOptionConfig;
	std::string oldString;
	int mOldInteger;
	int mSetKeyboard; // 1-10 for LeftKeyboard | 11-20 for RightKeyboard
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

/*! \class MiscOptionsState
	\brief State for managing the misc options menu
*/
class MiscOptionsState : public State
{
public:
	MiscOptionsState();
	virtual ~MiscOptionsState();
	virtual void step();
	virtual const char* getStateName() const;
private:
	/// writes current settings to disk
	void save();
	
	UserConfig mOptionConfig;
	std::vector<std::string> mBackgrounds;
	int mBackground;
	std::vector<std::string> mRules;
	int mRule;
	float mVolume;
	bool mMute;
	int mGameFPS;
	bool mShowFPS;
	bool mShowBlood;
	int mNetworkSide;
	std::string mLanguage;
};
