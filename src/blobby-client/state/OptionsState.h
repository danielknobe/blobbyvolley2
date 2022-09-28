/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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
#include "TextManager.h"
#include <vector>

/*! \class OptionState
	\brief State for managing the main options menu
*/
class OptionState : public State
{
public:
	OptionState();
	~OptionState() override;
	void step_impl() override;
	const char* getStateName() const override;

private:
	/// writes current settings to disk
	void save();

	UserConfig mOptionConfig;
	std::vector<std::string> mScriptNames;
	unsigned mPlayerOptions[MAX_PLAYERS];
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
	~GraphicOptionsState() override;
	void step_impl() override;
	const char* getStateName() const override;
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
	~InputOptionsState() override;
	void step_impl() override;
	const char* getStateName() const override;
private:

	enum InputAction
	{
		IA_LEFT,
		IA_RIGHT,
		IA_JUMP,
		IA_COUNT
	};

	/// writes current settings to disk
	void save();

	UserConfig mOptionConfig;
	std::string oldString;
	int mOldInteger;
	std::string mOldString;

	int mSetKeyboard; // 1-10 for LeftKeyboard | 11-20 for RightKeyboard
	//left data:
	std::string mLeftDevice;
	int mLeftMouseJumpbutton;
	float mLeftMouseSensitivity;
	/// \todo maybe use a struct here
	std::string mLeftKeyboard[IA_COUNT];
	std::string mLeftJoystick[IA_COUNT];
	//right data:
	std::string mRightDevice;
	int mRightMouseJumpbutton;
	float mRightMouseSensitivity;
	std::string mRightKeyboard[IA_COUNT];
	std::string mRightJoystick[IA_COUNT];
	//global data:
	int mBlobbyTouchType;

#ifdef __SWITCH__
	bool mConfirmButtonReleased;
#endif

	// input display functions
#ifdef __SWITCH__
	void handlePlayerInput(PlayerSide player, std::string joystick[]);
#else
	void handlePlayerInput(PlayerSide player, std::string& lastActionKey, int& mouse, std::string keyboard[], std::string joystick[]);
#endif
	void handleKeyboardInput(int base_x, std::string& lastActionKey, std::string input[]);
	void handleJoystickInput(int base_x, std::string input[]);
	void handleMouseInput(int base_x, int& input, float& sens);

	// helper function
	void getMouseInput(int& action, TextManager::STRING input);
	void getKeyboardInput(std::string& action, TextManager::STRING input, std::string lastActionKey);
	void getJoystickInput(std::string& action, TextManager::STRING input);
	void getInputPrompt(TextManager::STRING prompt, TextManager::STRING input);
	// returns the correct text manager string for the device string
	TextManager::STRING getDeviceName(const std::string& device) const;
};

/*! \class MiscOptionsState
	\brief State for managing the misc options menu
*/
class MiscOptionsState : public State
{
public:
	MiscOptionsState();
	~MiscOptionsState() override;
	void step_impl() override;
	const char* getStateName() const override;
private:
	/// writes current settings to disk
	void save();

	UserConfig mOptionConfig;
	std::vector<std::string> mBackgrounds;
	unsigned mBackground;
	std::vector<std::string> mRules;
	unsigned mRule;
	float mVolume;
	bool mMute;
	int mGameFPS;
	bool mShowFPS;
	bool mShowBlood;
	int mNetworkSide;
	std::string mLanguage;
};
