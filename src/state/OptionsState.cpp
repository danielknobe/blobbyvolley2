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

/* header include */
#include "OptionsState.h"

/* includes */
#include <sstream>
#include <string>

#include "State.h"
#include "RenderManager.h"
#include "InputManager.h"
#include "LocalInputSource.h"
#include "SpeedController.h"
#include "SoundManager.h"
#include "Blood.h"
#include "IMGUI.h"
#include "TextManager.h"
#include "FileSystem.h"

/* implementation */
OptionState::OptionState()
{
	mOptionConfig.loadFile("config.xml");
	mPlayerOptions[LEFT_PLAYER] = 0;
	mPlayerOptions[RIGHT_PLAYER] = 0;
	std::string leftScript = mOptionConfig.getString("left_script_name");
	std::string rightScript = mOptionConfig.getString("right_script_name");

	mScriptNames = FileSystem::getSingleton().enumerateFiles("scripts", ".lua");

	// hack. we cant use something like push_front, though
	mScriptNames.push_back("Human");
	std::swap(mScriptNames[0], mScriptNames[mScriptNames.size() - 1]);

	for(unsigned int i = 0; i < mScriptNames.size(); ++i)
	{
		if (mScriptNames[i] == leftScript)
			mPlayerOptions[LEFT_PLAYER] = i;
		if (mScriptNames[i] == rightScript)
			mPlayerOptions[RIGHT_PLAYER] = i;
	}

	if (mOptionConfig.getBool("left_player_human"))
		mPlayerOptions[LEFT_PLAYER] = 0;
	if (mOptionConfig.getBool("right_player_human"))
		mPlayerOptions[RIGHT_PLAYER] = 0;

	mPlayerName[LEFT_PLAYER] = mOptionConfig.getString("left_player_name");
	mPlayerName[RIGHT_PLAYER] = mOptionConfig.getString("right_player_name");
	mPlayerNamePosition[RIGHT_PLAYER] = 0;
	mPlayerNamePosition[LEFT_PLAYER] = 0;

	mBotStrength[LEFT_PLAYER] = mOptionConfig.getInteger("left_script_strength");
	mBotStrength[RIGHT_PLAYER] = mOptionConfig.getInteger("right_script_strength");
}

OptionState::~OptionState()
{
}

void OptionState::save()
{
	if (mPlayerOptions[LEFT_PLAYER] == 0)
	{
		mOptionConfig.setBool("left_player_human", true);
	}
	else
	{
		mOptionConfig.setBool("left_player_human", false);
		mOptionConfig.setString("left_script_name", mScriptNames[mPlayerOptions[LEFT_PLAYER]]);
	}

	if (mPlayerOptions[RIGHT_PLAYER] == 0)
	{
		mOptionConfig.setBool("right_player_human", true);
	}
	else
	{
		mOptionConfig.setBool("right_player_human", false);
		mOptionConfig.setString("right_script_name", mScriptNames[mPlayerOptions[RIGHT_PLAYER]]);
	}
	mOptionConfig.setString("left_player_name", mPlayerName[LEFT_PLAYER]);
	mOptionConfig.setString("right_player_name", mPlayerName[RIGHT_PLAYER]);
	mOptionConfig.setInteger("left_script_strength", mBotStrength[LEFT_PLAYER]);
	mOptionConfig.setInteger("right_script_strength", mBotStrength[RIGHT_PLAYER]);
	mOptionConfig.saveFile("config.xml");
}

void OptionState::step_impl()
{
	const int MAX_BOT_DELAY = 25;		// 25 frames = 0.33s (gamespeed: normal)

	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doEditbox(GEN_ID, Vector2(5.0, 10.0), 15, mPlayerName[LEFT_PLAYER], mPlayerNamePosition[LEFT_PLAYER]);
	imgui.doEditbox(GEN_ID, Vector2(425.0, 10.0), 15, mPlayerName[RIGHT_PLAYER], mPlayerNamePosition[RIGHT_PLAYER]);

	imgui.doSelectbox(GEN_ID, Vector2(5.0, 50.0), Vector2(375.0, 300.0), mScriptNames, mPlayerOptions[LEFT_PLAYER]);
	imgui.doSelectbox(GEN_ID, Vector2(425.0, 50.0), Vector2(795.0, 300.0), mScriptNames, mPlayerOptions[RIGHT_PLAYER]);

	imgui.doText(GEN_ID, Vector2(270.0, 310.0), TextManager::OP_DIFFICULTY );

	float f = 1.f - (float)mBotStrength[0] / MAX_BOT_DELAY;
	imgui.doScrollbar(GEN_ID, Vector2(15.0, 350.0), f);
	mBotStrength[0] = static_cast<unsigned int> ((1.f-f) * MAX_BOT_DELAY + 0.5f);
	imgui.doText(GEN_ID, Vector2(235.0, 350.0), f > 0.66 ? TextManager::OP_STRONG :
											 	(f > 0.33 ? TextManager::OP_MEDIUM:
												TextManager::OP_WEAK));

	f = 1.f - (float)mBotStrength[1] / MAX_BOT_DELAY;
	imgui.doScrollbar(GEN_ID, Vector2(440.0, 350.0), f);
	mBotStrength[1] = static_cast<unsigned int> ((1.f - f) * MAX_BOT_DELAY + 0.5f);
	imgui.doText(GEN_ID, Vector2(660.0, 350.0), f > 0.66 ? TextManager::OP_STRONG :
											 	(f > 0.33 ? TextManager::OP_MEDIUM:
												TextManager::OP_WEAK));

	if (imgui.doButton(GEN_ID, Vector2(40.0, 390.0), TextManager::OP_INPUT_OP))
	{
		save();
		switchState(new InputOptionsState());
	}

	if (imgui.doButton(GEN_ID, Vector2(40.0, 430.0), TextManager::OP_GFX_OP))
	{
		save();
		switchState(new GraphicOptionsState());
	}
	if (imgui.doButton(GEN_ID, Vector2(40.0, 470.0), TextManager::OP_MISC))
	{
		save();
		switchState(new MiscOptionsState());
	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), TextManager::LBL_OK))
	{
		save();
		switchState(new MainMenuState());
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), TextManager::LBL_CANCEL))
	{
		switchState(new MainMenuState());
	}
}

