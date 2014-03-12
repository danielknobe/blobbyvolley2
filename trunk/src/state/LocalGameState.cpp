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
#include "LocalGameState.h"

/* includes */
#include <ctime>

#include <boost/make_shared.hpp>

#include "DuelMatch.h"
#include "InputManager.h"
#include "IMGUI.h"
#include "ReplayRecorder.h"
#include "SoundManager.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "Blood.h"
#include "IUserConfigReader.h"
#include "FileExceptions.h"
#include "FileSystem.h"
#include "FileWrite.h"
#include "GenericIO.h"
#include "InputSourceFactory.h"

/* implementation */
LocalGameState::~LocalGameState()
{
}

LocalGameState::LocalGameState()
	: mRecorder(new ReplayRecorder())
{
	mSaveReplay = false;
	mWinner = false;
	mErrorMessage = "";

	boost::shared_ptr<IUserConfigReader> config = IUserConfigReader::createUserConfigReader("config.xml");
	PlayerIdentity leftPlayer = config->loadPlayerIdentity(LEFT_PLAYER, false);
	PlayerIdentity rightPlayer = config->loadPlayerIdentity(RIGHT_PLAYER, false);

	boost::shared_ptr<InputSource> leftInput = InputSourceFactory::createInputSource( config, LEFT_PLAYER);
	boost::shared_ptr<InputSource> rightInput = InputSourceFactory::createInputSource( config, RIGHT_PLAYER);

//	mLeftPlayer.loadFromConfig("left");
//	mRightPlayer.loadFromConfig("right");

	// create default replay name
	mFilename = leftPlayer.getName();
	if(mFilename.size() > 7)
		mFilename.resize(7);
	mFilename += " vs ";
	std::string oppname = rightPlayer.getName();
	if(oppname.size() > 7)
		oppname.resize(7);
	mFilename += oppname;


	// set speed
	SpeedController::getMainInstance()->setGameSpeed( (float)config->getInteger("gamefps") );


	SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);

	mMatch.reset(new DuelMatch( false, config->getString("rules")));
	mMatch->setPlayers(leftPlayer, rightPlayer);
	mMatch->setInputSources(leftInput, rightInput);

	mRecorder->setPlayerNames(leftPlayer.getName(), rightPlayer.getName());
	mRecorder->setPlayerColors( leftPlayer.getStaticColor(), rightPlayer.getStaticColor() );
	mRecorder->setGameSpeed((float)config->getInteger("gamefps"));

	IMGUI::getSingleton().resetSelection();
}

void LocalGameState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();
	if(mErrorMessage != "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100, 200), Vector2(700, 360));
		size_t split = mErrorMessage.find(':');
		std::string mProblem = mErrorMessage.substr(0, split);
		std::string mInfo = mErrorMessage.substr(split+1);
		imgui.doText(GEN_ID, Vector2(120, 220), mProblem);
		imgui.doText(GEN_ID, Vector2(120, 260), mInfo);
		if(imgui.doButton(GEN_ID, Vector2(330, 320), TextManager::LBL_OK))
		{
			mErrorMessage = "";
		}
		imgui.doCursor();
	}
	else if (mSaveReplay)
	{
		imgui.doOverlay(GEN_ID, Vector2(150, 200), Vector2(650, 400));
		imgui.doText(GEN_ID, Vector2(190, 220), TextManager::RP_SAVE_NAME);
		static unsigned cpos;
		imgui.doEditbox(GEN_ID, Vector2(180, 270), 18, mFilename, cpos);
		if (imgui.doButton(GEN_ID, Vector2(220, 330), TextManager::LBL_OK))
		{
			try
				{
				std::string repFileName = std::string("replays/") + mFilename + std::string(".bvr");
				if (mFilename != "")
				{
					boost::shared_ptr<FileWrite> savetarget = boost::make_shared<FileWrite>(repFileName);
					/// \todo add a check whether we overwrite a file
					mRecorder->save(savetarget);
					savetarget->close();
					mSaveReplay = false;
				}

				imgui.resetSelection();
			}
			catch( FileLoadException& ex)
			{
				mErrorMessage = std::string("Unable to create file:" + ex.getFileName());
				imgui.resetSelection();
			}
			catch( FileAlreadyExistsException& ex)
			{
				mErrorMessage = std::string("File already exists!:"+ ex.getFileName());
				imgui.resetSelection();
			}
			 catch( std::exception& ex)
			{
				mErrorMessage = std::string("Could not save replay: ");
				imgui.resetSelection();
			}
		}
		if (imgui.doButton(GEN_ID, Vector2(440, 330), TextManager::LBL_CANCEL))
		{
			mSaveReplay = false;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else if (mMatch->isPaused())
	{
		imgui.doOverlay(GEN_ID, Vector2(180, 200), Vector2(670, 400));
		imgui.doText(GEN_ID, Vector2(281, 260), TextManager::LBL_CONF_QUIT);
		if (imgui.doButton(GEN_ID, Vector2(530, 300), TextManager::LBL_NO)){
			mMatch->unpause();
		}
		if (imgui.doButton(GEN_ID, Vector2(260, 300), TextManager::LBL_YES))
		{
			switchState(new MainMenuState);
			return;
		}
		if (imgui.doButton(GEN_ID, Vector2(293, 340), TextManager::RP_SAVE))
		{
			mSaveReplay = true;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else if (mWinner)
	{
		std::string tmp = mMatch->getPlayer(mMatch->winningPlayer()).getName();
		imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(700, 450));
		imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
		imgui.doText(GEN_ID, Vector2(274, 250), tmp);
		imgui.doText(GEN_ID, Vector2(274, 300), TextManager::GAME_WIN);
		if (imgui.doButton(GEN_ID, Vector2(290, 350), TextManager::LBL_OK))
		{
			switchState(new MainMenuState());
			return;
		}
		if (imgui.doButton(GEN_ID, Vector2(400, 350), TextManager::GAME_TRY_AGAIN))
		{
			switchState(new LocalGameState());
			return;
		}
		if (imgui.doButton(GEN_ID, Vector2(320, 390), TextManager::RP_SAVE))
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
			switchState(new MainMenuState);
			return;
		}
		else
		{
			RenderManager::getSingleton().redraw();
			mMatch->pause();
		}
	}
	else
	{
		mRecorder->record(mMatch->getState());
		mMatch->step();

		if (mMatch->winningPlayer() != NO_PLAYER)
		{
			mWinner = true;
			mRecorder->record(mMatch->getState());
			mRecorder->finalize( mMatch->getScore(LEFT_PLAYER), mMatch->getScore(RIGHT_PLAYER) );
		}

		presentGame(*mMatch);
	}

	presentGameUI(*mMatch);
}

const char* LocalGameState::getStateName() const
{
	return "LocalGameState";
}

