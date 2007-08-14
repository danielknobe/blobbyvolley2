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

#include "State.h"
#include "OptionsState.h"
#include "RenderManager.h"
#include "LocalInputSource.h"
#include "SpeedController.h"
#include "SoundManager.h"
#include "Blood.h"
#include "IMGUI.h"

#include <physfs.h>
#include <sstream>

OptionState::OptionState()
{
	IMGUI::getSingleton().resetSelection();
	mSaveConfig = false;
	mOptionConfig.loadFile("config.xml");
	mPlayerOptions[LEFT_PLAYER] = 0;
	mPlayerOptions[RIGHT_PLAYER] = 0;
	mScriptNames.push_back("Human");
	std::string leftScript = mOptionConfig.getString("left_script_name");
	std::string rightScript = mOptionConfig.getString("right_script_name");
	char** filenames = PHYSFS_enumerateFiles("scripts");
	for (int i = 0; filenames[i] != 0; ++i)
	{
		std::string tmp(filenames[i]);
		if (tmp.find(".lua") != std::string::npos)
		{
			mScriptNames.push_back(tmp);
			int pos = mScriptNames.size() - 1;
			if (tmp == leftScript)
				mPlayerOptions[LEFT_PLAYER] = pos;
			if (tmp == rightScript)
				mPlayerOptions[RIGHT_PLAYER] = pos;
		}

	}
	if (mOptionConfig.getBool("left_player_human"))
		mPlayerOptions[LEFT_PLAYER] = 0;
	if (mOptionConfig.getBool("right_player_human"))
		mPlayerOptions[RIGHT_PLAYER] = 0;
	PHYSFS_freeList(filenames);
	mPlayerName[LEFT_PLAYER] = mOptionConfig.getString("left_player_name");
	mPlayerName[RIGHT_PLAYER] = mOptionConfig.getString("right_player_name");
	mPlayerNamePosition[RIGHT_PLAYER] = 0;
	mPlayerNamePosition[LEFT_PLAYER] = 0;
}

OptionState::~OptionState()
{
	if (mSaveConfig)
	{
		if (mPlayerOptions[LEFT_PLAYER] == 0)
		{
			mOptionConfig.setBool(
				"left_player_human", true);
		}
		else
		{
			mOptionConfig.setBool(
				"left_player_human", false);
			mOptionConfig.setString("left_script_name",
				mScriptNames[mPlayerOptions[LEFT_PLAYER]]);
		}

		if (mPlayerOptions[RIGHT_PLAYER] == 0)
		{
			mOptionConfig.setBool(
				"right_player_human", true);
		}
		else
		{
			mOptionConfig.setBool(
				"right_player_human", false);
			mOptionConfig.setString("right_script_name",
				mScriptNames[mPlayerOptions[RIGHT_PLAYER]]);
		}
		mOptionConfig.setString("left_player_name", mPlayerName[LEFT_PLAYER]);
		mOptionConfig.setString("right_player_name", mPlayerName[RIGHT_PLAYER]);
		mOptionConfig.saveFile("config.xml");
	}
}

void OptionState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doEditbox(GEN_ID, Vector2(5.0, 10.0), 15, mPlayerName[LEFT_PLAYER], mPlayerNamePosition[LEFT_PLAYER]);
	imgui.doEditbox(GEN_ID, Vector2(425.0, 10.0), 15, mPlayerName[RIGHT_PLAYER], mPlayerNamePosition[RIGHT_PLAYER]);

	imgui.doSelectbox(GEN_ID, Vector2(5.0, 50.0), Vector2(375.0, 300.0), mScriptNames, mPlayerOptions[LEFT_PLAYER]);
	imgui.doSelectbox(GEN_ID, Vector2(425.0, 50.0), Vector2(795.0, 300.0), mScriptNames, mPlayerOptions[RIGHT_PLAYER]);

	if (imgui.doButton(GEN_ID, Vector2(40.0, 360.0), "input options"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new InputOptionsState();
		return;
	}
	if (imgui.doButton(GEN_ID, Vector2(40.0, 400.0), "graphic options"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new GraphicOptionsState();
		return;
	}
	if (imgui.doButton(GEN_ID, Vector2(40.0, 440.0), "misc options"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new MiscOptionsState();
		return;
	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), "ok"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new MainMenuState();
		return;
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), "cancel"))
	{
		delete this;
		mCurrentState = new MainMenuState();
	}
}