const char* OptionState::getStateName() const
{
	return "OptionState";
}

GraphicOptionsState::GraphicOptionsState()
{
	mOptionConfig.loadFile("config.xml");
	mFullscreen = mOptionConfig.getBool("fullscreen");
	mRenderer = mOptionConfig.getString("device");
	mR1 = mOptionConfig.getInteger("left_blobby_color_r");
	mG1 = mOptionConfig.getInteger("left_blobby_color_g");
	mB1 = mOptionConfig.getInteger("left_blobby_color_b");
	mR2 = mOptionConfig.getInteger("right_blobby_color_r");
	mG2 = mOptionConfig.getInteger("right_blobby_color_g");
	mB2 = mOptionConfig.getInteger("right_blobby_color_b");
	mLeftMorphing = mOptionConfig.getBool("left_blobby_oscillate");
	mRightMorphing = mOptionConfig.getBool("right_blobby_oscillate");
	mShowShadow = mOptionConfig.getBool("show_shadow");
}

GraphicOptionsState::~GraphicOptionsState()
{
}

void GraphicOptionsState::save()
{
#if __DESKTOP__
	if ((mOptionConfig.getBool("fullscreen") != mFullscreen) ||	(mOptionConfig.getString("device") != mRenderer))
	{
		mOptionConfig.setBool("fullscreen", mFullscreen);
		mOptionConfig.setString("device", mRenderer);
		if (mRenderer == "OpenGL")
			RenderManager::createRenderManagerGL2D()->init(800, 600, mFullscreen);
		else
			RenderManager::createRenderManagerSDL()->init(800, 600, mFullscreen);
		RenderManager::getSingleton().setBackground(std::string("backgrounds/") + mOptionConfig.getString("background"));
	}
#endif

	RenderManager::getSingleton().showShadow(mShowShadow);
	mOptionConfig.setBool("show_shadow", mShowShadow);

	mOptionConfig.setInteger("left_blobby_color_r", mR1);
	mOptionConfig.setInteger("left_blobby_color_g", mG1);
	mOptionConfig.setInteger("left_blobby_color_b", mB1);
	mOptionConfig.setInteger("right_blobby_color_r", mR2);
	mOptionConfig.setInteger("right_blobby_color_g", mG2);
	mOptionConfig.setInteger("right_blobby_color_b", mB2);
	mOptionConfig.setBool("left_blobby_oscillate", mLeftMorphing);
	mOptionConfig.setBool("right_blobby_oscillate", mRightMorphing);

	mOptionConfig.saveFile("config.xml");
}

void GraphicOptionsState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

