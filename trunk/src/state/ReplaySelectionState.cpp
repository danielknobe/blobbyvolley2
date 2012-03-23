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
#include "ReplaySelectionState.h"

/* includes */
#include "ReplayState.h"
#include "IMGUI.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "FileSystem.h"

/* implementation */
ReplaySelectionState::ReplaySelectionState()
{
	IMGUI::getSingleton().resetSelection();
	mChecksumError = false;
	mVersionError = false;

	mSelectedReplay = 0;
	mReplayFiles = FileSystem::getSingleton().enumerateFiles("replays", ".bvr");
	if (mReplayFiles.size() == 0)
		mSelectedReplay = -1;
	std::sort(mReplayFiles.rbegin(), mReplayFiles.rend());
	
	SpeedController::getMainInstance()->setGameSpeed(75);
}

void ReplaySelectionState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	
	
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	if (imgui.doButton(GEN_ID, Vector2(224.0, 10.0), TextManager::RP_PLAY) &&
				mSelectedReplay != -1)
	{
		std::string loadrep = mReplayFiles[mSelectedReplay];
		
		/// \todo we have to do something against this construction! 
		///		this is dangerous. we delete this state before it has done
		///		all of its work.
		deleteCurrentState();
		ReplayState* rs = new ReplayState();
		rs->loadReplay(loadrep);
		setCurrentState(rs);
		imgui.resetSelection();
	}
	else if (imgui.doButton(GEN_ID, Vector2(424.0, 10.0), TextManager::LBL_CANCEL))
	{
		deleteCurrentState();
		setCurrentState(new MainMenuState());
	}
	else
		imgui.doSelectbox(GEN_ID, Vector2(34.0, 50.0), Vector2(634.0, 550.0), mReplayFiles, mSelectedReplay);
	if (imgui.doButton(GEN_ID, Vector2(644.0, 60.0), TextManager::RP_DELETE))
	{
		if (!mReplayFiles.empty())
		if (FileSystem::getSingleton().deleteFile("replays/" + mReplayFiles[mSelectedReplay] + ".bvr"))
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
		imgui.doText(GEN_ID, Vector2(250, 200), TextManager::RP_CHECKSUM);
		imgui.doText(GEN_ID, Vector2(250, 250), TextManager::RP_FILE_CORRUPT);

		if (imgui.doButton(GEN_ID, Vector2(400, 330), TextManager::LBL_OK))
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
		imgui.doText(GEN_ID, Vector2(250, 200), TextManager::RP_VERSION);
		imgui.doText(GEN_ID, Vector2(250, 250), TextManager::RP_FILE_OUTDATED);

		if (imgui.doButton(GEN_ID, Vector2(400, 330), TextManager::LBL_OK))
		{
			mVersionError = false;
		}
		else
		{
			imgui.doInactiveMode(true);
		}
	}
}

const char* ReplaySelectionState::getStateName() const
{
	return "ReplaySelectionState";
}

