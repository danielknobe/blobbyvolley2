/*=============================================================================
Blobby Volley 2
Copyright (C) 2008 Jonathan Sieber (jonathan_sieber@yahoo.de)

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

#include "LocalGameState.h"

#include <boost/lexical_cast.hpp>

#include "DuelMatch.h"
#include "InputManager.h"
#include "IMGUI.h"
#include "ReplayRecorder.h"
#include "SoundManager.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "Blood.h"
#include "IUserConfigReader.h"
#include <ctime>

LocalGameState::~LocalGameState()
{
	delete mMatch;
	delete mRecorder;
	InputManager::getSingleton()->endGame();
}

LocalGameState::LocalGameState()
	: State(),
	mLeftPlayer(LEFT_PLAYER),
	mRightPlayer(RIGHT_PLAYER)
{
	mSaveReplay = false;
	mWinner = false;
	
	mFilename = boost::lexical_cast<std::string> (std::time(0));
	
	mLeftPlayer.loadFromConfig("left");
	mRightPlayer.loadFromConfig("right");
	
	SpeedController::getMainInstance()->setGameSpeed(
			(float)IUserConfigReader::createUserConfigReader("config.xml")->getInteger("gamefps")
		);
	
	SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);

	mRecorder = new ReplayRecorder();
	mRecorder->setPlayerNames(mLeftPlayer.getName(), mRightPlayer.getName());
	mRecorder->setPlayerColors( mLeftPlayer.getColor(), mRightPlayer.getColor() );
	mRecorder->setGameSpeed((float)IUserConfigReader::createUserConfigReader("config.xml")->getInteger("gamefps"));

	mMatch = new DuelMatch(mLeftPlayer.getInputSource(), mRightPlayer.getInputSource(), true, false);

	RenderManager::getSingleton().setPlayernames(mLeftPlayer.getName(), mRightPlayer.getName());
	IMGUI::getSingleton().resetSelection();
}

void LocalGameState::step()
{
	RenderManager* rmanager = &RenderManager::getSingleton();

	IMGUI& imgui = IMGUI::getSingleton();
	if (mSaveReplay)
	{
		imgui.doOverlay(GEN_ID, Vector2(150, 200), Vector2(650, 400));
		imgui.doText(GEN_ID, Vector2(190, 220), TextManager::getSingleton()->getString(TextManager::RP_SAVE_NAME));
		static unsigned cpos;
		imgui.doEditbox(GEN_ID, Vector2(180, 270), 18, mFilename, cpos);
		if (imgui.doButton(GEN_ID, Vector2(220, 330), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
		{
			try
				{
				if (mFilename != "")
				{
					/// \todo add a check whether we overwrite a file
					mRecorder->save(std::string("replays/") + mFilename + std::string(".bvr"));
				}
				mSaveReplay = false;
				imgui.resetSelection();
			} 
			 catch( std::exception& ex) 
			{
				// only expected exception here is FileLoadException, which is thrown
				// when we try to create a file with invalid name.
				// don't reset selection when saving was not possible
				/// \todo add notification of user
				imgui.resetSelection();
			}
		}
		if (imgui.doButton(GEN_ID, Vector2(440, 330), TextManager::getSingleton()->getString(TextManager::LBL_CANCEL)))
		{
			mSaveReplay = false;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else if (mMatch->isPaused())
	{
		imgui.doOverlay(GEN_ID, Vector2(180, 200), Vector2(670, 400));
		imgui.doText(GEN_ID, Vector2(281, 260), TextManager::getSingleton()->getString(TextManager::LBL_CONF_QUIT));
		if (imgui.doButton(GEN_ID, Vector2(530, 300), TextManager::getSingleton()->getString(TextManager::LBL_NO))){
			mMatch->unpause();
		}
		if (imgui.doButton(GEN_ID, Vector2(260, 300), TextManager::getSingleton()->getString(TextManager::LBL_YES)))
		{
			deleteCurrentState();
			setCurrentState(new MainMenuState);
		}
		if (imgui.doButton(GEN_ID, Vector2(293, 340), TextManager::getSingleton()->getString(TextManager::RP_SAVE)))
		{
			mSaveReplay = true;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else if (mWinner)
	{
		std::string tmp;
		if(mMatch->winningPlayer() == LEFT_PLAYER)
			tmp = mLeftPlayer.getName();
		else
			tmp = mRightPlayer.getName();
		imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(700, 450));
		imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
		imgui.doText(GEN_ID, Vector2(274, 250), tmp);
		imgui.doText(GEN_ID, Vector2(274, 300), TextManager::getSingleton()->getString(TextManager::GAME_WIN));
		if (imgui.doButton(GEN_ID, Vector2(290, 350), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
		{
			deleteCurrentState();
			setCurrentState(new MainMenuState());
		}
		if (imgui.doButton(GEN_ID, Vector2(400, 350), TextManager::getSingleton()->getString(TextManager::GAME_TRY_AGAIN)))
		{
			deleteCurrentState();
			setCurrentState(new LocalGameState());
		}
		if (imgui.doButton(GEN_ID, Vector2(320, 390), TextManager::getSingleton()->getString(TextManager::RP_SAVE)))
		{
			mSaveReplay = true;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else if (InputManager::getSingleton()->exit())
	{
		if (mSaveReplay)
		{
			mSaveReplay = false;
			IMGUI::getSingleton().resetSelection();
		}
		else if (mMatch->isPaused())
		{
			deleteCurrentState();
			setCurrentState(new MainMenuState);
		}
		else
		{
			RenderManager::getSingleton().redraw();
			mMatch->pause();
		}
	}
	else
	{
		mRecorder->record(mMatch->getPlayersInput());
		mMatch->step();

		if (mMatch->winningPlayer() != NO_PLAYER)
			mWinner = true;
			
		presentGame(*mMatch);
		rmanager->setBlobColor(LEFT_PLAYER, mLeftPlayer.getColor());
		rmanager->setBlobColor(RIGHT_PLAYER, mRightPlayer.getColor());
	}
}

const char* LocalGameState::getStateName() const
{
	return "LocalGameState";
}