#if __DESKTOP__
	imgui.doText(GEN_ID, Vector2(34.0, 10.0), TextManager::OP_VIDEO);

	if (imgui.doButton(GEN_ID, Vector2(34.0, 40.0), TextManager::OP_FULLSCREEN))
		mFullscreen = true;
	if (imgui.doButton(GEN_ID, Vector2(34.0, 70.0), TextManager::OP_WINDOW))
		mFullscreen = false;
	if (mFullscreen)
		imgui.doImage(GEN_ID, Vector2(18.0, 52.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(18.0, 82.0), "gfx/pfeil_rechts.bmp");

	imgui.doText(GEN_ID, Vector2(444.0, 10.0), TextManager::OP_RENDER_DEVICE);
	if (imgui.doButton(GEN_ID, Vector2(444.0, 40.0), "OpenGL"))
		mRenderer = "OpenGL";
	if (imgui.doButton(GEN_ID, Vector2(444.0, 70.0), "SDL"))
		mRenderer = "SDL";
	if (mRenderer == "OpenGL")
		imgui.doImage(GEN_ID, Vector2(428.0, 52.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(428.0, 82.0), "gfx/pfeil_rechts.bmp");
#endif
	float heightOfElement = 110.0;

#if __MOBILE__
	heightOfElement = 10.0;
#endif

	imgui.doText(GEN_ID, Vector2(34.0, heightOfElement), TextManager::OP_SHOW_SHADOW);
	heightOfElement += 30;
	if (imgui.doButton(GEN_ID, Vector2(72.0, heightOfElement), TextManager::LBL_YES))
		mShowShadow = true;
	if (imgui.doButton(GEN_ID, Vector2(220.0, heightOfElement), TextManager::LBL_NO))
		mShowShadow = false;
	if (mShowShadow)
		imgui.doImage(GEN_ID, Vector2(54.0, heightOfElement + 13.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(204.0, heightOfElement + 13.0), "gfx/pfeil_rechts.bmp");

#if __MOBILE__
	float standardLineHeight = 50.0;
#else
	float standardLineHeight = 30.0;
#endif

	heightOfElement += standardLineHeight;

	//Blob colors:
	imgui.doText(GEN_ID, Vector2(280.0, heightOfElement), TextManager::OP_BLOB_COLORS);
	heightOfElement += 40.0;

	float playerColorSettingsHeight = heightOfElement;

	//left blob:
	imgui.doText(GEN_ID, Vector2(34.0, heightOfElement), TextManager::OP_LEFT_PLAYER);

	heightOfElement += standardLineHeight;

	{
		imgui.doText(GEN_ID, Vector2(34.0, heightOfElement), TextManager::OP_RED);
		float r1 = (float)mR1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, heightOfElement), r1);
		mR1 = (int)(r1*255);
	}

	heightOfElement += standardLineHeight;

	{
		imgui.doText(GEN_ID, Vector2(34.0, heightOfElement), TextManager::OP_GREEN);
		float g1 = (float)mG1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, heightOfElement), g1);
		mG1 = (int)(g1*255);
	}

	heightOfElement += standardLineHeight;

	{
		imgui.doText(GEN_ID, Vector2(34.0, heightOfElement), TextManager::OP_BLUE);
		float b1 = (float)mB1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, heightOfElement), b1);
		mB1 = (int)(b1*255);
	}
	imgui.doText(GEN_ID, Vector2(34.0, 360), TextManager::OP_MORPHING);
	if (imgui.doButton(GEN_ID, Vector2(72.0, 390), TextManager::LBL_YES))
		mLeftMorphing = true;
	if (imgui.doButton(GEN_ID, Vector2(220.0, 390), TextManager::LBL_NO))
		mLeftMorphing = false;
	if (mLeftMorphing)
		imgui.doImage(GEN_ID, Vector2(54.0, 402.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(204.0, 402.0), "gfx/pfeil_rechts.bmp");
	//draw left blob:
	{
		float time = float(SDL_GetTicks()) / 1000.0;
		Color ourCol = Color(mR1, mG1, mB1);
		if (mLeftMorphing)
			ourCol = Color(int((sin(time*2) + 1.0) * 128),
							int((sin(time*4) + 1.0) * 128),
							int((sin(time*3) + 1.0) * 128));
		imgui.doBlob(GEN_ID, Vector2(110, 500), ourCol);
	}

	//right blob:
	heightOfElement = playerColorSettingsHeight;

	imgui.doText(GEN_ID, Vector2(434.0, heightOfElement), TextManager::OP_RIGHT_PLAYER);

	heightOfElement += standardLineHeight;

	{
		imgui.doText(GEN_ID, Vector2(434.0, heightOfElement), TextManager::OP_RED);
		float r2 = (float)mR2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, heightOfElement), r2);
		mR2 = (int)(r2*255);
	}

	heightOfElement += standardLineHeight;

	{
		imgui.doText(GEN_ID, Vector2(434.0, heightOfElement), TextManager::OP_GREEN);
		float g2 = (float)mG2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, heightOfElement), g2);
		mG2 = (int)(g2*255);
	}

	heightOfElement += standardLineHeight;

	{
		imgui.doText(GEN_ID, Vector2(434.0, heightOfElement), TextManager::OP_BLUE);
		float b2 = (float)mB2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, heightOfElement), b2);
		mB2 = (int)(b2*255);
	}
	imgui.doText(GEN_ID, Vector2(434.0, 360), TextManager::OP_MORPHING);
	if (imgui.doButton(GEN_ID, Vector2(472.0, 390), TextManager::LBL_YES))
		mRightMorphing = true;
	if (imgui.doButton(GEN_ID, Vector2(620.0, 390), TextManager::LBL_NO))
		mRightMorphing = false;
	if (mRightMorphing)
		imgui.doImage(GEN_ID, Vector2(454.0, 402.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(604.0, 402.0), "gfx/pfeil_rechts.bmp");
	//draw right blob:
	{
		float time = float(SDL_GetTicks()) / 1000.0;
		Color ourCol = Color(mR2, mG2, mB2);
		if (mRightMorphing)
			ourCol = Color(int((cos(time*2) + 1.0) * 128),
							int((cos(time*4) + 1.0) * 128),
							int((cos(time*3) + 1.0) * 128));
		imgui.doBlob(GEN_ID, Vector2(670, 500), ourCol);
	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), TextManager::LBL_OK))
	{
		save();
		switchState(new OptionState());
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), TextManager::LBL_CANCEL))
	{
		switchState(new OptionState());
	}
}

const char* GraphicOptionsState::getStateName() const
{
	return "GraphicOptionsState";
}