GraphicOptionsState::GraphicOptionsState()
{
	IMGUI::getSingleton().resetSelection();
	mSaveConfig = false;
	mOptionConfig.loadFile("config.xml");
	mFullscreen = mOptionConfig.getBool("fullscreen");
	mRenderer = mOptionConfig.getString("device");
	mR1 = mOptionConfig.getInteger("r1");
	mG1 = mOptionConfig.getInteger("g1");
	mB1 = mOptionConfig.getInteger("b1");
	mR2 = mOptionConfig.getInteger("r2");
	mG2 = mOptionConfig.getInteger("g2");
	mB2 = mOptionConfig.getInteger("b2");
	mLeftMorphing = mOptionConfig.getBool("left_blobby_oscillate");
	mRightMorphing = mOptionConfig.getBool("right_blobby_oscillate");
}

GraphicOptionsState::~GraphicOptionsState()
{
	if (mSaveConfig)
	{
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
		mOptionConfig.setInteger("r1", mR1);
		mOptionConfig.setInteger("g1", mG1);
		mOptionConfig.setInteger("b1", mB1);
		mOptionConfig.setInteger("r2", mR2);
		mOptionConfig.setInteger("g2", mG2);
		mOptionConfig.setInteger("b2", mB2);
		mOptionConfig.setBool("left_blobby_oscillate", mLeftMorphing);
		mOptionConfig.setBool("right_blobby_oscillate", mRightMorphing);

		mOptionConfig.saveFile("config.xml");
	}
}

