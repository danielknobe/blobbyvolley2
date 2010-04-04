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
#include "state/LocalGameState.h"
#include "LocalInputSource.h"
#include "DuelMatch.h"
#include "ReplayRecorder.h"
#include "ScriptedInputSource.h"
#include "SoundManager.h"
#include "IMGUI.h"
#include "NetworkState.h"
#include "OptionsState.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "Blood.h"

#include <physfs.h>
#include <algorithm>
#include <sstream>

State* State::mCurrentState = 0;

State::State()
{

}

State* State::getCurrentState()
{
	if (mCurrentState == 0) {
		mCurrentState = new MainMenuState;
	}
	return mCurrentState;
}

void State::deleteCurrentState(){
	delete mCurrentState;
	mCurrentState = 0;
}
void State::setCurrentState(State* newState){
	assert(!mCurrentState);
	mCurrentState = newState;
}

MainMenuState::MainMenuState()
{
	IMGUI::getSingleton().resetSelection();
	
	// set main menu fps
	SpeedController::getMainInstance()->setGameSpeed(75);
}

MainMenuState::~MainMenuState()
{
}

void MainMenuState::step()
{
	RenderManager::getSingleton().drawGame(false);
	IMGUI& imgui = IMGUI::getSingleton();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doImage(GEN_ID, Vector2(250.0, 210.0), "gfx/titel.bmp");
	if (imgui.doButton(GEN_ID, Vector2(484, 370.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_NETWORK)))
	{
		deleteCurrentState();
		setCurrentState(new NetworkSearchState());
	}
	if (imgui.doButton(GEN_ID, Vector2(484.0, 400.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_START)))
	{
		try
		{
			deleteCurrentState();
			setCurrentState(new LocalGameState());
		}
		catch (ScriptException except)
		{
			FILE* file = fopen("lualog.txt", "wb");
			fprintf(file, "Lua Error: %s\n",
				except.luaerror.c_str());
			fclose(file);
		}
	}

	if (imgui.doButton(GEN_ID, Vector2(484.0, 430.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_OPTIONS)))
	{
		deleteCurrentState();
		setCurrentState(new OptionState());
	}

	if (imgui.doButton(GEN_ID, Vector2(484.0, 460.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_REPLAY)))
	{
		deleteCurrentState();
		setCurrentState(new ReplayMenuState());
	}

	if (imgui.doButton(GEN_ID, Vector2(484.0, 490.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_CREDITS)))
	{
		deleteCurrentState();
		setCurrentState(new CreditsState());
	}

	if (imgui.doButton(GEN_ID, Vector2(484.0, 520.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_EXIT)))
	{
		RenderManager::getSingleton().deinit();
		SoundManager::getSingleton().deinit();
		deleteCurrentState();
		SDL_Quit();
		exit(0);
	}
}

CreditsState::CreditsState()
{
	IMGUI::getSingleton().resetSelection();
	mYPosition = 600;
}