InputOptionsState::InputOptionsState()
{
	mSetKeyboard = 0;
	mOptionConfig.loadFile("inputconfig.xml");
	//left data:
	mLeftDevice = mOptionConfig.getString("left_blobby_device");
	mLeftMouseJumpbutton = mOptionConfig.getInteger("left_blobby_mouse_jumpbutton");
	mLeftMouseSensitivity = mOptionConfig.getFloat("left_blobby_mouse_sensitivity");
	mLeftKeyboard[IA_LEFT] = mOptionConfig.getString("left_blobby_keyboard_left");
	mLeftKeyboard[IA_RIGHT] = mOptionConfig.getString("left_blobby_keyboard_right");
	mLeftKeyboard[IA_JUMP] = mOptionConfig.getString("left_blobby_keyboard_jump");
	mLeftJoystick[IA_LEFT] = mOptionConfig.getString("left_blobby_joystick_left");
	mLeftJoystick[IA_RIGHT] = mOptionConfig.getString("left_blobby_joystick_right");
	mLeftJoystick[IA_JUMP] = mOptionConfig.getString("left_blobby_joystick_jump");
	//right data:
	mRightDevice = mOptionConfig.getString("right_blobby_device");
	mRightMouseJumpbutton = mOptionConfig.getInteger("right_blobby_mouse_jumpbutton");
	mRightMouseSensitivity = mOptionConfig.getFloat("right_blobby_mouse_sensitivity");
	mRightKeyboard[IA_LEFT] = mOptionConfig.getString("right_blobby_keyboard_left");
	mRightKeyboard[IA_RIGHT] = mOptionConfig.getString("right_blobby_keyboard_right");
	mRightKeyboard[IA_JUMP] = mOptionConfig.getString("right_blobby_keyboard_jump");
	mRightJoystick[IA_LEFT] = mOptionConfig.getString("right_blobby_joystick_left");
	mRightJoystick[IA_RIGHT] = mOptionConfig.getString("right_blobby_joystick_right");
	mRightJoystick[IA_JUMP] = mOptionConfig.getString("right_blobby_joystick_jump");
	//global data:
	mBlobbyTouchType = mOptionConfig.getInteger("blobby_touch_type");
}

InputOptionsState::~InputOptionsState()
{
}

void InputOptionsState::save()
{
	//left data:
	mOptionConfig.setString("left_blobby_device", mLeftDevice);
	mOptionConfig.setInteger("left_blobby_mouse_jumpbutton", mLeftMouseJumpbutton);
	mOptionConfig.setFloat("left_blobby_mouse_sensitivity", mLeftMouseSensitivity);
	mOptionConfig.setString("left_blobby_keyboard_left", mLeftKeyboard[IA_LEFT]);
	mOptionConfig.setString("left_blobby_keyboard_right", mLeftKeyboard[IA_RIGHT]);
	mOptionConfig.setString("left_blobby_keyboard_jump", mLeftKeyboard[IA_JUMP]);
	mOptionConfig.setString("left_blobby_joystick_left", mLeftJoystick[IA_LEFT]);
	mOptionConfig.setString("left_blobby_joystick_right", mLeftJoystick[IA_RIGHT]);
	mOptionConfig.setString("left_blobby_joystick_jump", mLeftJoystick[IA_JUMP]);
	//right data:
	mOptionConfig.setString("right_blobby_device", mRightDevice);
	mOptionConfig.setInteger("right_blobby_mouse_jumpbutton", mRightMouseJumpbutton);
	mOptionConfig.setFloat("right_blobby_mouse_sensitivity", mRightMouseSensitivity);
	mOptionConfig.setString("right_blobby_keyboard_left", mRightKeyboard[IA_LEFT]);
	mOptionConfig.setString("right_blobby_keyboard_right", mRightKeyboard[IA_RIGHT]);
	mOptionConfig.setString("right_blobby_keyboard_jump", mRightKeyboard[IA_JUMP]);
	mOptionConfig.setString("right_blobby_joystick_left", mRightJoystick[IA_LEFT]);
	mOptionConfig.setString("right_blobby_joystick_right", mRightJoystick[IA_RIGHT]);
	mOptionConfig.setString("right_blobby_joystick_jump", mRightJoystick[IA_JUMP]);
	//global data:
	mOptionConfig.setInteger("blobby_touch_type", mBlobbyTouchType);

	mOptionConfig.saveFile("inputconfig.xml");
}

