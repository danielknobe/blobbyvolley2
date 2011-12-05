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

#include "ReplayState.h"
#include "IMGUI.h"
#include "ReplayRecorder.h"
#include "DuelMatch.h"
#include "SoundManager.h"
#include "TextManager.h"
#include "SpeedController.h"
#include <sstream>

#include <physfs.h>

ReplayState::ReplayState() :
	mLeftPlayer(LEFT_PLAYER),
	mRightPlayer(RIGHT_PLAYER)
{
	IMGUI::getSingleton().resetSelection();
	mReplaying = false;
	mChecksumError = false;
	mVersionError = false;

	mReplayMatch = 0;
	mReplayRecorder = 0;
	mSelectedReplay = 0;
	char** filenames = PHYSFS_enumerateFiles("replays");
	for (int i = 0; filenames[i] != 0; ++i)
	{
		std::string tmp(filenames[i]);
		if (tmp.find(".bvr") != std::string::npos)
		{
			mReplayFiles.push_back(std::string(tmp.begin(), tmp.end()-4));
		}
	}
	if (mReplayFiles.size() == 0)
		mSelectedReplay = -1;
	std::sort(mReplayFiles.rbegin(), mReplayFiles.rend());

	mLeftPlayer.loadFromConfig("left");
	mRightPlayer.loadFromConfig("right");
}

void ReplayState::loadCurrentReplay()
{
	mReplayRecorder = new ReplayRecorder(MODE_REPLAY_DUEL);

	try
	{
		mReplayRecorder->load(std::string("replays/" + mReplayFiles[mSelectedReplay] + ".bvr"));
		mReplaying = true;
		mReplayMatch = new DuelMatch(0, 0, true, false);
		mReplayMatch->setServingPlayer(mReplayRecorder->getServingPlayer());
		RenderManager::getSingleton().setPlayernames(
			mReplayRecorder->getPlayerName(LEFT_PLAYER), mReplayRecorder->getPlayerName(RIGHT_PLAYER));
		SoundManager::getSingleton().playSound(
				"sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
				
		UserConfig gameConfig;
		gameConfig.loadFile("config.xml");
		SpeedController::getMainInstance()->setGameSpeed((float)gameConfig.getInteger("gamefps"));
	
	}
	catch (ChecksumException& e)
	{
		delete mReplayRecorder;
		mReplayRecorder = 0;
		mChecksumError = true;
	}
	catch (VersionMismatchException& e)
	{
		delete mReplayRecorder;
		mReplayRecorder = 0;
		mVersionError = true;
	}
}

void ReplayState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	if (mReplaying)
	{
		RenderManager* rmanager = &RenderManager::getSingleton();

		if(mReplayRecorder->getPacketType()==ID_INPUT)
		{
			mReplayMatch->setPlayersInput(mReplayRecorder->getInput());
			mReplayMatch->step();
		}
		
		presentGame(*mReplayMatch);
		rmanager->setBlobColor(LEFT_PLAYER, mLeftPlayer.getColor());
		rmanager->setBlobColor(RIGHT_PLAYER, mRightPlayer.getColor());

		PlayerSide side = mReplayMatch->winningPlayer();
		if (side != NO_PLAYER)
		{
			std::stringstream tmp;
			if(side == LEFT_PLAYER)
				tmp << mReplayRecorder->getPlayerName(LEFT_PLAYER);
			else
				tmp << mReplayRecorder->getPlayerName(RIGHT_PLAYER);
			imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(650, 450));
			imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
			imgui.doText(GEN_ID, Vector2(274, 250), tmp.str());
			imgui.doText(GEN_ID, Vector2(274, 300), TextManager::getSingleton()->getString(TextManager::GAME_WIN));
			if (imgui.doButton(GEN_ID, Vector2(290, 350), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
			{
				mReplaying = false;
				delete mReplayMatch;
				delete mReplayRecorder;
				imgui.resetSelection();
				SpeedController::getMainInstance()->setGameSpeed(75);
			}
			if (imgui.doButton(GEN_ID, Vector2(400, 350), TextManager::getSingleton()->getString(TextManager::RP_SHOW_AGAIN)))
			{
				delete mReplayMatch;
				delete mReplayRecorder;
				loadCurrentReplay();
				imgui.resetSelection();
			}
			imgui.doCursor();
		}
		else if ((InputManager::getSingleton()->exit()) || (mReplayRecorder->endOfFile()))
		{
			mReplaying = false;
			delete mReplayMatch;
			delete mReplayRecorder;
			imgui.resetSelection();
			SpeedController::getMainInstance()->setGameSpeed(75);
		}
	}
	else
	{
		imgui.doCursor();
		imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
		imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

		if (imgui.doButton(GEN_ID, Vector2(224.0, 10.0), TextManager::getSingleton()->getString(TextManager::RP_PLAY)) &&
					mSelectedReplay != -1)
		{
			loadCurrentReplay();
			imgui.resetSelection();
		}
		else if (imgui.doButton(GEN_ID, Vector2(424.0, 10.0), TextManager::getSingleton()->getString(TextManager::LBL_CANCEL)))
		{
			deleteCurrentState();
			setCurrentState(new MainMenuState());
		}
		else
			imgui.doSelectbox(GEN_ID, Vector2(34.0, 50.0), Vector2(634.0, 550.0), mReplayFiles, mSelectedReplay);
		if (imgui.doButton(GEN_ID, Vector2(644.0, 60.0), TextManager::getSingleton()->getString(TextManager::RP_DELETE)))
		{
			if (!mReplayFiles.empty())
			if (PHYSFS_delete(std::string("replays/" + mReplayFiles[mSelectedReplay] + ".bvr").c_str()))
			{
				mReplayFiles.erase(mReplayFiles.begin()+mSelectedReplay);
				if (mSelectedReplay >= mReplayFiles.size())
					mSelectedReplay = mReplayFiles.size()-1;
			}
		}

		if (mChecksumError)
		{
			imgui.doInactiveMode(false);
			imgui.doOverlay(GEN_ID, Vector2(210, 180), Vector2(650, 370));
			imgui.doText(GEN_ID, Vector2(250, 200), TextManager::getSingleton()->getString(TextManager::RP_CHECKSUM));
			imgui.doText(GEN_ID, Vector2(250, 250), TextManager::getSingleton()->getString(TextManager::RP_FILE_CORRUPT));

			if (imgui.doButton(GEN_ID, Vector2(400, 330), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
			{
				mChecksumError = false;
			}
			else
			{
				imgui.doInactiveMode(true);
			}
		}
		
		if (mVersionError)
		{
			imgui.doInactiveMode(false);
			imgui.doOverlay(GEN_ID, Vector2(210, 180), Vector2(650, 370));
			imgui.doText(GEN_ID, Vector2(250, 200), TextManager::getSingleton()->getString(TextManager::RP_VERSION));
			imgui.doText(GEN_ID, Vector2(250, 250), TextManager::getSingleton()->getString(TextManager::RP_FILE_OUTDATED));

			if (imgui.doButton(GEN_ID, Vector2(400, 330), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
			{
				mVersionError = false;
			}
			else
			{
				imgui.doInactiveMode(true);
			}
		}
	}
}

