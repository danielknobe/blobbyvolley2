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
#include "LastMatchesState.h"
#include "RankedState.h"

#include "IMGUI.h"
#include "blobnet/layer/Http.hpp"
#include "blobnet/layer/HttpRequestHeader.hpp"

#include "tinyxml2.h"

#include <future>
#include <sstream>
#include <string>
#include <iostream>

/* implementation */
LastMatchesState::LastMatchesState(const std::string& username, const std::string& password)
: mUsername(username)
, mPassword(password)
{
	std::async(std::launch::async, [this](){ updateLastMatchesResult(); });
}

LastMatchesState::~LastMatchesState()
{
}

void LastMatchesState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doImage(GEN_ID, Vector2(400, 305.0), "gfx/schild.bmp");
	for(std::size_t i = 0; i < mLastMatchesResults.size(); i++) {
		imgui.doText(GEN_ID, Vector2(160, 140.0 + 58.0 * i), std::get<0>(mLastMatchesResults[i]), TF_ALIGN_RIGHT);
		imgui.doText(GEN_ID, Vector2(170.0, 140.0 + 58.0 * i ), ":", TF_ALIGN_CENTER);
		imgui.doText(GEN_ID, Vector2(180, 140.0 + 58.0 * i), std::get<1>(mLastMatchesResults[i]), TF_ALIGN_LEFT);
		imgui.doText(GEN_ID, Vector2(260, 140.0 + 58.0 * i), std::get<2>(mLastMatchesResults[i]), TF_ALIGN_LEFT);
	}

	if (imgui.doButton(GEN_ID, Vector2(400.0, 480.0), TextManager::NET_REFRESH, TF_ALIGN_CENTER)) 
	{
		std::async(std::launch::async, [this](){ updateLastMatchesResult(); });
	}

	if (imgui.doButton(GEN_ID, Vector2(620.0, 560.0), TextManager::LBL_CANCEL, TF_ALIGN_CENTER)) 
	{
		switchState(new RankedState(mUsername, mPassword));
	}
}

const char* LastMatchesState::getStateName() const
{
	return "LastMatchesState";
}

void LastMatchesState::updateLastMatchesResult()
{
	try
	{
		// Get data from server
		BlobNet::Layer::Http http("blobbyvolley.de", 80);
		auto httpRequestHeader = BlobNet::Layer::HttpRequestHeader();
		httpRequestHeader.addAuthorization(mUsername, mPassword);
		std::stringstream result;
		http.request("api/getLastMatchesResults.php", result, httpRequestHeader);

		// parse data
		auto xml = std::make_shared<tinyxml2::XMLDocument>();
		xml->Parse(result.str().c_str());

		if (auto responseXml = xml->FirstChildElement("response")) 
		{
			mLastMatchesResults.clear();
			for (auto matchXml = responseXml->FirstChildElement("match"); matchXml != nullptr; matchXml = matchXml->NextSiblingElement("match"))
			{
				std::string const winPlayerName = matchXml->FirstChildElement("win_player_name")->GetText();
				std::string const losePlayerName = matchXml->FirstChildElement("lose_player_name")->GetText();
				std::string const winPlayerScore = matchXml->FirstChildElement("win_player_score")->GetText();
				std::string const losePlayerScore = matchXml->FirstChildElement("lose_player_score")->GetText();

				if (winPlayerName == mUsername)
				{
					mLastMatchesResults.push_back(std::make_tuple(winPlayerScore, losePlayerScore, losePlayerName));
				}
				else
				{
					mLastMatchesResults.push_back(std::make_tuple(losePlayerScore, winPlayerScore, winPlayerName));
				}
			}
		}
	} catch (std::exception& e) 
	{
		std::cout << "Can't get data from server: " << e.what() << std::endl;
	} catch (...) 
	{
		std::cout << "Can't get data from server: Unknown error." << std::endl;
	}
}