void CreditsState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition), TextManager::getSingleton()->getString(TextManager::CRD_PROGRAMMERS));
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+30), "Daniel Knobe");
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+60), "(daniel-knobe(at)web.de)");
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+95), "Jonathan Sieber");
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+125), "(jonathan_sieber(at)yahoo.de)");
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+160), "Sven Rech");
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+190), "(svenrech(at)gmx.de)");

	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+230), TextManager::getSingleton()->getString(TextManager::CRD_GRAPHICS));
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+260), "Silvio Mummert");
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+290), "(mummertathome(at)t-online.de)");
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+325), "Richard Bertrand");
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+355), "(ricbertrand(at)hotmail.com)");

	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+400), TextManager::getSingleton()->getString(TextManager::CRD_THX));
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+430), "Daniel Skoraszewsky");
	imgui.doText(GEN_ID, Vector2(40.0, mYPosition+460), "(skoraszewsky(at)t-online.de)");

	if (mYPosition > 20)
		mYPosition -= 2.5;

	if (imgui.doButton(GEN_ID, Vector2(400.0, 560.0), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
	{
		deleteCurrentState();
		setCurrentState(new MainMenuState());
		return;
	}
}

ReplayMenuState::ReplayMenuState() :
	mLeftPlayer(LEFT_PLAYER),
	mRightPlayer(RIGHT_PLAYER)
{
	IMGUI::getSingleton().resetSelection();
	mReplaying = false;
	mChecksumError = false;

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

void ReplayMenuState::loadCurrentReplay()
{
	mReplayRecorder = new ReplayRecorder(MODE_REPLAY_DUEL);

	try
	{
		mReplayRecorder->load(std::string("replays/" + mReplayFiles[mSelectedReplay] + ".bvr"));
		mReplaying = true;
		mReplayMatch = new DuelMatch(0, 0, true);
		mReplayMatch->setServingPlayer(mReplayRecorder->getServingPlayer());
		RenderManager::getSingleton().setPlayernames(
			mReplayRecorder->getPlayerName(LEFT_PLAYER), mReplayRecorder->getPlayerName(RIGHT_PLAYER));
		SoundManager::getSingleton().playSound(
				"sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
	}
	catch (ChecksumException& e)
	{
		delete mReplayRecorder;
		mReplayRecorder = 0;
		mChecksumError = true;
	}
}

void ReplayMenuState::step()
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
		
		rmanager->setBlobColor(LEFT_PLAYER, mLeftPlayer.getColor());
		rmanager->setBlobColor(RIGHT_PLAYER, mRightPlayer.getColor());
		rmanager->setScore(mReplayMatch->getScore(LEFT_PLAYER), mReplayMatch->getScore(RIGHT_PLAYER),
			mReplayMatch->getServingPlayer() == LEFT_PLAYER, mReplayMatch->getServingPlayer() == RIGHT_PLAYER);
			
		rmanager->setBlob(LEFT_PLAYER, mReplayMatch->getBlobPosition(LEFT_PLAYER),
			mReplayMatch->getWorld().getBlobState(LEFT_PLAYER));
		rmanager->setBlob(RIGHT_PLAYER, mReplayMatch->getBlobPosition(RIGHT_PLAYER),
			mReplayMatch->getWorld().getBlobState(RIGHT_PLAYER));
		
		rmanager->setBall(mReplayMatch->getBallPosition(),
				mReplayMatch->getWorld().getBallRotation());
				
		rmanager->setTime(mReplayMatch->getClock().getTimeString());
				
		int events = mReplayMatch->getEvents();
		SoundManager* smanager = &SoundManager::getSingleton();
		if(events & DuelMatch::EVENT_LEFT_BLOBBY_HIT)
		{
			smanager->playSound("sounds/bums.wav",
					mReplayMatch->getWorld().lastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
			Vector2 hitPos = mReplayMatch->getBallPosition() +
					(mReplayMatch->getBlobPosition(LEFT_PLAYER) - mReplayMatch->getBallPosition()).normalise().scale(31.5);
			BloodManager::getSingleton().spillBlood(hitPos, mReplayMatch->getWorld().lastHitIntensity(), 0);
		}
		
		if (events & DuelMatch::EVENT_RIGHT_BLOBBY_HIT)
		{
			smanager->playSound("sounds/bums.wav",
				mReplayMatch->getWorld().lastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
			Vector2 hitPos = mReplayMatch->getBallPosition() +
				(mReplayMatch->getBlobPosition(RIGHT_PLAYER) - mReplayMatch->getBallPosition()).normalise().scale(31.5);
			BloodManager::getSingleton().spillBlood(hitPos, mReplayMatch->getWorld().lastHitIntensity(), 1);
		}
		
		if (events & DuelMatch::EVENT_ERROR)
		{
			smanager->playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
		}

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
	}
}


