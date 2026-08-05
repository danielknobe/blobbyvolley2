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
#include "DuelMatch.h"
#include "InputManager.h"
#include "InputDevice.h"
#include "IMGUI.h"
#include "replays/ReplayRecorder.h"
#include "SoundManager.h"
#include "SpeedController.h"
#include "IUserConfigReader.h"

#include "LocalInputSource.h"
#include "ScriptedInputSource.h"

#include <sstream>

/* implementation */
LocalGameState::~LocalGameState() = default;

LocalGameState::LocalGameState()
	: mWinner(false), mSetFinished(false), mSetWinner(NO_PLAYER),
	  mFinishedSetNumber(0), mFinishedLeftScore(0), mFinishedRightScore(0),
	  mRecorder(new ReplayRecorder())
{

}

std::shared_ptr<InputSource> LocalGameState::createInputSource(IUserConfigReader& config, PlayerSide player,
	                                                           PlayerSide side, const DuelMatch* match) {
	std::string prefix = player == LEFT_PLAYER ? "left" : "right";
	try
	{
		// these operations may throw, i.e., when the script is not found (should not happen)
		//  or has errors
		if (config.getBool(prefix + "_player_human"))
		{
			return std::make_shared<LocalInputSource>(getInputMgr().beginGame(side));
		}
		else
		{
			return std::make_shared<ScriptedInputSource>("scripts/" + config.getString(prefix + "_script_name"),
													 side, config.getInteger(prefix + "_script_strength"), match);
		}
	} catch (std::exception& e)
	{
		/// \todo REWORK ERROR REPORTING
		std::cerr << e.what() << std::endl;
		return std::make_shared<InputSource>();
	}
}

void LocalGameState::init()
{
	std::shared_ptr<IUserConfigReader> config = IUserConfigReader::createUserConfigReader("config.xml");
	PlayerIdentity leftPlayer = config->loadPlayerIdentity(LEFT_PLAYER, false);
	PlayerIdentity rightPlayer = config->loadPlayerIdentity(RIGHT_PLAYER, false);
	mBlobbyRules = config->getString("rules");
	mSeries = MatchSeries(matchFormatForRules(mBlobbyRules), config->getBool("sets_enabled", false));

	// create default replay name
	setDefaultReplayName(leftPlayer.getName(), rightPlayer.getName());

	// set speed
	SpeedController::getMainInstance()->setGameSpeed( (float)config->getInteger("gamefps") );

	playSound(SoundManager::WHISTLE, ROUND_START_SOUND_VOLUME);

	mMatch.reset(new DuelMatch(false, mBlobbyRules, mSeries.isSeries() ? mSeries.scoreToWin() : 0));
	std::shared_ptr<InputSource> leftInput = createInputSource(*config, LEFT_PLAYER, LEFT_PLAYER, mMatch.get());
	std::shared_ptr<InputSource> rightInput = createInputSource(*config, RIGHT_PLAYER, RIGHT_PLAYER, mMatch.get());
	mMatch->setPlayers(leftPlayer, rightPlayer);
	mMatch->setInputSources(leftInput, rightInput);
	if (mSeries.isSeries())
		mMatch->setServingPlayer(mSeries.firstServer());

	mRecorder->setPlayerNames(leftPlayer.getName(), rightPlayer.getName());
	mRecorder->setPlayerColors( leftPlayer.getStaticColor(), rightPlayer.getStaticColor() );
	mRecorder->setGameSpeed((float)config->getInteger("gamefps"));
	mRecorder->setGameRules(mBlobbyRules);
}