#if __DESKTOP__
void InputOptionsState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	std::string lastActionKey = InputManager::getSingleton()->getLastActionKey();

	// left player side:
	handlePlayerInput(LEFT_PLAYER, lastActionKey, mLeftMouseJumpbutton, mLeftKeyboard, mLeftJoystick);

	//right player side:
	handlePlayerInput(RIGHT_PLAYER, lastActionKey, mRightMouseJumpbutton, mRightKeyboard, mRightJoystick);

	//check if a capture window is open, to set all widgets inactive:
	if (mLeftKeyboard[IA_LEFT] != "" && mLeftKeyboard[IA_RIGHT] != "" && mLeftKeyboard[IA_JUMP] != "" && mLeftJoystick[IA_LEFT] != "" &&
		 mLeftJoystick[IA_RIGHT] != "" && mLeftJoystick[IA_JUMP] != "" && mLeftMouseJumpbutton != -1 && mRightKeyboard[IA_LEFT] != "" &&
		 mRightKeyboard[IA_RIGHT] != "" && mRightKeyboard[IA_JUMP] != "" && mRightJoystick[IA_LEFT] != "" && mRightJoystick[IA_RIGHT] != "" &&
		 mRightJoystick[IA_JUMP] != "" && mRightMouseJumpbutton != -1)
	{
		imgui.doCursor(true);
		imgui.doInactiveMode(false);
	}
	else
	{
		imgui.doInactiveMode(true);
		imgui.doCursor(false);
	}

	//Capture dialogs:
	getMouseInput(mLeftMouseJumpbutton, TextManager::OP_JUMPING);

	getKeyboardInput(mLeftKeyboard[IA_LEFT], TextManager::OP_MOVING_LEFT, lastActionKey);
	getKeyboardInput(mLeftKeyboard[IA_RIGHT], TextManager::OP_MOVING_RIGHT, lastActionKey);
	getKeyboardInput(mLeftKeyboard[IA_JUMP], TextManager::OP_JUMPING, lastActionKey);

	getJoystickInput(mLeftJoystick[IA_LEFT], TextManager::OP_MOVING_LEFT);
	getJoystickInput(mLeftJoystick[IA_RIGHT], TextManager::OP_MOVING_RIGHT);
	getJoystickInput(mLeftJoystick[IA_JUMP], TextManager::OP_JUMPING);

	getMouseInput(mRightMouseJumpbutton, TextManager::OP_JUMPING);

	getKeyboardInput(mRightKeyboard[IA_LEFT], TextManager::OP_MOVING_LEFT, lastActionKey);
	getKeyboardInput(mRightKeyboard[IA_RIGHT], TextManager::OP_MOVING_RIGHT, lastActionKey);
	getKeyboardInput(mRightKeyboard[IA_JUMP], TextManager::OP_JUMPING, lastActionKey);

	getJoystickInput(mRightJoystick[IA_LEFT], TextManager::OP_MOVING_LEFT);
	getJoystickInput(mRightJoystick[IA_RIGHT], TextManager::OP_MOVING_RIGHT);
	getJoystickInput(mRightJoystick[IA_JUMP], TextManager::OP_JUMPING);

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), TextManager::LBL_OK))
	{
		save();
		switchState(new OptionState());
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), TextManager::LBL_CANCEL))
	{
		switchState(new OptionState());
	}
}

void InputOptionsState::handlePlayerInput(PlayerSide player, std::string& lastActionKey, int& mouse, std::string keyboard[], std::string joystick[])
{
	IMGUI& imgui = IMGUI::getSingleton();

	TextManager::STRING p_str = (player == LEFT_PLAYER) ? TextManager::OP_LEFT_PLAYER : TextManager::OP_RIGHT_PLAYER;
	std::string& device = (player == LEFT_PLAYER) ? mLeftDevice : mRightDevice;
	int base_x = (player == LEFT_PLAYER) ? 0 : 400;
	imgui.doText(GEN_ID, Vector2(base_x + 34.0, 10.0), p_str);

	if (imgui.doButton(GEN_ID, Vector2(base_x + 80.0, 60.0), getDeviceName(device)))
	{
		if (device == "mouse")
		{
			device = "keyboard";
		}
		else if (device == "keyboard")
		{
			device = "joystick";
		}
		else if (device == "joystick")
		{
			if (mRightDevice != "mouse" && mLeftDevice != "mouse")
			{
				device = "mouse";
			}
			else
			{
				device = "keyboard";
			}
		}
	}
	//if mouse device is selected:
	if (device == "mouse")
	{
		handleMouseInput(base_x, mouse, player == LEFT_PLAYER ? mLeftMouseSensitivity : mRightMouseSensitivity);
	}
	if ((mouse == -2) && (InputManager::getSingleton()->getLastMouseButton() == -1))
		mouse = -1;
	//if keyboard device is selected:
	if (device == "keyboard")
	{
		handleKeyboardInput(base_x, lastActionKey, keyboard);
	}
	//if joystick device is selected:
	if (device == "joystick")
	{
		handleJoystickInput(base_x, joystick);
	}
}

void InputOptionsState::handleKeyboardInput(int base_x, std::string& lastActionKey, std::string input[])
{
	auto& imgui = IMGUI::getSingleton();

	if (imgui.doButton(GEN_ID, Vector2(base_x + 34, 350.0), TextManager::OP_SET_ALL))
		mSetKeyboard = base_x + 1;

	imgui.doText(GEN_ID, Vector2(base_x + 34.0, 120.0), TextManager::OP_LEFT_KEY);
	if (imgui.doButton(GEN_ID, Vector2(base_x + 50, 150.0), std::string("Key ")+input[IA_LEFT]) || mSetKeyboard == base_x + 1)
	{
		lastActionKey = "";
		mOldString = input[IA_LEFT];
		input[IA_LEFT] = "";
	}

	if (mSetKeyboard == base_x + 1)
		mSetKeyboard = base_x + 2;

	if (mSetKeyboard == base_x + 2 && input[IA_LEFT] != "")
		mSetKeyboard = base_x + 3;

	imgui.doText(GEN_ID, Vector2(base_x + 34.0, 190.0), TextManager::OP_RIGHT_KEY);
	if (imgui.doButton(GEN_ID, Vector2(base_x + 50, 220.0), std::string("Key ")+input[IA_RIGHT]) || mSetKeyboard == base_x + 3)
	{
		lastActionKey = "";
		mOldString = input[IA_RIGHT];
		input[IA_RIGHT] = "";
	}

	if (mSetKeyboard == base_x + 3)
		mSetKeyboard = base_x + 4;

	if (mSetKeyboard == base_x + 4 && input[IA_RIGHT] != "")
		mSetKeyboard = base_x + 5;

	imgui.doText(GEN_ID, Vector2(base_x + 34.0, 260.0), TextManager::OP_JUMP_KEY );
	if (imgui.doButton(GEN_ID, Vector2(base_x + 50, 290.0), std::string("Key ")+input[IA_JUMP]) || mSetKeyboard == base_x + 5)
	{
		lastActionKey = "";
		mOldString = input[IA_JUMP];
		input[IA_JUMP] = "";
	}

	if (mSetKeyboard == base_x + 5)
		mSetKeyboard = base_x + 6;

	if (mSetKeyboard == base_x + 6 && input[IA_JUMP] != "")
		mSetKeyboard = 0;
}

