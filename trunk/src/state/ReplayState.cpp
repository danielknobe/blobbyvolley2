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
#include "ReplayPlayer.h"
#include "DuelMatch.h"
#include "SoundManager.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "IUserConfigReader.h"
#include <sstream>
#include "ReplaySelectionState.h"

ReplayState::ReplayState() :
	mLeftPlayer(LEFT_PLAYER),
	mRightPlayer(RIGHT_PLAYER)
{
	IMGUI::getSingleton().resetSelection();

	mLeftPlayer.loadFromConfig("left");
	mRightPlayer.loadFromConfig("right");
	
	mPositionJump = -1;
	mPaused = false;
	
	mSpeedValue = 8;
	mSpeedTimer = 0;
}

void ReplayState::loadReplay(const std::string& file)
{
	mReplayPlayer.reset( new ReplayPlayer() );

	//try
	//{
		mReplayPlayer->load(std::string("replays/" + file + ".bvr"));
		mReplayMatch.reset(new DuelMatch(0, 0, true, false));
		RenderManager::getSingleton().setPlayernames(
			mReplayPlayer->getPlayerName(LEFT_PLAYER), mReplayPlayer->getPlayerName(RIGHT_PLAYER));
		SoundManager::getSingleton().playSound(
				"sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
		
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

void ReplayState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	
	RenderManager* rmanager = &RenderManager::getSingleton();
	
	// only draw cursor when mouse moved in the last second
	if(mLastMousePosition != InputManager::getSingleton()->position())
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
		if(mReplayPlayer->gotoPlayingPosition(mPositionJump, mReplayMatch.get()))
			mPositionJump = -1;
	}
		else if(!mPaused)
	{
		while( mSpeedTimer >= 8)
		{
			mPaused = !mReplayPlayer->play(mReplayMatch.get());
			mSpeedTimer -= 8;
			presentGame(*mReplayMatch);
		} 
		mSpeedTimer += mSpeedValue;
		
	}

	rmanager->setBlobColor(LEFT_PLAYER, mLeftPlayer.getColor());
	rmanager->setBlobColor(RIGHT_PLAYER, mRightPlayer.getColor());
	
	// draw the progress bar
	Vector2 prog_pos = Vector2(50, 600-22);
	imgui.doOverlay(GEN_ID, prog_pos, Vector2(750, 600-3), Color(0,0,0));
	imgui.doOverlay(GEN_ID, prog_pos, Vector2(700*mReplayPlayer->getPlayProgress()+50, 600-3), Color(0,255,0));
	//imgui.doImage(GEN_ID, Vector2(50 + 700*mReplayPlayer->getPlayProgress(), 600-16), "gfx/scrollbar.bmp");
	
	// control replay position
	Vector2 mousepos = InputManager::getSingleton()->position();
	if (mousepos.x + 5 > prog_pos.x &&
		mousepos.y > prog_pos.y &&
		mousepos.x < prog_pos.x + 700 &&
		mousepos.y < prog_pos.y + 24.0)
	{

		if (InputManager::getSingleton()->click())
		{
			float pos = (mousepos.x - prog_pos.x) / 700.0;
			mPositionJump = pos * mReplayPlayer->getReplayLength();
		}
	}
	
	// play/pause button
	imgui.doOverlay(GEN_ID, Vector2(350, 535.0), Vector2(450, 575.0));
	if(mPaused)
	{
		imgui.doImage(GEN_ID, Vector2(400, 555.0), "gfx/btn_play.bmp");
	} else
	{
		imgui.doImage(GEN_ID, Vector2(400, 555.0), "gfx/btn_pause.bmp");
	}
	
	imgui.doImage(GEN_ID, Vector2(430, 555.0), "gfx/btn_fast.bmp");
	imgui.doImage(GEN_ID, Vector2(370, 555.0), "gfx/btn_slow.bmp");
	
	// handle these image buttons. IMGUI is not capable of doing this.
	if (InputManager::getSingleton()->click())
	{
		Vector2 mousepos = InputManager::getSingleton()->position();
		Vector2 btnpos = Vector2(400-12, 550.0-12);
		if (mousepos.x > btnpos.x &&
			mousepos.y > btnpos.y &&
			mousepos.x < btnpos.x + 24.0 &&
			mousepos.y < btnpos.y + 24.0)
		{

			if(mPaused) 
			{
				mPaused = false;
				if(mReplayPlayer->endOfFile())
					mPositionJump = 0;
			}
			else
				mPaused = true;
		}
		
		Vector2 fastpos = Vector2(430-12, 550.0-12);
		if (mousepos.x > fastpos.x &&
			mousepos.y > fastpos.y &&
			mousepos.x < fastpos.x + 24.0 &&
			mousepos.y < fastpos.y + 24.0)
		{	
			mSpeedValue *= 2;
			if(mSpeedValue > 64)
				mSpeedValue = 64;
		}
		
		Vector2 slowpos = Vector2(370-12, 550.0-12);
		if (mousepos.x > slowpos.x &&
			mousepos.y > slowpos.y &&
			mousepos.x < slowpos.x + 24.0 &&
			mousepos.y < slowpos.y + 24.0)
		{	
			mSpeedValue /= 2;
			if(mSpeedValue < 1)
				mSpeedValue = 1;
		}
	}
	
	PlayerSide side = mReplayMatch->winningPlayer();
	if (side != NO_PLAYER)
	{
		std::stringstream tmp;
		if(side == LEFT_PLAYER)
			tmp << mReplayPlayer->getPlayerName(LEFT_PLAYER);
		else
			tmp << mReplayPlayer->getPlayerName(RIGHT_PLAYER);
		imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(650, 450));
		imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
		imgui.doText(GEN_ID, Vector2(274, 250), tmp.str());
		imgui.doText(GEN_ID, Vector2(274, 300), TextManager::getSingleton()->getString(TextManager::GAME_WIN));
		if (imgui.doButton(GEN_ID, Vector2(290, 350), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
		{
			deleteCurrentState();
			setCurrentState(new ReplaySelectionState());
		}
		if (imgui.doButton(GEN_ID, Vector2(400, 350), TextManager::getSingleton()->getString(TextManager::RP_SHOW_AGAIN)))
		{
			/// \todo how do we handle reload?
		}
		imgui.doCursor();
	}
	else if ((InputManager::getSingleton()->exit()))
	{
		deleteCurrentState();
		setCurrentState(new ReplaySelectionState());
	}
}

const char* ReplayState::getStateName() const
{
	return "ReplayState";
}

