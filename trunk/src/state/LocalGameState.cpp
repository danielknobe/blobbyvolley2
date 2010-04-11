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

#include <sstream>

#include "DuelMatch.h"
#include "InputManager.h"
#include "IMGUI.h"
#include "ReplayRecorder.h"
#include "SoundManager.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "Blood.h"

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
	
	std::stringstream temp;
	temp << time(0);
	mFilename = temp.str();
	
	mLeftPlayer.loadFromConfig("left");
	mRightPlayer.loadFromConfig("right");
	
	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");
	SpeedController::getMainInstance()->setGameSpeed((float)gameConfig.getInteger("gamefps"));
	
	SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);

	mRecorder = new ReplayRecorder(MODE_RECORDING_DUEL);
	mRecorder->setPlayerNames(mLeftPlayer.getName(), mRightPlayer.getName());
	mRecorder->setServingPlayer(LEFT_PLAYER);

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
			if (mFilename != "")
			{
				mRecorder->save(std::string("replays/") + mFilename + std::string(".bvr"));
			}
			mSaveReplay = false;
			imgui.resetSelection();
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
		UserConfig gameConfig;
		gameConfig.loadFile("config.xml");
		std::stringstream tmp;
		if(mMatch->winningPlayer() == LEFT_PLAYER)
			tmp << mLeftPlayer.getName();
		else
			tmp << mRightPlayer.getName();
		imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(700, 450));
		imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
		imgui.doText(GEN_ID, Vector2(274, 250), tmp.str());
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
	else if (mRecorder->endOfFile())
	{
		deleteCurrentState();
		setCurrentState(new MainMenuState);
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


