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
	IMGUI::getSingleton().resetSelection();
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

void OptionState::step()
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
#ifndef __ANDROID__
#ifdef __APPLE__
#if MAC_OS_X
	if (imgui.doButton(GEN_ID, Vector2(40.0, 390.0), TextManager::OP_INPUT_OP))
	{
		save();
		deleteCurrentState();
		setCurrentState(new InputOptionsState());
	}
#endif
#else
	if (imgui.doButton(GEN_ID, Vector2(40.0, 390.0), TextManager::OP_INPUT_OP))
	{
		save();
		deleteCurrentState();
		setCurrentState(new InputOptionsState());
	}
#endif
#endif

	if (imgui.doButton(GEN_ID, Vector2(40.0, 430.0), TextManager::OP_GFX_OP))
	{
		save();
		deleteCurrentState();
		setCurrentState(new GraphicOptionsState());
	}
	if (imgui.doButton(GEN_ID, Vector2(40.0, 470.0), TextManager::OP_MISC))
	{
		save();
		deleteCurrentState();
		setCurrentState(new MiscOptionsState());
	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), TextManager::LBL_OK))
	{
		save();
		deleteCurrentState();
		setCurrentState(new MainMenuState());
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), TextManager::LBL_CANCEL))
	{
		deleteCurrentState();
		setCurrentState(new MainMenuState());
	}
}

const char* OptionState::getStateName() const
{
	return "OptionState";
}

GraphicOptionsState::GraphicOptionsState()
{
	IMGUI::getSingleton().resetSelection();
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
#ifndef __ANDROID__
#ifdef __APPLE__
#if MAC_OS_X
	if ((mOptionConfig.getBool("fullscreen") != mFullscreen) ||
			(mOptionConfig.getString("device") != mRenderer))
	{
		mOptionConfig.setBool("fullscreen", mFullscreen);
		mOptionConfig.setString("device", mRenderer);
		if (mRenderer == "OpenGL")
			RenderManager::createRenderManagerGL2D()->init(800, 600, mFullscreen);
		else
			RenderManager::createRenderManagerSDL()->init(800, 600, mFullscreen);
		RenderManager::getSingleton().setBackground(
			std::string("backgrounds/") +
			mOptionConfig.getString("background"));
	}
#endif
#else
	if ((mOptionConfig.getBool("fullscreen") != mFullscreen) ||
        (mOptionConfig.getString("device") != mRenderer))
	{
		mOptionConfig.setBool("fullscreen", mFullscreen);
		mOptionConfig.setString("device", mRenderer);
		if (mRenderer == "OpenGL")
			RenderManager::createRenderManagerGL2D()->init(800, 600, mFullscreen);
		else
			RenderManager::createRenderManagerSDL()->init(800, 600, mFullscreen);
		RenderManager::getSingleton().setBackground(
                                                    std::string("backgrounds/") +
                                                    mOptionConfig.getString("background"));
	}
#endif
#endif

	if(mOptionConfig.getBool("show_shadow") != mShowShadow)
	{
		RenderManager::getSingleton().showShadow(mShowShadow);
		mOptionConfig.setBool("show_shadow", mShowShadow);
	}

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

void GraphicOptionsState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

#ifndef __ANDROID__
#ifdef __APPLE__
#if MAC_OS_X
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
#else
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
#endif
	float heightOfElement = 110.0;

#ifdef __ANDROID__
	heightOfElement = 10.0;
#else
#ifdef __APPLE__
#if !MAC_OS_X
	heightOfElement = 10.0;
#endif
#endif
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

#ifdef __ANDROID__
	heightOfElement += 50.0;
#else
#ifdef __APPLE__
#if !MAC_OS_X
	heightOfElement += 50.0;
#else
	heightOfElement += 30.0;
#endif
#else
	heightOfElement += 30.0;
#endif
#endif

	//Blob colors:
	imgui.doText(GEN_ID, Vector2(280.0, heightOfElement), TextManager::OP_BLOB_COLORS);
	heightOfElement += 40.0;
	//left blob:
	imgui.doText(GEN_ID, Vector2(34.0, heightOfElement), TextManager::OP_LEFT_PLAYER);
#ifdef __ANDROID__
	heightOfElement += 50.0;
#else
#ifdef __APPLE__
#if !MAC_OS_X
	heightOfElement += 50.0;
#else
	heightOfElement += 30.0;
#endif
#else
	heightOfElement += 30.0;
#endif
#endif
	{
		imgui.doText(GEN_ID, Vector2(34.0, heightOfElement), TextManager::OP_RED);
		float r1 = (float)mR1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, heightOfElement), r1);
		mR1 = (int)(r1*255);
	}