void GraphicOptionsState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doText(GEN_ID, Vector2(34.0, 50.0), "Video Settings");

	if (imgui.doButton(GEN_ID, Vector2(34.0, 80.0), "Fullscreen Mode"))
		mFullscreen = true;
	if (imgui.doButton(GEN_ID, Vector2(34.0, 110.0), "Window Mode"))
		mFullscreen = false;
	if (mFullscreen)
		imgui.doImage(GEN_ID, Vector2(18.0, 92.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(18.0, 122.0), "gfx/pfeil_rechts.bmp");

	imgui.doText(GEN_ID, Vector2(444.0, 50.0), "Render Device");
	if (imgui.doButton(GEN_ID, Vector2(444.0, 80.0), "OpenGL"))
		mRenderer = "OpenGL";
	if (imgui.doButton(GEN_ID, Vector2(444.0, 110.0), "SDL"))
		mRenderer = "SDL";
	if (mRenderer == "OpenGL")
		imgui.doImage(GEN_ID, Vector2(428.0, 92.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(428.0, 122.0), "gfx/pfeil_rechts.bmp");

	//Blob colors:
	imgui.doText(GEN_ID, Vector2(280.0, 170.0), "blob colors");
	//left blob:
	imgui.doText(GEN_ID, Vector2(34.0, 220.0), "left player");
	{
		imgui.doText(GEN_ID, Vector2(34.0, 250), "red");
		float r1 = (float)mR1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, 250.0), r1);
		mR1 = (int)(r1*255);
	}
	{
		imgui.doText(GEN_ID, Vector2(34.0, 280), "green");
		float g1 = (float)mG1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, 280.0), g1);
		mG1 = (int)(g1*255);
	}
	{
		imgui.doText(GEN_ID, Vector2(34.0, 310), "blue");
		float b1 = (float)mB1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, 310), b1);
		mB1 = (int)(b1*255);
	}
	imgui.doText(GEN_ID, Vector2(34.0, 350), "morphing blob?");
	if (imgui.doButton(GEN_ID, Vector2(72.0, 380), "yes"))
		mLeftMorphing = true;
	if (imgui.doButton(GEN_ID, Vector2(220.0, 380), "no"))
		mLeftMorphing = false;
	if (mLeftMorphing)
		imgui.doImage(GEN_ID, Vector2(54.0, 392.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(204.0, 392.0), "gfx/pfeil_rechts.bmp");
	//draw left blob:
	{
		float time = float(SDL_GetTicks()) / 1000.0;
		Color ourCol = Color(mR1, mG1, mB1);
		if (mLeftMorphing)
			ourCol = Color(int((sin(time*2) + 1.0) * 128),
							int((sin(time*4) + 1.0) * 128),
							int((sin(time*3) + 1.0) * 128));
		imgui.doBlob(GEN_ID, Vector2(120, 490), ourCol);
	}

	//right blob:
	imgui.doText(GEN_ID, Vector2(434.0, 220.0), "right player");
	{
		imgui.doText(GEN_ID, Vector2(434.0, 250), "red");
		float r2 = (float)mR2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, 250.0), r2);
		mR2 = (int)(r2*255);
	}
	{
		imgui.doText(GEN_ID, Vector2(434.0, 280), "green");
		float g2 = (float)mG2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, 280.0), g2);
		mG2 = (int)(g2*255);
	}
	{
		imgui.doText(GEN_ID, Vector2(434.0, 310), "blue");
		float b2 = (float)mB2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, 310), b2);
		mB2 = (int)(b2*255);
	}
	imgui.doText(GEN_ID, Vector2(434.0, 350), "morphing blob?");
	if (imgui.doButton(GEN_ID, Vector2(472.0, 380), "yes"))
		mRightMorphing = true;
	if (imgui.doButton(GEN_ID, Vector2(620.0, 380), "no"))
		mRightMorphing = false;
	if (mRightMorphing)
		imgui.doImage(GEN_ID, Vector2(454.0, 392.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(604.0, 392.0), "gfx/pfeil_rechts.bmp");
	//draw right blob:
	{
		float time = float(SDL_GetTicks()) / 1000.0;
		Color ourCol = Color(mR2, mG2, mB2);
		if (mRightMorphing)
			ourCol = Color(int((cos(time*2) + 1.0) * 128),
							int((cos(time*4) + 1.0) * 128),
							int((cos(time*3) + 1.0) * 128));
		imgui.doBlob(GEN_ID, Vector2(660, 490), ourCol);
	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), "ok"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new OptionState();
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), "cancel"))
	{
		delete this;
		mCurrentState = new OptionState();
	}
}

InputOptionsState::InputOptionsState()
{
	IMGUI::getSingleton().resetSelection();
	mSaveConfig = false;
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
	if (mSaveConfig)
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
}

