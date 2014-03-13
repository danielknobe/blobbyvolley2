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
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/* header include */
#include "State.h"

/* includes */
#include <algorithm>
#include <cstdio>

#include <boost/make_shared.hpp>

#include "LocalGameState.h"
#include "ReplaySelectionState.h"
#include "NetworkState.h"
#include "NetworkSearchState.h"
#include "OptionsState.h"

#include "ReplayRecorder.h"
#include "DuelMatch.h"
#include "SoundManager.h"
#include "IMGUI.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "Blood.h"
#include "MatchEvents.h"
#include "PhysicWorld.h"
#include "FileWrite.h"
#include "InputManager.h"


/* implementation */

// static state functions
boost::scoped_ptr<State> State::mCurrentState(nullptr);
boost::scoped_ptr<State> State::mStateToSwitchTo(nullptr);


void State::deinit()
{
	mCurrentState.reset( nullptr );
	mStateToSwitchTo.reset( nullptr );
}

void State::step()
{
	// check that we are in a valid state
	if(mCurrentState == nullptr )
	{
		// otherwise, create one
		mCurrentState.reset( new MainMenuState );
	}


	// perform a state step
	mCurrentState->step_impl();

	// check if we should switch to a new state
	if( mStateToSwitchTo != nullptr )
	{
		// maybe this debug statuses will help pinpointing crashes faster
		DEBUG_STATUS( std::string("switching to state ") + mStateToSwitchTo->getStateName());

		// if yes, set that new state
		// use swap so the states do not get destroyed here
		mCurrentState.swap( mStateToSwitchTo );

		// now destroy the old state, which is saved in mStateToSwitchTo at this point
		mStateToSwitchTo.reset(nullptr);
	}
}

const char* State::getCurrenStateName()
{
	return mCurrentState->getStateName();
}

State::State()
{
}

void State::switchState(State* newState)
{
	mStateToSwitchTo.reset(newState);
}

GameState::GameState(DuelMatch* match) : mMatch(match), mSaveReplay(false), mErrorMessage("")
{
	// enable game drawing
	RenderManager::getSingleton().drawGame(true);
}

GameState::~GameState()
{
	// disable game drawing
	RenderManager::getSingleton().drawGame(false);
}