void LocalGameState::step_impl()
{
	IMGUI& imgui = getIMGUI();

	if(!mErrorMessage.empty())
	{
		displayErrorMessageBox();
	}
	else if (mSaveReplay && !mSeries.isSeries())
	{
		if ( displaySaveReplayPrompt() )
		{
			saveReplay( *mRecorder );
		}
	}
	else if (mMatch->isPaused())
	{
		if (mSeries.isSeries())
			displaySeriesQuitPrompt();
		else
			displayQueryPrompt(200,
				TextManager::LBL_CONF_QUIT,
				std::make_tuple(TextManager::LBL_YES, [&](){ switchState(new MainMenuState); }),
				std::make_tuple(TextManager::LBL_NO,  [&](){ mMatch->unpause(); }),
				std::make_tuple(TextManager::RP_SAVE, [&](){ mSaveReplay = true; imgui.resetSelection(); }));

		imgui.doCursor();
	}
	else if (mSetFinished)
	{
		displaySetWinnerScreen();
	}
	else if (mWinner)
	{
		const PlayerSide winnerSide = mSeries.isSeries()
			? mSeries.playerOnSide(mSeries.matchWinner()) : mMatch->winningPlayer();
		displayWinningPlayerScreen(winnerSide);
		if (imgui.doButton(GEN_ID, Vector2(310, 340), TextManager::LBL_OK))
		{
			switchState(new MainMenuState());
		}
		if (imgui.doButton(GEN_ID, Vector2(420, 340), TextManager::GAME_TRY_AGAIN))
		{
			switchState(new LocalGameState());
		}
		if (!mSeries.isSeries() && imgui.doButton(GEN_ID, Vector2(500, 390), TextManager::RP_SAVE, TF_ALIGN_CENTER))
		{
			mSaveReplay = true;
			imgui.resetSelection();
		}
	}
	else if (is_exiting())
	{
		if (mSaveReplay)
		{
			mSaveReplay = false;
			getIMGUI().resetSelection();
		}
		else if (mMatch->isPaused())
		{
			switchState(new MainMenuState);
		}
		else
		{
			mMatch->pause();
		}
	}
	else
	{
		if (!mSeries.isSeries())
			mRecorder->record(mMatch->getState());
		mMatch->step();

		if (mMatch->winningPlayer() != NO_PLAYER)
		{
			if (mSeries.isSeries())
			{
				mSetWinner = mMatch->winningPlayer();
				mFinishedSetNumber = mSeries.currentSet();
				mFinishedLeftScore = mMatch->getScore(LEFT_PLAYER);
				mFinishedRightScore = mMatch->getScore(RIGHT_PLAYER);
				mSeries.finishSet(mSeries.playerOnSide(mSetWinner), mFinishedLeftScore, mFinishedRightScore);
				mWinner = mSeries.matchWinner() != NO_PLAYER;
				mSetFinished = !mWinner;
			}
			else
			{
				mWinner = true;
				mRecorder->record(mMatch->getState());
				mRecorder->finalize(mMatch->getScore(LEFT_PLAYER), mMatch->getScore(RIGHT_PLAYER));
			}
		}

		presentGame();
	}

	presentGameUI();
	if (mSeries.isSeries() && !mSetFinished)
		presentSeriesUI();
}

void LocalGameState::startNextSet()
{
	std::shared_ptr<IUserConfigReader> config = IUserConfigReader::createUserConfigReader("config.xml");
	const PlayerSide leftPlayer = mSeries.playerOnSide(LEFT_PLAYER);
	const PlayerSide rightPlayer = mSeries.playerOnSide(RIGHT_PLAYER);

	mMatch.reset(new DuelMatch(false, mBlobbyRules, mSeries.scoreToWin()));
	mMatch->setPlayers(config->loadPlayerIdentity(leftPlayer, false),
	                   config->loadPlayerIdentity(rightPlayer, false));
	mMatch->setInputSources(createInputSource(*config, leftPlayer, LEFT_PLAYER, mMatch.get()),
	                        createInputSource(*config, rightPlayer, RIGHT_PLAYER, mMatch.get()));
	mMatch->setServingPlayer(mSeries.firstServer());
	mSetFinished = false;
	mSetWinner = NO_PLAYER;
	playSound(SoundManager::WHISTLE, ROUND_START_SOUND_VOLUME);
}

void LocalGameState::presentSeriesUI()
{
	getIMGUI().doText(GEN_ID, Vector2(212, 24),
	                  std::to_string(mSeries.setsWon(mSeries.playerOnSide(LEFT_PLAYER))), TF_ALIGN_CENTER);
	getIMGUI().doText(GEN_ID, Vector2(588, 24),
	                  std::to_string(mSeries.setsWon(mSeries.playerOnSide(RIGHT_PLAYER))), TF_ALIGN_CENTER);
}

void LocalGameState::displaySetWinnerScreen()
{
	IMGUI& imgui = getIMGUI();
	imgui.doOverlay(GEN_ID, Vector2(0, 150), Vector2(800, 450));
	imgui.doText(GEN_ID, Vector2(400, 205), mMatch->getPlayer(mSetWinner).getName(), TF_ALIGN_CENTER);
	std::ostringstream result;
	result << imgui.getText(TextManager::GAME_WINS_SET) << " " << mFinishedSetNumber << "   "
	       << mFinishedLeftScore << " - " << mFinishedRightScore;
	imgui.doText(GEN_ID, Vector2(400, 265), result.str(), TF_ALIGN_CENTER);
	if (imgui.doButton(GEN_ID, Vector2(400, 340), TextManager::GAME_NEXT_SET, TF_ALIGN_CENTER))
		startNextSet();
	imgui.doCursor();
}

void LocalGameState::displaySeriesQuitPrompt()
{
	IMGUI& imgui = getIMGUI();
	imgui.doOverlay(GEN_ID, Vector2(0, 200), Vector2(800, 400));
	imgui.doText(GEN_ID, Vector2(400, 230), TextManager::LBL_CONF_QUIT, TF_ALIGN_CENTER);
	if (imgui.doButton(GEN_ID, Vector2(330, 300), TextManager::LBL_YES, TF_ALIGN_CENTER))
		switchState(new MainMenuState);
	if (imgui.doButton(GEN_ID, Vector2(470, 300), TextManager::LBL_NO, TF_ALIGN_CENTER))
		mMatch->unpause();
}

const char* LocalGameState::getStateName() const
{
	return "LocalGameState";
}