void InputOptionsState::handleJoystickInput(int base_x, std::string input[])
{
	auto& imgui = IMGUI::getSingleton();

	imgui.doText(GEN_ID, Vector2(base_x + 34.0, 120.0), TextManager::OP_LEFT_BUTTON);
	if (imgui.doButton(GEN_ID, Vector2(base_x + 50, 150.0), input[IA_LEFT]))
	{
		mOldString = input[IA_LEFT];
		input[IA_LEFT] = "";
	}
	imgui.doText(GEN_ID, Vector2(base_x + 34.0, 190.0), TextManager::OP_RIGHT_BUTTON);
	if (imgui.doButton(GEN_ID, Vector2(base_x + 50, 220.0), input[IA_RIGHT]))
	{
		mOldString = input[IA_RIGHT];
		input[IA_RIGHT] = "";
	}
	imgui.doText(GEN_ID, Vector2(base_x + 34.0, 260.0), TextManager::OP_JUMP_BUTTON);
	if (imgui.doButton(GEN_ID, Vector2(base_x + 50, 290.0), input[IA_JUMP]))
	{
		mOldString = input[IA_JUMP];
		input[IA_JUMP] = "";
	}
}

void InputOptionsState::handleMouseInput(int base_x, int& input, float& sens)
{
	auto& imgui = IMGUI::getSingleton();

	imgui.doText(GEN_ID, Vector2(base_x + 34.0, 120.0), TextManager::OP_JUMP_BUTTON);
	std::ostringstream text;
	if (input >= 0)
		text << "Button " << input;
	else
		text << "Button ";
	if (imgui.doButton(GEN_ID, Vector2(base_x + 50, 150.0), text.str()))
	{
		mOldInteger = input;
		input = -2;
	}

	// sensitivity settings
	imgui.doText(GEN_ID, Vector2(base_x + 34.0, 200), /*TextManager::OP_SENSITIVITY*/"SENSITIVITY");
	imgui.doScrollbar(GEN_ID, Vector2(base_x + 50, 240), sens);

}

void InputOptionsState::getInputPrompt(TextManager::STRING prompt, TextManager::STRING input)
{
	auto& imgui = IMGUI::getSingleton();

	imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
	imgui.doText(GEN_ID, Vector2(400.0, 250.0), prompt, TF_ALIGN_CENTER);
	imgui.doText(GEN_ID, Vector2(400.0, 300.0), input, TF_ALIGN_CENTER);
}

void InputOptionsState::getMouseInput(int& action, TextManager::STRING input)
{
	// if already set, do nothing
	if(action != -1)
		return;

	getInputPrompt(TextManager::OP_PRESS_MOUSE_BUTTON, input);
	action = InputManager::getSingleton()->getLastMouseButton();
	if (InputManager::getSingleton()->exit())
		action = mOldInteger;
}

void InputOptionsState::getKeyboardInput(std::string& action, TextManager::STRING input, std::string lastActionKey)
{
	// if already set, do nothing
	if (action != "")
		return;

	getInputPrompt(TextManager::OP_PRESS_KEY_FOR, input);
	action = lastActionKey;
	if (InputManager::getSingleton()->exit())
		action = mOldString;
}

void InputOptionsState::getJoystickInput(std::string& action, TextManager::STRING input)
{
	// if already set, do nothing
	if (action != "")
		return;

	getInputPrompt(TextManager::OP_PRESS_BUTTON_FOR, input);
	action = InputManager::getSingleton()->getLastJoyAction();
	if (InputManager::getSingleton()->exit())
		action = mOldString;

}


TextManager::STRING InputOptionsState::getDeviceName(const std::string& device) const
{
	// dirty hack for languagesupport
	if (device[0] == 'k')
		return TextManager::OP_KEYBOARD;
	else if (device[0] == 'm')
		return TextManager::OP_MOUSE;
	return TextManager::OP_JOYSTICK;
}

#else

