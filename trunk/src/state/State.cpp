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

void State::presentGame(const DuelMatch& match)
{
	RenderManager& rmanager = RenderManager::getSingleton();
	SoundManager& smanager = SoundManager::getSingleton();
	
	rmanager.setScore(match.getScore(LEFT_PLAYER), match.getScore(RIGHT_PLAYER),
		match.getServingPlayer() == LEFT_PLAYER, match.getServingPlayer() == RIGHT_PLAYER);
		
	rmanager.setBlob(LEFT_PLAYER, match.getBlobPosition(LEFT_PLAYER),
							match.getWorld().getBlobState(LEFT_PLAYER));
	rmanager.setBlob(RIGHT_PLAYER, match.getBlobPosition(RIGHT_PLAYER),
							match.getWorld().getBlobState(RIGHT_PLAYER));
	
	rmanager.setBall(match.getBallPosition(), match.getWorld().getBallRotation());
			
	rmanager.setTime(match.getClock().getTimeString());
			
	int events = match.getEvents();
	if(events & DuelMatch::EVENT_LEFT_BLOBBY_HIT)
	{
		smanager.playSound("sounds/bums.wav", match.getWorld().lastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
		Vector2 hitPos = match.getBallPosition() +
				(match.getBlobPosition(LEFT_PLAYER) - match.getBallPosition()).normalise().scale(31.5);
		BloodManager::getSingleton().spillBlood(hitPos, match.getWorld().lastHitIntensity(), 0);
	}
	
	if (events & DuelMatch::EVENT_RIGHT_BLOBBY_HIT)
	{
		smanager.playSound("sounds/bums.wav", match.getWorld().lastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
		Vector2 hitPos = match.getBallPosition() +
				(match.getBlobPosition(RIGHT_PLAYER) - match.getBallPosition()).normalise().scale(31.5);
		BloodManager::getSingleton().spillBlood(hitPos, match.getWorld().lastHitIntensity(), 1);
	}
	
	if (events & DuelMatch::EVENT_ERROR)
		smanager.playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);

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
	if (imgui.doButton(GEN_ID, Vector2(434, 350.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_ONLINE)))
	{
		deleteCurrentState();
		setCurrentState(new OnlineSearchState());
	}
	if (imgui.doButton(GEN_ID, Vector2(434, 380.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_LAN)))
	{
		deleteCurrentState();
		setCurrentState(new LANSearchState());
	}
	if (imgui.doButton(GEN_ID, Vector2(434.0, 410.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_START)))
	{
		try
		{
			deleteCurrentState();
			setCurrentState(new LocalGameState());
		}
		catch (const ScriptException& except)
		{
			FILE* file = fopen("lualog.txt", "wb");
			fprintf(file, "Lua Error: %s\n",
				except.luaerror.c_str());
			fclose(file);
		}
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 440.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_OPTIONS)))
	{
		deleteCurrentState();
		setCurrentState(new OptionState());
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 470.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_REPLAY)))
	{
		deleteCurrentState();
		setCurrentState(new ReplayMenuState());
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 500.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_CREDITS)))
	{
		deleteCurrentState();
		setCurrentState(new CreditsState());
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 530.0), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_EXIT)))
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
	
	const float xPosition = 50;
	
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition), TextManager::getSingleton()->getString(TextManager::CRD_PROGRAMMERS));
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+30), "Daniel Knobe");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+60), "  (daniel-knobe(at)web.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+85), "Jonathan Sieber");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+115), "  (jonathan_sieber(at)yahoo.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+140), "Sven Rech");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+170), "  (svenrech(at)gmx.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+195), "Erik Schultheis");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+225), "  (erik-schutlheis(at)freenet.de)", TF_SMALL_FONT);

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+255), TextManager::getSingleton()->getString(TextManager::CRD_GRAPHICS));
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+285), "Silvio Mummert");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+315), "  (mummertathome(at)t-online.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+340), "Richard Bertrand");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+370), "  (ricbertrand(at)hotmail.com)", TF_SMALL_FONT);

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+415), TextManager::getSingleton()->getString(TextManager::CRD_THX));
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+445), "Daniel Skoraszewsky");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+475), "  (skoraszewsky(at)t-online.de)", TF_SMALL_FONT);

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
		mReplayMatch = new DuelMatch(0, 0, true, false);
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