#ifdef __ANDROID__
	heightOfElement += 50.0;
#else
#ifdef __APPLE__
#if !MAC_OS_X
	heightOfElement += 50.0;
#else
	heightOfElement += 30.0;
#endif
#else
	heightOfElement += 30.0;
#endif
#endif
	{
		imgui.doText(GEN_ID, Vector2(34.0, heightOfElement), TextManager::OP_GREEN);
		float g1 = (float)mG1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, heightOfElement), g1);
		mG1 = (int)(g1*255);
	}
#ifdef __ANDROID__
	heightOfElement += 50.0;
#else
#ifdef __APPLE__
#if !MAC_OS_X
	heightOfElement += 50.0;
#else
	heightOfElement += 30.0;
#endif
#else
	heightOfElement += 30.0;
#endif
#endif
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
#ifdef __ANDROID__
	heightOfElement = 160.0;
#else
#ifdef __APPLE__
#if !MAC_OS_X
	heightOfElement = 160.0;
#else
	heightOfElement = 230.0;
#endif
#else
	heightOfElement = 230.0;
#endif
#endif

	imgui.doText(GEN_ID, Vector2(434.0, heightOfElement), TextManager::OP_RIGHT_PLAYER);
#ifdef __ANDROID__
	heightOfElement += 50.0;
#else
#ifdef __APPLE__
#if !MAC_OS_X
	heightOfElement += 50.0;
#else
	heightOfElement += 30.0;
#endif
#else
	heightOfElement += 30.0;
#endif
#endif
	{
		imgui.doText(GEN_ID, Vector2(434.0, heightOfElement), TextManager::OP_RED);
		float r2 = (float)mR2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, heightOfElement), r2);
		mR2 = (int)(r2*255);
	}
#ifdef __ANDROID__
	heightOfElement += 50.0;
#else
#ifdef __APPLE__
#if !MAC_OS_X
	heightOfElement += 50.0;
#else
	heightOfElement += 30.0;
#endif
#else
	heightOfElement += 30.0;
#endif
#endif
	{
		imgui.doText(GEN_ID, Vector2(434.0, heightOfElement), TextManager::OP_GREEN);
		float g2 = (float)mG2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, heightOfElement), g2);
		mG2 = (int)(g2*255);
	}
#ifdef __ANDROID__
	heightOfElement += 50.0;
#else
#ifdef __APPLE__
#if !MAC_OS_X
	heightOfElement += 50.0;
#else
	heightOfElement += 30.0;
#endif
#else
	heightOfElement += 30.0;
#endif
#endif
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
		deleteCurrentState();
		setCurrentState(new OptionState());
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), TextManager::LBL_CANCEL))
	{
		deleteCurrentState();
		setCurrentState(new OptionState());
	}
}

const char* GraphicOptionsState::getStateName() const
{
	return "GraphicOptionsState";
}

