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
#include <algorithm>
#include <ctime>
#include <iostream> // for cerr

#include <boost/lexical_cast.hpp>

#include "ReplayState.h"
#include "IMGUI.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "FileSystem.h"
#include "IReplayLoader.h"


/* implementation */
ReplaySelectionState::ReplaySelectionState()
{
	mChecksumError = false;
	mVersionError = false;
	mShowReplayInfo = false;

	mSelectedReplay = 0;
	mReplayFiles = FileSystem::getSingleton().enumerateFiles("replays", ".bvr");
	if (mReplayFiles.size() == 0)
		mSelectedReplay = -1;
	std::sort(mReplayFiles.rbegin(), mReplayFiles.rend());

	SpeedController::getMainInstance()->setGameSpeed(75);
}

void ReplaySelectionState::step_impl()
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

		ReplayState* rs = 0;
		try
		{
			rs = new ReplayState();
			rs->loadReplay(loadrep);

			imgui.resetSelection();
			// at least make sure we end here!

			switchState(rs);
			return;
		}
		 catch (std::exception& exp)
		{
			delete rs;
			std::cerr << exp.what() << "\n";
		}
		return;
	}
	else if (imgui.doButton(GEN_ID, Vector2(424.0, 10.0), TextManager::LBL_CANCEL))
	{
		switchState(new MainMenuState());
		return;
	}
	else
		imgui.doSelectbox(GEN_ID, Vector2(34.0, 50.0), Vector2(634.0, 550.0), mReplayFiles, mSelectedReplay);

	if (imgui.doButton(GEN_ID, Vector2(644.0, 60.0), TextManager::RP_INFO))
	{
		if (!mReplayFiles.empty())
		{
			try
			{
				mReplayLoader.reset(IReplayLoader::createReplayLoader(std::string("replays/" + mReplayFiles[mSelectedReplay] + ".bvr")));
				mShowReplayInfo = true;
			}
			catch (std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
		}
	}
	if (imgui.doButton(GEN_ID, Vector2(644.0, 95.0), TextManager::RP_DELETE))
	{
		if (!mReplayFiles.empty())
		if (FileSystem::getSingleton().deleteFile("replays/" + mReplayFiles[mSelectedReplay] + ".bvr"))
		{
			mReplayFiles.erase(mReplayFiles.begin()+mSelectedReplay);
			if (mSelectedReplay >= mReplayFiles.size())
				mSelectedReplay = mReplayFiles.size()-1;
		}
	}

	if(mShowReplayInfo)
	{
		// setup
		std::string left =  mReplayLoader->getPlayerName(LEFT_PLAYER);
		std::string right =  mReplayLoader->getPlayerName(RIGHT_PLAYER);

		const int MARGIN = std::min(std::max(int(300 - 24*(std::max(left.size(),right.size()))), 50), 150);

		const int RIGHT = 800 - MARGIN;
		imgui.doInactiveMode(false);
		imgui.doOverlay(GEN_ID, Vector2(MARGIN, 180), Vector2(800-MARGIN, 445));
		std::string repname = mReplayFiles[mSelectedReplay];
		imgui.doText(GEN_ID, Vector2(400-repname.size()*12, 190), repname);

		// calculate text positions


		imgui.doText(GEN_ID, Vector2(MARGIN + 20, 225), left);
		imgui.doText(GEN_ID, Vector2(400-24, 225), "vs");
		imgui.doText(GEN_ID, Vector2(RIGHT - 20 - 24*right.size(), 225), right);

		time_t rd = mReplayLoader->getDate();
		struct tm* ptm;
		ptm = gmtime ( &rd );
		//std::
		char buffer[255];
		std::strftime(buffer, sizeof(buffer), "%d.%m.%Y - %H:%M", ptm);
		std::string date = buffer;
		imgui.doText(GEN_ID, Vector2(400 - 12*date.size(), 255), date);

		imgui.doText(GEN_ID, Vector2(MARGIN+20, 300), TextManager::OP_SPEED);
		std::string speed = boost::lexical_cast<std::string>(mReplayLoader->getSpeed() *100 / 75) + "%" ;
		imgui.doText(GEN_ID, Vector2(RIGHT - 20 - 24*speed.size(), 300), speed);

		imgui.doText(GEN_ID, Vector2(MARGIN+20, 335), TextManager::RP_DURATION);
		std::string dur;
		if(mReplayLoader->getDuration() > 99)
		{
			// +30 because of rounding
			dur = boost::lexical_cast<std::string>((mReplayLoader->getDuration() + 30) / 60) + "min";
		} else
		{
			dur = boost::lexical_cast<std::string>(mReplayLoader->getDuration()) + "s";
		}
		imgui.doText(GEN_ID, Vector2(RIGHT - 20 - 24*dur.size(), 335), dur);

		std::string res;
		res = boost::lexical_cast<std::string>(mReplayLoader->getFinalScore(LEFT_PLAYER)) + " : " +  boost::lexical_cast<std::string>(mReplayLoader->getFinalScore(RIGHT_PLAYER));

		imgui.doText(GEN_ID, Vector2(MARGIN+20, 370), TextManager::RP_RESULT);
		imgui.doText(GEN_ID, Vector2(RIGHT - 20 - 24*res.size(), 370), res);

		if (imgui.doButton(GEN_ID, Vector2(400-24, 410), TextManager::LBL_OK))
		{
			mShowReplayInfo = false;
		}
		else
		{
			imgui.doInactiveMode(true);
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

