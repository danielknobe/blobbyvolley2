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
#include "ReplayState.h"

/* includes */
#include <sstream>

#include "IMGUI.h"
#include "replays/ReplayPlayer.h"
#include "DuelMatch.h"
#include "SoundManager.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "IUserConfigReader.h"
#include "ReplaySelectionState.h"
#include "InputManager.h"

/* implementation */

extern const std::string DUMMY_RULES_NAME;

ReplayState::ReplayState()
{
	IMGUI::getSingleton().resetSelection();

	mPositionJump = -1;
	mPaused = false;

	mSpeedValue = 8;
	mSpeedTimer = 0;
}

ReplayState::~ReplayState()
{

}

void ReplayState::loadReplay(const std::string& file)
{
	mReplayPlayer.reset( new ReplayPlayer() );

	//try
	//{
		mReplayPlayer->load(std::string("replays/" + file + ".bvr"));
		mMatch.reset(new DuelMatch(false, DUMMY_RULES_NAME));

		SoundManager::getSingleton().playSound(
				"sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);

		mMatch->setPlayers(mReplayPlayer->getPlayerName(LEFT_PLAYER), mReplayPlayer->getPlayerName(RIGHT_PLAYER));

		mMatch->getPlayer(LEFT_PLAYER).setStaticColor(mReplayPlayer->getBlobColor(LEFT_PLAYER));
		mMatch->getPlayer(RIGHT_PLAYER).setStaticColor(mReplayPlayer->getBlobColor(RIGHT_PLAYER));

		SpeedController::getMainInstance()->setGameSpeed(
				(float)IUserConfigReader::createUserConfigReader("config.xml")->getInteger("gamefps")
			);

	//}
	/*catch (ChecksumException& e)
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
	}*/
	/// \todo reintroduce error handling
}

void ReplayState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();

	// only draw cursor when mouse moved or clicked in the last second
	if(mLastMousePosition != InputManager::getSingleton()->position() || InputManager::getSingleton()->click())
	{
		/// \todo we must do this framerate independent
		mMouseShowTimer = 75;
	}

	if(mMouseShowTimer > 0)
	{
		imgui.doCursor();
		mMouseShowTimer--;
	}

	mLastMousePosition = InputManager::getSingleton()->position();


	if(mPositionJump != -1)
	{
		if(mReplayPlayer->gotoPlayingPosition(mPositionJump, mMatch.get()))
			mPositionJump = -1;
	}
	 else if(!mPaused)
	{
		while( mSpeedTimer >= 8)
		{
			mPaused = !mReplayPlayer->play(mMatch.get());
			mSpeedTimer -= 8;
			presentGame();
		}
		mSpeedTimer += mSpeedValue;

	}

	mMatch->getClock().setTime( mReplayPlayer->getReplayPosition() / mReplayPlayer->getGameSpeed() );

	// draw the progress bar
	Vector2 prog_pos = Vector2(50, 600-22);
	imgui.doOverlay(GEN_ID, prog_pos, Vector2(750, 600-3), Color(0,0,0));
	imgui.doOverlay(GEN_ID, prog_pos, Vector2(700*mReplayPlayer->getPlayProgress()+50, 600-3), Color(0,255,0));

	PlayerSide side = NO_PLAYER;
	if (mReplayPlayer->endOfFile())
	{
		int diff = mMatch->getScore(LEFT_PLAYER) - mMatch->getScore(RIGHT_PLAYER);
		if (diff > 0)
		{
			side = LEFT_PLAYER;
		}
		else if (diff < 0)
		{
			side = RIGHT_PLAYER;
		}
	}

	// play/pause button
	imgui.doOverlay(GEN_ID, Vector2(350, 535.0), Vector2(450, 575.0));
	bool pause_click = imgui.doImageButton(GEN_ID, Vector2(400, 555), Vector2(24, 24), mPaused ? "gfx/btn_play.bmp" : "gfx/btn_pause.bmp");
	bool fast_click = imgui.doImageButton(GEN_ID, Vector2(430, 555), Vector2(24, 24),  "gfx/btn_fast.bmp");
	bool slow_click = imgui.doImageButton(GEN_ID, Vector2(370, 555), Vector2(24, 24),  "gfx/btn_slow.bmp");

	// handle these image buttons. IMGUI is not capable of doing this.
	if(side == NO_PLAYER)
	{
		// control replay position
		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x + 5 > prog_pos.x && mousepos.y > prog_pos.y &&
			mousepos.x < prog_pos.x + 700 && mousepos.y < prog_pos.y + 24.0)
		{

			if (InputManager::getSingleton()->click())
			{
				float pos = (mousepos.x - prog_pos.x) / 700.0;
				mPositionJump = pos * mReplayPlayer->getReplayLength();
			}
		}

		if (pause_click)
		{
			if(mPaused)
			{
				if(mReplayPlayer->endOfFile())
					mPositionJump = 0;
			}
			mPaused = !mPaused;
		}

		if (fast_click)
		{
			mSpeedValue *= 2;
			if(mSpeedValue > 64)
				mSpeedValue = 64;
		}

		if (slow_click)
		{
			mSpeedValue /= 2;
			if(mSpeedValue < 1)
				mSpeedValue = 1;
		}

		if ((InputManager::getSingleton()->exit()))
		{
			switchState(new ReplaySelectionState());
			return;
		}
	}
	else
	{
		displayWinningPlayerScreen(side);

		if (imgui.doButton(GEN_ID, Vector2(290, 350), TextManager::LBL_OK))
		{
			switchState(new ReplaySelectionState());
			return;
		}
		if (imgui.doButton(GEN_ID, Vector2(400, 350), TextManager::RP_SHOW_AGAIN))
		{
			// we don't have to reset the match, cause we don't use lua rules
			// we just have to jump to the beginning
			SoundManager::getSingleton().playSound(
					"sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);

			// do we really need this?
			// maybe, users want to replay the game on the current speed
			SpeedController::getMainInstance()->setGameSpeed(
					(float)IUserConfigReader::createUserConfigReader("config.xml")->getInteger("gamefps")
				);

			mPaused = false;
			mPositionJump = 0;
		}
		imgui.doCursor();
	}

	// show the game ui
	presentGameUI();
}

const char* ReplayState::getStateName() const
{
	return "ReplayState";
}