InputOptionsState::InputOptionsState()
{
	IMGUI::getSingleton().resetSelection();
	mSetKeyboard = 0;
	mOptionConfig.loadFile("inputconfig.xml");
	//left data:
	mLeftBlobbyDevice = mOptionConfig.getString("left_blobby_device");
	mLeftBlobbyMouseJumpbutton = mOptionConfig.getInteger("left_blobby_mouse_jumpbutton");
	mLeftBlobbyKeyboardLeft = mOptionConfig.getString("left_blobby_keyboard_left");
	mLeftBlobbyKeyboardRight = mOptionConfig.getString("left_blobby_keyboard_right");
	mLeftBlobbyKeyboardJump = mOptionConfig.getString("left_blobby_keyboard_jump");
	mLeftBlobbyJoystickLeft = mOptionConfig.getString("left_blobby_joystick_left");
	mLeftBlobbyJoystickRight = mOptionConfig.getString("left_blobby_joystick_right");
	mLeftBlobbyJoystickJump = mOptionConfig.getString("left_blobby_joystick_jump");
	//right data:
	mRightBlobbyDevice = mOptionConfig.getString("right_blobby_device");
	mRightBlobbyMouseJumpbutton = mOptionConfig.getInteger("right_blobby_mouse_jumpbutton");
	mRightBlobbyKeyboardLeft = mOptionConfig.getString("right_blobby_keyboard_left");
	mRightBlobbyKeyboardRight = mOptionConfig.getString("right_blobby_keyboard_right");
	mRightBlobbyKeyboardJump = mOptionConfig.getString("right_blobby_keyboard_jump");
	mRightBlobbyJoystickLeft = mOptionConfig.getString("right_blobby_joystick_left");
	mRightBlobbyJoystickRight = mOptionConfig.getString("right_blobby_joystick_right");
	mRightBlobbyJoystickJump = mOptionConfig.getString("right_blobby_joystick_jump");
}

InputOptionsState::~InputOptionsState()
{
}

void InputOptionsState::save()
{
	//left data:
	mOptionConfig.setString("left_blobby_device", mLeftBlobbyDevice);
	mOptionConfig.setInteger("left_blobby_mouse_jumpbutton", mLeftBlobbyMouseJumpbutton);
	mOptionConfig.setString("left_blobby_keyboard_left", mLeftBlobbyKeyboardLeft);
	mOptionConfig.setString("left_blobby_keyboard_right", mLeftBlobbyKeyboardRight);
	mOptionConfig.setString("left_blobby_keyboard_jump", mLeftBlobbyKeyboardJump);
	mOptionConfig.setString("left_blobby_joystick_left", mLeftBlobbyJoystickLeft);
	mOptionConfig.setString("left_blobby_joystick_right", mLeftBlobbyJoystickRight);
	mOptionConfig.setString("left_blobby_joystick_jump", mLeftBlobbyJoystickJump);
	//right data:
	mOptionConfig.setString("right_blobby_device", mRightBlobbyDevice);
	mOptionConfig.setInteger("right_blobby_mouse_jumpbutton", mRightBlobbyMouseJumpbutton);
	mOptionConfig.setString("right_blobby_keyboard_left", mRightBlobbyKeyboardLeft);
	mOptionConfig.setString("right_blobby_keyboard_right", mRightBlobbyKeyboardRight);
	mOptionConfig.setString("right_blobby_keyboard_jump", mRightBlobbyKeyboardJump);
	mOptionConfig.setString("right_blobby_joystick_left", mRightBlobbyJoystickLeft);
	mOptionConfig.setString("right_blobby_joystick_right", mRightBlobbyJoystickRight);
	mOptionConfig.setString("right_blobby_joystick_jump", mRightBlobbyJoystickJump);

	mOptionConfig.saveFile("inputconfig.xml");
}