void GameState::presentGame()
{
	RenderManager& rmanager = RenderManager::getSingleton();
	SoundManager& smanager = SoundManager::getSingleton();



	rmanager.setBlob(LEFT_PLAYER, mMatch->getBlobPosition(LEFT_PLAYER), mMatch->getWorld().getBlobState(LEFT_PLAYER));
	rmanager.setBlob(RIGHT_PLAYER, mMatch->getBlobPosition(RIGHT_PLAYER),	mMatch->getWorld().getBlobState(RIGHT_PLAYER));

	if(mMatch->getPlayer(LEFT_PLAYER).getOscillating())
	{
		rmanager.setBlobColor(LEFT_PLAYER, rmanager.getOscillationColor());
	}
	 else
	{
		rmanager.setBlobColor(LEFT_PLAYER, mMatch->getPlayer(LEFT_PLAYER).getStaticColor());
	}

	if(mMatch->getPlayer(RIGHT_PLAYER).getOscillating())
	{
		rmanager.setBlobColor(RIGHT_PLAYER, rmanager.getOscillationColor());
	}
	 else
	{
		rmanager.setBlobColor(RIGHT_PLAYER, mMatch->getPlayer(RIGHT_PLAYER).getStaticColor());
	}

	rmanager.setBall(mMatch->getBallPosition(), mMatch->getWorld().getBallRotation());

	int events = mMatch->getEvents();
	if(events & EVENT_LEFT_BLOBBY_HIT)
	{
		smanager.playSound("sounds/bums.wav", mMatch->getWorld().getLastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
		Vector2 hitPos = mMatch->getBallPosition() +
				(mMatch->getBlobPosition(LEFT_PLAYER) - mMatch->getBallPosition()).normalise().scale(31.5);
		BloodManager::getSingleton().spillBlood(hitPos, mMatch->getWorld().getLastHitIntensity(), 0);
	}

	if (events & EVENT_RIGHT_BLOBBY_HIT)
	{
		smanager.playSound("sounds/bums.wav", mMatch->getWorld().getLastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
		Vector2 hitPos = mMatch->getBallPosition() +
				(mMatch->getBlobPosition(RIGHT_PLAYER) - mMatch->getBallPosition()).normalise().scale(31.5);
		BloodManager::getSingleton().spillBlood(hitPos, mMatch->getWorld().getLastHitIntensity(), 1);
	}

	if (events & EVENT_ERROR)
		smanager.playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
}

void GameState::presentGameUI()
{
	auto& imgui = IMGUI::getSingleton();

	// Scores
	char textBuffer[64];
	snprintf(textBuffer, 8, mMatch->getServingPlayer() == LEFT_PLAYER ? "%02d!" : "%02d ", mMatch->getScore(LEFT_PLAYER));
	imgui.doText(GEN_ID, Vector2(24, 24), textBuffer);
	snprintf(textBuffer, 8, mMatch->getServingPlayer() == RIGHT_PLAYER ? "%02d!" : "%02d ", mMatch->getScore(RIGHT_PLAYER));
	imgui.doText(GEN_ID, Vector2(800-24, 24), textBuffer, TF_ALIGN_RIGHT);

	// blob name / time textfields
	imgui.doText(GEN_ID, Vector2(12, 550), mMatch->getPlayer(LEFT_PLAYER).getName());
	imgui.doText(GEN_ID, Vector2(788, 550), mMatch->getPlayer(RIGHT_PLAYER).getName(), TF_ALIGN_RIGHT);
	imgui.doText(GEN_ID, Vector2(400, 24), mMatch->getClock().getTimeString(), TF_ALIGN_CENTER);
}

bool GameState::displaySaveReplayPrompt()
{
	auto& imgui = IMGUI::getSingleton();

	imgui.doCursor();

	imgui.doOverlay(GEN_ID, Vector2(150, 200), Vector2(650, 400));
	imgui.doText(GEN_ID, Vector2(190, 220), TextManager::RP_SAVE_NAME);
	static unsigned cpos;
	imgui.doEditbox(GEN_ID, Vector2(180, 270), 18, mFilename, cpos);
	if(imgui.doButton(GEN_ID, Vector2(220, 330), TextManager::LBL_OK))
	{
		if(mFilename != "")
		{
			imgui.resetSelection();
			return true;
		}
	}

	if (imgui.doButton(GEN_ID, Vector2(440, 330), TextManager::LBL_CANCEL))
	{
		mSaveReplay = false;
		imgui.resetSelection();
		return false;
	}
	return false;
}

bool GameState::displayErrorMessageBox()
{
	auto& imgui = IMGUI::getSingleton();

	imgui.doCursor();

	imgui.doOverlay(GEN_ID, Vector2(100, 200), Vector2(700, 360));
	size_t split = mErrorMessage.find(':');
	std::string mProblem = mErrorMessage.substr(0, split);
	std::string mInfo = mErrorMessage.substr(split+1);
	imgui.doText(GEN_ID, Vector2(120, 220), mProblem);
	imgui.doText(GEN_ID, Vector2(120, 260), mInfo);
	if(imgui.doButton(GEN_ID, Vector2(330, 320), TextManager::LBL_OK))
	{
		mErrorMessage = "";
		return true;
	}
	return false;
}

bool GameState::displayWinningPlayerScreen(PlayerSide winner)
{
	auto& imgui = IMGUI::getSingleton();

	std::string tmp = mMatch->getPlayer(winner).getName();
	imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(700, 450));
	imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
	imgui.doText(GEN_ID, Vector2(274, 240), tmp);
	imgui.doText(GEN_ID, Vector2(274, 300), TextManager::GAME_WIN);
	imgui.doCursor();

	return false;
}

void GameState::setDefaultReplayName(const std::string& left, const std::string& right)
{
	mFilename = left;
	if(mFilename.size() > 7)
		mFilename.resize(7);
	mFilename += " vs ";

	std::string opp = right;
	if(right.size() > 7)
		opp.resize(7);
	mFilename += opp;
}

void GameState::saveReplay(ReplayRecorder& recorder)
{
	try
	{
		if (mFilename != "")
		{
			std::string repFileName = std::string("replays/") + mFilename + std::string(".bvr");

			boost::shared_ptr<FileWrite> savetarget = boost::make_shared<FileWrite>(repFileName);
			/// \todo add a check whether we overwrite a file
			recorder.save(savetarget);
			savetarget->close();
			mSaveReplay = false;
		}
	}
	catch( FileLoadException& ex)
	{
		mErrorMessage = std::string("Unable to create file:" + ex.getFileName());
		mSaveReplay = true;	// try again
	}
	catch( FileAlreadyExistsException& ex)
	{
		mErrorMessage = std::string("File already exists!:"+ ex.getFileName());
		mSaveReplay = true;	// try again
	}
	 catch( std::exception& ex)
	{
		mErrorMessage = std::string("Could not save replay: ");
		mSaveReplay = true;	// try again
	}
}


// --------------------------------------------------------------------------------------------------------------------------------
//  concrete implementation of MainMenuState and CreditsState
// -----------------------------------------------------------

MainMenuState::MainMenuState()
{
	IMGUI::getSingleton().resetSelection();

	// set main menu fps
	SpeedController::getMainInstance()->setGameSpeed(75);
}

MainMenuState::~MainMenuState()
{
}

void MainMenuState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doImage(GEN_ID, Vector2(250.0, 210.0), "gfx/titel.bmp");
	if (imgui.doButton(GEN_ID, Vector2(434, 300.0), TextManager::MNU_LABEL_ONLINE))
	{
		switchState( new OnlineSearchState() );
	}
	if (imgui.doButton(GEN_ID, Vector2(434, 340.0), TextManager::MNU_LABEL_LAN))
	{
		switchState( new LANSearchState() );
	}
	if (imgui.doButton(GEN_ID, Vector2(434.0, 380.0), TextManager::MNU_LABEL_START))
	{
		try
		{
			switchState(new LocalGameState());
		}
		catch (const ScriptException& except)
		{
			FILE* file = fopen("lualog.txt", "wb");
			fprintf(file, "Lua Error: %s\n",
				except.luaerror.c_str());
			fclose(file);
		}
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 420.0), TextManager::MNU_LABEL_OPTIONS))
	{
		switchState(new OptionState());
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 460.0), TextManager::MNU_LABEL_REPLAY))
	{
		switchState(new ReplaySelectionState());
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 500.0), TextManager::MNU_LABEL_CREDITS))
	{
		switchState(new CreditsState());
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 540.0), TextManager::MNU_LABEL_EXIT))
	{
		InputManager::getSingleton()->setEndBlobby();
	}
}