void InputOptionsState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	std::string lastActionKey = InputManager::getSingleton()->getLastActionKey();

	//left player side:
	imgui.doText(GEN_ID, Vector2(34.0, 10.0), "left player");
	if (imgui.doButton(GEN_ID, Vector2(80.0, 60.0), mLeftBlobbyDevice))
	{
		if (mLeftBlobbyDevice == "mouse")
			mLeftBlobbyDevice = "keyboard";
		else if (mLeftBlobbyDevice == "keyboard")
			mLeftBlobbyDevice = "joystick";
		else if (mLeftBlobbyDevice == "joystick")
			if (mRightBlobbyDevice != "mouse")
				mLeftBlobbyDevice = "mouse";
			else
				mLeftBlobbyDevice = "keyboard";
	}
	//if mouse device is selected:
	if (mLeftBlobbyDevice == "mouse")
	{
		imgui.doText(GEN_ID, Vector2(34.0, 120.0), "Jump Button");
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

		if (imgui.doButton(GEN_ID, Vector2(34, 350.0), "Set All"))
			mSetKeyboard = 1;

		imgui.doText(GEN_ID, Vector2(34.0, 120.0), "Left Key");
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

		imgui.doText(GEN_ID, Vector2(34.0, 190.0), "Right Key");
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

		imgui.doText(GEN_ID, Vector2(34.0, 260.0), "Jump Key");
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
		imgui.doText(GEN_ID, Vector2(34.0, 120.0), "Left Button");
		if (imgui.doButton(GEN_ID, Vector2(50, 150.0), mLeftBlobbyJoystickLeft))
		{
			oldString = mLeftBlobbyJoystickLeft;
			mLeftBlobbyJoystickLeft = "";
		}
		imgui.doText(GEN_ID, Vector2(34.0, 190.0), "Right Button");
		if (imgui.doButton(GEN_ID, Vector2(50, 220.0), mLeftBlobbyJoystickRight))
		{
			oldString = mLeftBlobbyJoystickRight;
			mLeftBlobbyJoystickRight = "";
		}
		imgui.doText(GEN_ID, Vector2(34.0, 260.0), "Jump Button");
		if (imgui.doButton(GEN_ID, Vector2(50, 290.0), mLeftBlobbyJoystickJump))
		{
			oldString = mLeftBlobbyJoystickJump;
			mLeftBlobbyJoystickJump = "";
		}
	}

	//right player side:
	imgui.doText(GEN_ID, Vector2(434.0, 10.0), "right player");
	if (imgui.doButton(GEN_ID, Vector2(480.0, 60.0), mRightBlobbyDevice))
	{
		if (mRightBlobbyDevice == "mouse")
			mRightBlobbyDevice = "keyboard";
		else if (mRightBlobbyDevice == "keyboard")
			mRightBlobbyDevice = "joystick";
		else if (mRightBlobbyDevice == "joystick")
			if (mLeftBlobbyDevice != "mouse")
				mRightBlobbyDevice = "mouse";
			else
				mRightBlobbyDevice = "keyboard";
	}
	//if mouse device is selected:
	if (mRightBlobbyDevice == "mouse")
	{
		imgui.doText(GEN_ID, Vector2(434.0, 120.0), "Jump Button");
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

		if (imgui.doButton(GEN_ID, Vector2(434.0, 350.0), "Set All"))
			mSetKeyboard = 11;

		imgui.doText(GEN_ID, Vector2(434.0, 120.0), "Left Key");
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

		imgui.doText(GEN_ID, Vector2(434.0, 190.0), "Right Key");
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

		imgui.doText(GEN_ID, Vector2(434.0, 260.0), "Jump Key");
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
		imgui.doText(GEN_ID, Vector2(434.0, 120.0), "Left Button");
		if (imgui.doButton(GEN_ID, Vector2(450, 150.0), mRightBlobbyJoystickLeft))
		{
			oldString = mRightBlobbyJoystickLeft;
			mRightBlobbyJoystickLeft = "";
		}
		imgui.doText(GEN_ID, Vector2(434.0, 190.0), "Right Button");
		if (imgui.doButton(GEN_ID, Vector2(450, 220.0), mRightBlobbyJoystickRight))
		{
			oldString = mRightBlobbyJoystickRight;
			mRightBlobbyJoystickRight = "";
		}
		imgui.doText(GEN_ID, Vector2(434.0, 260.0), "Jump Button");
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
		imgui.doText(GEN_ID, Vector2(180.0, 250.0), "Press Mouse Button");
		imgui.doText(GEN_ID, Vector2(290.0, 300.0), "For Jump");
		mLeftBlobbyMouseJumpbutton = InputManager::getSingleton()->getLastMouseButton();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyMouseJumpbutton = mOldInteger;
	}
	if (mLeftBlobbyKeyboardLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Left");
		mLeftBlobbyKeyboardLeft = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyKeyboardLeft = oldString;
	}
	if (mLeftBlobbyKeyboardRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mLeftBlobbyKeyboardRight = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyKeyboardRight = oldString;
	}
	if (mLeftBlobbyKeyboardJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Jumping");
		mLeftBlobbyKeyboardJump = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyKeyboardJump = oldString;
	}
	if (mLeftBlobbyJoystickLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Left");
		mLeftBlobbyJoystickLeft = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyJoystickLeft = oldString;
	}
	if (mLeftBlobbyJoystickRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mLeftBlobbyJoystickRight = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyJoystickRight = oldString;
	}
	if (mLeftBlobbyJoystickJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Jumping");
		mLeftBlobbyJoystickJump = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyJoystickJump = oldString;
	}
	if (mRightBlobbyMouseJumpbutton == -1)
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(180.0, 250.0), "Press Mouse Button");
		imgui.doText(GEN_ID, Vector2(290.0, 300.0), "For Jump");
		mRightBlobbyMouseJumpbutton = InputManager::getSingleton()->getLastMouseButton();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyMouseJumpbutton = mOldInteger;
	}
	if (mRightBlobbyKeyboardLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Left");
		mRightBlobbyKeyboardLeft = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mRightBlobbyKeyboardLeft = oldString;
	}
	if (mRightBlobbyKeyboardRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mRightBlobbyKeyboardRight = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mRightBlobbyKeyboardRight = oldString;
	}
	if (mRightBlobbyKeyboardJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Jumping");
		mRightBlobbyKeyboardJump = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mRightBlobbyKeyboardJump = oldString;
	}
	if (mRightBlobbyJoystickLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Left");
		mRightBlobbyJoystickLeft = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyJoystickLeft = oldString;
	}
	if (mRightBlobbyJoystickRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mRightBlobbyJoystickRight = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyJoystickRight = oldString;
	}
	if (mRightBlobbyJoystickJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Jumping");
		mRightBlobbyJoystickJump = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyJoystickJump = oldString;
	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), "ok"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new OptionState();
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), "cancel"))
	{
		delete this;
		mCurrentState = new OptionState();
	}
}