void InputOptionsState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	std::string lastActionKey = InputManager::getSingleton()->getLastActionKey();

	// left player side:
	imgui.doText(GEN_ID, Vector2(34.0, 10.0), TextManager::OP_LEFT_PLAYER);
	// dirty hack for languagesupport
	TextManager::STRING deviceLanguage = TextManager::OP_JOYSTICK;
	if (mLeftBlobbyDevice[0] == 'k')
		deviceLanguage = TextManager::OP_KEYBOARD;
	else if (mLeftBlobbyDevice[0] == 'm')
		deviceLanguage = TextManager::OP_MOUSE;

	if (imgui.doButton(GEN_ID, Vector2(80.0, 60.0), deviceLanguage))
	{
		if (mLeftBlobbyDevice == "mouse")
		{
			mLeftBlobbyDevice = "keyboard";
		}
		else if (mLeftBlobbyDevice == "keyboard")
		{
			mLeftBlobbyDevice = "joystick";
		}
		else if (mLeftBlobbyDevice == "joystick")
		{
			if (mRightBlobbyDevice != "mouse")
			{
				mLeftBlobbyDevice = "mouse";
			}
			else
			{
				mLeftBlobbyDevice = "keyboard";
			}
		}
	}
	//if mouse device is selected:
	if (mLeftBlobbyDevice == "mouse")
	{
		imgui.doText(GEN_ID, Vector2(34.0, 120.0), TextManager::OP_JUMP_BUTTON);
		std::ostringstream text;
		if (mLeftBlobbyMouseJumpbutton >= 0)
			text << "Button " << mLeftBlobbyMouseJumpbutton;
		else
			text << "Button ";
		if (imgui.doButton(GEN_ID, Vector2(50, 150.0), text.str()))
		{
			mOldInteger = mLeftBlobbyMouseJumpbutton;
			mLeftBlobbyMouseJumpbutton = -2;
		}
	}
	if ((mLeftBlobbyMouseJumpbutton == -2) && (InputManager::getSingleton()->getLastMouseButton() == -1))
		mLeftBlobbyMouseJumpbutton = -1;
	//if keyboard device is selected:
	if (mLeftBlobbyDevice == "keyboard")
	{

		if (imgui.doButton(GEN_ID, Vector2(34, 350.0), TextManager::OP_SET_ALL))
			mSetKeyboard = 1;

		imgui.doText(GEN_ID, Vector2(34.0, 120.0), TextManager::OP_LEFT_KEY);
		if (imgui.doButton(GEN_ID, Vector2(50, 150.0), std::string("Key ")+mLeftBlobbyKeyboardLeft) || mSetKeyboard == 1)
		{
			lastActionKey = "";
			oldString = mLeftBlobbyKeyboardLeft;
			mLeftBlobbyKeyboardLeft = "";
		}

		if (mSetKeyboard == 1)
			mSetKeyboard = 2;

		if (mSetKeyboard == 2 && mLeftBlobbyKeyboardLeft != "")
			mSetKeyboard = 3;

		imgui.doText(GEN_ID, Vector2(34.0, 190.0), TextManager::OP_RIGHT_KEY);
		if (imgui.doButton(GEN_ID, Vector2(50, 220.0), std::string("Key ")+mLeftBlobbyKeyboardRight) || mSetKeyboard == 3)
		{
			lastActionKey = "";
			oldString = mLeftBlobbyKeyboardRight;
			mLeftBlobbyKeyboardRight = "";
		}

		if (mSetKeyboard == 3)
			mSetKeyboard = 4;

		if (mSetKeyboard == 4 && mLeftBlobbyKeyboardRight != "")
			mSetKeyboard = 5;

		imgui.doText(GEN_ID, Vector2(34.0, 260.0), TextManager::OP_JUMP_KEY );
		if (imgui.doButton(GEN_ID, Vector2(50, 290.0), std::string("Key ")+mLeftBlobbyKeyboardJump) || mSetKeyboard == 5)
		{
			lastActionKey = "";
			oldString = mLeftBlobbyKeyboardJump;
			mLeftBlobbyKeyboardJump = "";
		}

		if (mSetKeyboard == 5)
			mSetKeyboard = 6;

		if (mSetKeyboard == 6 && mLeftBlobbyKeyboardJump != "")
			mSetKeyboard = 0;
	}
	//if joystick device is selected:
	if (mLeftBlobbyDevice == "joystick")
	{
		imgui.doText(GEN_ID, Vector2(34.0, 120.0), TextManager::OP_LEFT_BUTTON);
		if (imgui.doButton(GEN_ID, Vector2(50, 150.0), mLeftBlobbyJoystickLeft))
		{
			oldString = mLeftBlobbyJoystickLeft;
			mLeftBlobbyJoystickLeft = "";
		}
		imgui.doText(GEN_ID, Vector2(34.0, 190.0), TextManager::OP_RIGHT_BUTTON);
		if (imgui.doButton(GEN_ID, Vector2(50, 220.0), mLeftBlobbyJoystickRight))
		{
			oldString = mLeftBlobbyJoystickRight;
			mLeftBlobbyJoystickRight = "";
		}
		imgui.doText(GEN_ID, Vector2(34.0, 260.0), TextManager::OP_JUMP_BUTTON);
		if (imgui.doButton(GEN_ID, Vector2(50, 290.0), mLeftBlobbyJoystickJump))
		{
			oldString = mLeftBlobbyJoystickJump;
			mLeftBlobbyJoystickJump = "";
		}
	}

	//right player side:
	imgui.doText(GEN_ID, Vector2(434.0, 10.0), TextManager::OP_RIGHT_PLAYER);
	// dirty hack for languagesupport
	deviceLanguage = TextManager::OP_JOYSTICK;
	if (mRightBlobbyDevice[0] == 'k')
		deviceLanguage = TextManager::OP_KEYBOARD;
	else if (mRightBlobbyDevice[0] == 'm')
		deviceLanguage = TextManager::OP_MOUSE;

	if (imgui.doButton(GEN_ID, Vector2(480.0, 60.0), deviceLanguage))
	{
		if (mRightBlobbyDevice == "mouse")
		{
			mRightBlobbyDevice = "keyboard";
		}
		else if (mRightBlobbyDevice == "keyboard")
		{
			mRightBlobbyDevice = "joystick";
		}
		else if (mRightBlobbyDevice == "joystick")
		{
			if (mLeftBlobbyDevice != "mouse")
			{
				mRightBlobbyDevice = "mouse";
			}
			else
			{
				mRightBlobbyDevice = "keyboard";
			}
		}
	}
	//if mouse device is selected:
	if (mRightBlobbyDevice == "mouse")
	{
		imgui.doText(GEN_ID, Vector2(434.0, 120.0), TextManager::OP_JUMP_BUTTON);
		std::ostringstream text;
		if (mRightBlobbyMouseJumpbutton >= 0)
			text << "Button " << mRightBlobbyMouseJumpbutton;
		else
			text << "Button ";
		if (imgui.doButton(GEN_ID, Vector2(450, 150.0), text.str()))
		{
			mOldInteger = mRightBlobbyMouseJumpbutton;
			mRightBlobbyMouseJumpbutton = -2;
		}
	}
	if ((mRightBlobbyMouseJumpbutton == -2) && (InputManager::getSingleton()->getLastMouseButton() == -1))
		mRightBlobbyMouseJumpbutton = -1;
	//if keyboard device is selected:
	if (mRightBlobbyDevice == "keyboard")
	{

		if (imgui.doButton(GEN_ID, Vector2(434.0, 350.0), TextManager::OP_SET_ALL))
			mSetKeyboard = 11;

		imgui.doText(GEN_ID, Vector2(434.0, 120.0), TextManager::OP_LEFT_KEY);
		if (imgui.doButton(GEN_ID, Vector2(450, 150.0), std::string("Key ")+mRightBlobbyKeyboardLeft) || mSetKeyboard == 11)
		{
			lastActionKey = "";
			oldString = mRightBlobbyKeyboardLeft;
			mRightBlobbyKeyboardLeft = "";
		}

		if (mSetKeyboard == 11)
			mSetKeyboard = 12;

		if (mSetKeyboard == 12 && mRightBlobbyKeyboardLeft != "")
			mSetKeyboard = 13;

		imgui.doText(GEN_ID, Vector2(434.0, 190.0), TextManager::OP_RIGHT_KEY);
		if (imgui.doButton(GEN_ID, Vector2(450, 220.0), std::string("Key ")+mRightBlobbyKeyboardRight) || mSetKeyboard == 13)
		{
			lastActionKey = "";
			oldString = mRightBlobbyKeyboardRight;
			mRightBlobbyKeyboardRight = "";
		}

		if (mSetKeyboard == 13)
			mSetKeyboard = 14;

		if (mSetKeyboard == 14 && mRightBlobbyKeyboardRight != "")
			mSetKeyboard = 15;

		imgui.doText(GEN_ID, Vector2(434.0, 260.0), TextManager::OP_JUMP_KEY);
		if (imgui.doButton(GEN_ID, Vector2(450, 290.0), std::string("Key ")+mRightBlobbyKeyboardJump) || mSetKeyboard == 15)
		{
			lastActionKey = "";
			oldString = mRightBlobbyKeyboardJump;
			mRightBlobbyKeyboardJump = "";
		}

		if (mSetKeyboard == 15)
			mSetKeyboard = 16;

		if (mSetKeyboard == 16 && mRightBlobbyKeyboardJump != "")
			mSetKeyboard = 17;
	}
	//if joystick device is selected:
	if (mRightBlobbyDevice == "joystick")
	{
		imgui.doText(GEN_ID, Vector2(434.0, 120.0), TextManager::OP_LEFT_BUTTON);
		if (imgui.doButton(GEN_ID, Vector2(450, 150.0), mRightBlobbyJoystickLeft))
		{
			oldString = mRightBlobbyJoystickLeft;
			mRightBlobbyJoystickLeft = "";
		}
		imgui.doText(GEN_ID, Vector2(434.0, 190.0), TextManager::OP_RIGHT_BUTTON);
		if (imgui.doButton(GEN_ID, Vector2(450, 220.0), mRightBlobbyJoystickRight))
		{
			oldString = mRightBlobbyJoystickRight;
			mRightBlobbyJoystickRight = "";
		}
		imgui.doText(GEN_ID, Vector2(434.0, 260.0), TextManager::OP_JUMP_BUTTON);
		if (imgui.doButton(GEN_ID, Vector2(450, 290.0), mRightBlobbyJoystickJump))
		{
			oldString = mRightBlobbyJoystickJump;
			mRightBlobbyJoystickJump = "";
		}
	}

	//check if a capture window is open, to set all widgets inactive:
	if (mLeftBlobbyKeyboardLeft != "" && mLeftBlobbyKeyboardRight != "" && mLeftBlobbyKeyboardJump != "" && mLeftBlobbyJoystickLeft != "" && mLeftBlobbyJoystickRight != "" && mLeftBlobbyJoystickJump != "" && mLeftBlobbyMouseJumpbutton != -1 && mRightBlobbyKeyboardLeft != "" && mRightBlobbyKeyboardRight != "" && mRightBlobbyKeyboardJump != "" && mRightBlobbyJoystickLeft != "" && mRightBlobbyJoystickRight != "" && mRightBlobbyJoystickJump != "" && mRightBlobbyMouseJumpbutton != -1)
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
	if (mLeftBlobbyMouseJumpbutton == -1)
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(180.0, 250.0), TextManager::OP_PRESS_MOUSE_BUTTON);
		imgui.doText(GEN_ID, Vector2(290.0, 300.0), TextManager::OP_JUMPING);
		mLeftBlobbyMouseJumpbutton = InputManager::getSingleton()->getLastMouseButton();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyMouseJumpbutton = mOldInteger;
	}
	if (mLeftBlobbyKeyboardLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_KEY_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_MOVING_LEFT);
		mLeftBlobbyKeyboardLeft = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyKeyboardLeft = oldString;
	}
	if (mLeftBlobbyKeyboardRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_KEY_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_MOVING_RIGHT);
		mLeftBlobbyKeyboardRight = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyKeyboardRight = oldString;
	}
	if (mLeftBlobbyKeyboardJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_MOVING_LEFT);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_JUMPING);
		mLeftBlobbyKeyboardJump = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyKeyboardJump = oldString;
	}
	if (mLeftBlobbyJoystickLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_BUTTON_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_MOVING_LEFT);
		mLeftBlobbyJoystickLeft = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyJoystickLeft = oldString;
	}
	if (mLeftBlobbyJoystickRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_BUTTON_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_MOVING_RIGHT);
		mLeftBlobbyJoystickRight = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyJoystickRight = oldString;
	}
	if (mLeftBlobbyJoystickJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_BUTTON_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_JUMPING);
		mLeftBlobbyJoystickJump = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyJoystickJump = oldString;
	}
	if (mRightBlobbyMouseJumpbutton == -1)
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(180.0, 250.0), TextManager::OP_PRESS_MOUSE_BUTTON);
		imgui.doText(GEN_ID, Vector2(290.0, 300.0), TextManager::OP_JUMPING);
		mRightBlobbyMouseJumpbutton = InputManager::getSingleton()->getLastMouseButton();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyMouseJumpbutton = mOldInteger;
	}
	if (mRightBlobbyKeyboardLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_KEY_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_MOVING_LEFT);
		mRightBlobbyKeyboardLeft = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mRightBlobbyKeyboardLeft = oldString;
	}
	if (mRightBlobbyKeyboardRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_KEY_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_MOVING_RIGHT);
		mRightBlobbyKeyboardRight = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mRightBlobbyKeyboardRight = oldString;
	}
	if (mRightBlobbyKeyboardJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_KEY_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_JUMPING);
		mRightBlobbyKeyboardJump = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mRightBlobbyKeyboardJump = oldString;
	}
	if (mRightBlobbyJoystickLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_BUTTON_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_MOVING_LEFT);
		mRightBlobbyJoystickLeft = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyJoystickLeft = oldString;
	}
	if (mRightBlobbyJoystickRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_BUTTON_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_MOVING_RIGHT);
		mRightBlobbyJoystickRight = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyJoystickRight = oldString;
	}
	if (mRightBlobbyJoystickJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), TextManager::OP_PRESS_BUTTON_FOR);
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), TextManager::OP_JUMPING);
		mRightBlobbyJoystickJump = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyJoystickJump = oldString;
	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), TextManager::LBL_OK))
	{
		save();
		deleteCurrentState();
		setCurrentState(new OptionState());
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), TextManager::LBL_CANCEL))
	{
		deleteCurrentState();
		setCurrentState(new OptionState());
	}
}