const char* MainMenuState::getStateName() const
{
	return "MainMenuState";
}

CreditsState::CreditsState()
{
	IMGUI::getSingleton().resetSelection();
	mYPosition = 600;
}

void CreditsState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	const float xPosition = 50;

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition), TextManager::CRD_PROGRAMMERS);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+30), "Daniel Knobe");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+60), "  (daniel-knobe(at)web.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+85), "Jonathan Sieber");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+115), "  (jonathan_sieber(at)yahoo.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+140), "Sven Rech");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+170), "  (svenrech(at)gmx.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+195), "Erik Schultheis");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+225), "  (erik-schultheis(at)freenet.de)", TF_SMALL_FONT);

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+255), TextManager::CRD_GRAPHICS);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+285), "Silvio Mummert");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+315), "  (mummertathome(at)t-online.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+340), "Richard Bertrand");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+370), "  (ricbertrand(at)hotmail.com)", TF_SMALL_FONT);

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+415), TextManager::CRD_THX);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+445), "Daniel Skoraszewsky");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+475), "  (skoraszewsky(at)t-online.de)", TF_SMALL_FONT);

	if (mYPosition > 20)
		mYPosition -= 2.5;

	if (imgui.doButton(GEN_ID, Vector2(400.0, 560.0), TextManager::LBL_CANCEL))
	{
		switchState(new MainMenuState());
		return;
	}
}

const char* CreditsState::getStateName() const
{
	return "CreditsState";
}