MiscOptionsState::MiscOptionsState()
{
	IMGUI::getSingleton().resetSelection();
	mSaveConfig = false;
	mOptionConfig.loadFile("config.xml");
	std::string currentBackground = mOptionConfig.getString("background");
	mBackground = -1;
	char** filenames = PHYSFS_enumerateFiles("backgrounds");
	for (int i = 0; filenames[i] != 0; ++i)
	{
		std::string tmp(filenames[i]);
		if (tmp.find(".bmp") != std::string::npos)
		{
			mBackgrounds.push_back(tmp);
			int pos = mBackgrounds.size() - 1;
			if (tmp == currentBackground)
				mBackground = pos;
		}
	}
	PHYSFS_freeList(filenames);
	mShowFPS = mOptionConfig.getBool("showfps");
	mShowBlood = mOptionConfig.getBool("blood");
	mVolume = mOptionConfig.getFloat("global_volume");
	mMute = mOptionConfig.getBool("mute");
	mGameFPS = mOptionConfig.getInteger("gamefps");
	mNetworkSide = mOptionConfig.getInteger("network_side");
}

MiscOptionsState::~MiscOptionsState()
{
	if (mSaveConfig)
	{
		mOptionConfig.setBool("showfps", mShowFPS);
		mOptionConfig.setBool("blood", mShowBlood);
		mOptionConfig.setFloat("global_volume", mVolume);
		mOptionConfig.setBool("mute", mMute);
		mOptionConfig.setInteger("gamefps", mGameFPS);
		mOptionConfig.setInteger("network_side", mNetworkSide);
		if (mBackground > -1)
			mOptionConfig.setString("background", mBackgrounds[mBackground]);
		mOptionConfig.saveFile("config.xml");
	}
	SpeedController::getMainInstance()->setDrawFPS(mOptionConfig.getBool("showfps"));
	BloodManager::getSingleton().enable(mOptionConfig.getBool("blood"));
	SoundManager::getSingleton().setVolume(mOptionConfig.getFloat("global_volume"));
	SoundManager::getSingleton().setMute(mOptionConfig.getBool("mute"));
	SpeedController::getMainInstance()->setGameSpeed(mOptionConfig.getInteger("gamefps"));
	RenderManager::getSingleton().setBackground(std::string("backgrounds/") + mOptionConfig.getString("background"));
}