const char* InputOptionsState::getStateName() const
{
	return "InputOptionsState";
}

MiscOptionsState::MiscOptionsState()
{
	IMGUI::getSingleton().resetSelection();

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
	if (mBackground > -1)
		mOptionConfig.setString("background", mBackgrounds[mBackground]);
	if (mRule > -1)
		mOptionConfig.setString("rules", mRules[mRule] + ".lua");
	mOptionConfig.saveFile("config.xml");

	SpeedController::getMainInstance()->setDrawFPS(mOptionConfig.getBool("showfps"));
	BloodManager::getSingleton().enable(mOptionConfig.getBool("blood"));
	SoundManager::getSingleton().setVolume(mOptionConfig.getFloat("global_volume"));
	SoundManager::getSingleton().setMute(mOptionConfig.getBool("mute"));
	RenderManager::getSingleton().setBackground(std::string("backgrounds/") + mOptionConfig.getString("background"));
	TextManager::switchLanguage(mOptionConfig.getString("language"));
}

void MiscOptionsState::step()
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
		if (mBackground > -1)
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

#ifndef __ANDROID__
#ifdef __APPLE__
#if MAC_OS_X
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
#else
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

	//! \todo this must be reworket
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
		deleteCurrentState();
		setCurrentState(new OptionState());
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), TextManager::LBL_CANCEL))
	{
		deleteCurrentState();
		setCurrentState(new OptionState());
	}
}

const char* MiscOptionsState::getStateName() const
{
	return "MiscOptionsState";
}