void InputOptionsState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doText(GEN_ID, Vector2(43.0, 10.0), TextManager::OP_TOUCH_TYPE);
	if (imgui.doButton(GEN_ID, Vector2(70.0, 50.0), TextManager::OP_TOUCH_DIRECT))
		mBlobbyTouchType = 0;
	if (imgui.doButton(GEN_ID, Vector2(70.0, 100.0), TextManager::OP_TOUCH_ARROWS))
		mBlobbyTouchType = 1;
	if (mBlobbyTouchType == 0)
		imgui.doImage(GEN_ID, Vector2(52.0, 62.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(52.0, 112.0), "gfx/pfeil_rechts.bmp");

	imgui.doOverlay(GEN_ID, Vector2(180.0, 150.0), Vector2(620.0, 490.0));
	imgui.doImage(GEN_ID, Vector2(400.0, 320.0), "background", Vector2(400.0, 300.0));

	// We draw the range of the touchsurfaces here
	if (mBlobbyTouchType == 0)
	{
		// Left arrowkey
		imgui.doOverlay(GEN_ID, Vector2(200.0 + 5.0, 170.0 + 100.0 + 5.0), Vector2(200.0 + 95.0 + 100.0, 170.0 + 295));
		imgui.doImage(GEN_ID, Vector2(289.0, 440.0), "gfx/pfeil_links.bmp");
		imgui.doImage(GEN_ID, Vector2(311.0, 440.0), "gfx/pfeil_rechts.bmp");
	}
	else
	{
		// Background is x: 200, y: 170, w: 400, h: 300

		// Left arrowkey
		imgui.doOverlay(GEN_ID, Vector2(200.0 + 5.0, 170.0 + 100.0 + 5.0), Vector2(200.0 + 95.0, 170.0 + 295));
		imgui.doImage(GEN_ID, Vector2(250.0, 440.0), "gfx/pfeil_links.bmp");

		// Right arrowkey
		imgui.doOverlay(GEN_ID, Vector2(200.0 + 5.0 + 100.0, 170.0 + 100.0 + 5.0), Vector2(200.0 + 95.0 + 100.0, 170.0 + 295));
		imgui.doImage(GEN_ID, Vector2(350.0, 440.0), "gfx/pfeil_rechts.bmp");
	}

	imgui.doOverlay(GEN_ID, Vector2(200.0 + 5.0 + 250.0, 170.0 + 100.0 + 5.0), Vector2(200.0 + 95.0 + 300.0, 170.0 + 295));
	imgui.doImage(GEN_ID, Vector2(525.0, 440.0), "gfx/pfeil_oben.bmp");


	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), TextManager::LBL_OK))
	{
		save();
		switchState(new OptionState());
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), TextManager::LBL_CANCEL))
	{
		switchState(new OptionState());
	}
}
#endif

const char* InputOptionsState::getStateName() const
{
	return "InputOptionsState";
}

MiscOptionsState::MiscOptionsState()
{
	mOptionConfig.loadFile("config.xml");

	std::string currentBackground = mOptionConfig.getString("background");
	mBackground = -1;

	mBackgrounds = FileSystem::getSingleton().enumerateFiles("backgrounds", ".bmp", true);

	for(unsigned int i = 0; i < mBackgrounds.size(); ++i)
	{
		if (mBackgrounds[i] == currentBackground)
		{
			mBackground = i;
			break;
		}
	}

	std::string currentRules = mOptionConfig.getString("rules");
	currentRules = currentRules.substr(0, currentRules.length() - 4);
	mRule = -1;

	mRules = FileSystem::getSingleton().enumerateFiles("rules", ".lua", false);

	for(unsigned int i = 0; i < mRules.size(); ++i)
	{
		if (mRules[i] == currentRules)
		{
			mRule = i;
			break;
		}
	}

	mShowFPS = mOptionConfig.getBool("showfps");
	mShowBlood = mOptionConfig.getBool("blood");
	mVolume = mOptionConfig.getFloat("global_volume");
	mMute = mOptionConfig.getBool("mute");
	mGameFPS = mOptionConfig.getInteger("gamefps");
	mNetworkSide = mOptionConfig.getInteger("network_side");
	mLanguage = mOptionConfig.getString("language");
}

MiscOptionsState::~MiscOptionsState()
{
}

void MiscOptionsState::save()
{
	mOptionConfig.setBool("showfps", mShowFPS);
	mOptionConfig.setBool("blood", mShowBlood);
	mOptionConfig.setFloat("global_volume", mVolume);
	mOptionConfig.setBool("mute", mMute);
	mOptionConfig.setInteger("gamefps", mGameFPS);
	mOptionConfig.setInteger("network_side", mNetworkSide);
	mOptionConfig.setString("language", mLanguage);
	mOptionConfig.setString("background", mBackgrounds[mBackground]);
	mOptionConfig.setString("rules", mRules[mRule] + ".lua");
	mOptionConfig.saveFile("config.xml");

	SpeedController::getMainInstance()->setDrawFPS(mOptionConfig.getBool("showfps"));
	BloodManager::getSingleton().enable(mOptionConfig.getBool("blood"));
	SoundManager::getSingleton().setVolume(mOptionConfig.getFloat("global_volume"));
	SoundManager::getSingleton().setMute(mOptionConfig.getBool("mute"));
	RenderManager::getSingleton().setBackground(std::string("backgrounds/") + mOptionConfig.getString("background"));
	TextManager::switchLanguage(mOptionConfig.getString("language"));
}