void MiscOptionsState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doText(GEN_ID, Vector2(34.0, 10.0), "Background:");
	int tmp = mBackground;
	imgui.doSelectbox(GEN_ID, Vector2(34.0, 40.0), Vector2(400.0, 280.0), mBackgrounds, tmp);
	if (tmp != mBackground)
	{
		mBackground = tmp;
		if (mBackground > -1)
			RenderManager::getSingleton().setBackground(std::string("backgrounds/") + mBackgrounds[mBackground]);
	}

	imgui.doText(GEN_ID, Vector2(484.0, 10.0), "Volume:");
	if (imgui.doScrollbar(GEN_ID, Vector2(484.0, 50.0), mVolume))
	{
		SoundManager::getSingleton().setVolume(mVolume);
		SoundManager::getSingleton().playSound("sounds/bums.wav", 1.0);
	}
	if (imgui.doButton(GEN_ID, Vector2(531.0, 80.0), "Mute"))
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
	if (imgui.doButton(GEN_ID, Vector2(484.0, 120.0), "show fps"))
	{
		mShowFPS = !mShowFPS;
		SpeedController::getMainInstance()->setDrawFPS(mShowFPS);
	}
	if (mShowFPS)
	{
		imgui.doImage(GEN_ID, Vector2(466.0, 132.0), "gfx/pfeil_rechts.bmp");
	}
	if (imgui.doButton(GEN_ID, Vector2(484.0, 160.0), "show blood"))
	{
		mShowBlood = !mShowBlood;
		BloodManager::getSingleton().enable(mShowBlood);
		BloodManager::getSingleton().spillBlood(Vector2(484.0, 160.0), 1.5, 2);
	}
	if (mShowBlood)
	{
		imgui.doImage(GEN_ID, Vector2(466.0 ,172.0), "gfx/pfeil_rechts.bmp");
	}

	imgui.doText(GEN_ID, Vector2(434.0, 200.0), "Network Side:");
	if (imgui.doButton(GEN_ID, Vector2(450.0, 240.0), "Left"))
		mNetworkSide = 0;
	if (imgui.doButton(GEN_ID, Vector2(630.0, 240.0), "Right"))
		mNetworkSide = 1;
	if (mNetworkSide == 0)
		imgui.doImage(GEN_ID, Vector2(432.0, 252.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(612.0, 252.0), "gfx/pfeil_rechts.bmp");

	imgui.doText(GEN_ID, Vector2(292.0, 290.0), "Gamespeed:");
	float gamefps = (mGameFPS-30)/90.0;
	if (gamefps < 0.0)
		gamefps = 0.0;
	imgui.doScrollbar(GEN_ID, Vector2(295.0, 330.0), gamefps);
		mGameFPS = (int)(gamefps*90.0+30);
	if (imgui.doButton(GEN_ID, Vector2(205.0, 380.0), "very slow"))
		mGameFPS = 30;
	if (imgui.doButton(GEN_ID, Vector2(481.0, 380.0), "slow"))
		mGameFPS = 60;
	if (imgui.doButton(GEN_ID, Vector2(319.0, 415.0), "default"))
		mGameFPS = 75;
	if (imgui.doButton(GEN_ID, Vector2(205.0, 450.0), "fast"))
		mGameFPS = 90;
	if (imgui.doButton(GEN_ID, Vector2(360.0, 450.0), "very fast"))
		mGameFPS = 120;
	SpeedController::getMainInstance()->setGameSpeed(mGameFPS);

	std::stringstream FPSInPercent;
	FPSInPercent << int((float)mGameFPS/75*100);
	imgui.doText(GEN_ID, Vector2(515.0, 330.0), FPSInPercent.str()+="%");

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), "ok"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new OptionState();
		return;
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), "cancel"))
	{
		delete this;
		mCurrentState = new OptionState();
	}
}