void MiscOptionsState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doText(GEN_ID, Vector2(34.0, 10.0), TextManager::OP_BACKGROUND);
	unsigned tmp = mBackground;
	imgui.doSelectbox(GEN_ID, Vector2(34.0, 40.0), Vector2(400.0, 175.0), mBackgrounds, tmp);
	if (tmp != mBackground)
	{
		mBackground = tmp;
		RenderManager::getSingleton().setBackground(std::string("backgrounds/") + mBackgrounds[mBackground]);
	}

	imgui.doText(GEN_ID, Vector2(34.0, 190.0), TextManager::OP_RULES);
	imgui.doSelectbox(GEN_ID, Vector2(34.0, 220.0), Vector2(400.0, 354.0), mRules, mRule);

	imgui.doText(GEN_ID, Vector2(484.0, 10.0), TextManager::OP_VOLUME);
	if (imgui.doScrollbar(GEN_ID, Vector2(484.0, 50.0), mVolume))
	{
		SoundManager::getSingleton().setVolume(mVolume);
		SoundManager::getSingleton().playSound("sounds/bums.wav", 1.0);
	}
	if (imgui.doButton(GEN_ID, Vector2(531.0, 80.0), TextManager::OP_MUTE))
	{
		mMute = !mMute;
		SoundManager::getSingleton().setMute(mMute);
		if (!mMute)
			SoundManager::getSingleton().playSound("sounds/bums.wav", 1.0);
	}
	if (mMute)
	{
		imgui.doImage(GEN_ID, Vector2(513.0, 92.0), "gfx/pfeil_rechts.bmp");
	}

#if __DESKTOP__
	if (imgui.doButton(GEN_ID, Vector2(484.0, 120.0), TextManager::OP_FPS))
	{
		mShowFPS = !mShowFPS;
		SpeedController::getMainInstance()->setDrawFPS(mShowFPS);
	}
	if (mShowFPS)
	{
		imgui.doImage(GEN_ID, Vector2(466.0, 132.0), "gfx/pfeil_rechts.bmp");
	}
#endif
	if (imgui.doButton(GEN_ID, Vector2(484.0, 160.0), TextManager::OP_BLOOD))
	{
		mShowBlood = !mShowBlood;
		BloodManager::getSingleton().enable(mShowBlood);
		BloodManager::getSingleton().spillBlood(Vector2(484.0, 160.0), 1.5, 2);
	}
	if (mShowBlood)
	{
		imgui.doImage(GEN_ID, Vector2(466.0 ,172.0), "gfx/pfeil_rechts.bmp");
	}

	imgui.doText(GEN_ID, Vector2(434.0, 200.0), TextManager::OP_NETWORK_SIDE);
	if (imgui.doButton(GEN_ID, Vector2(450.0, 240.0), TextManager::OP_LEFT))
		mNetworkSide = 0;
	if (imgui.doButton(GEN_ID, Vector2(630.0, 240.0), TextManager::OP_RIGHT))
		mNetworkSide = 1;
	if (mNetworkSide == 0)
		imgui.doImage(GEN_ID, Vector2(432.0, 252.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(612.0, 252.0), "gfx/pfeil_rechts.bmp");

	imgui.doText(GEN_ID, Vector2(484.0, 290.0), TextManager::OP_SPEED);
	float gamefps = (mGameFPS - 30) / 90.0;
	if (gamefps < 0.0)
		gamefps = 0.0;
	imgui.doScrollbar(GEN_ID, Vector2(440.0, 330.0), gamefps);
		mGameFPS = (int)(gamefps*90.0+30);
	if (imgui.doButton(GEN_ID, Vector2(155.0, 380.0), TextManager::OP_VSLOW))
		mGameFPS = 30;
	if (imgui.doButton(GEN_ID, Vector2(450.0, 380.0), TextManager::OP_SLOW))
		mGameFPS = 60;
	if (imgui.doButton(GEN_ID, Vector2(319.0, 415.0), TextManager::OP_DEFAULT))
		mGameFPS = 75;
	if (imgui.doButton(GEN_ID, Vector2(155.0, 450.0), TextManager::OP_FAST))
		mGameFPS = 90;
	if (imgui.doButton(GEN_ID, Vector2(410.0, 450.0), TextManager::OP_VFAST))
		mGameFPS = 120;

	std::stringstream FPSInPercent;
	FPSInPercent << int((float)mGameFPS/75*100);
	FPSInPercent << "%";
	imgui.doText(GEN_ID, Vector2(660.0, 330.0), FPSInPercent.str());

	//! \todo this must be reworked
	std::map<std::string, std::string>::iterator olang = TextManager::language_names.find(TextManager::getSingleton()->getLang());
	if(++olang == TextManager::language_names.end()){
		olang = TextManager::language_names.begin();
	}
	if (imgui.doButton(GEN_ID, Vector2(300.0, 490.0), (*olang).second)){
		//! \todo autogenerierte liste mit allen lang_ dateien, namen auslesen
		mLanguage = (*olang).first;
		TextManager::switchLanguage(mLanguage);

	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), TextManager::LBL_OK))
	{
		save();
		switchState(new OptionState());
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), TextManager::LBL_CANCEL))
	{
		switchState(new OptionState());
	}
}

const char* MiscOptionsState::getStateName() const
{
	return "MiscOptionsState";
}
