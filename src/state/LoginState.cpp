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
#include "LoginState.h"

#include "IMGUI.h"
#include "blobnet/layer/Http.hpp"
#include "blobnet/layer/HttpRequestHeader.hpp"

#include <SDL_misc.h>

#include "RankedState.h"

#include "tinyxml2.h"

#include <future>
#include <sstream>

#include <iostream>

/* implementation */
LoginState::LoginState()
{
}

LoginState::~LoginState()
{
}

void LoginState::step_impl()
{
	IMGUI& imgui = getIMGUI();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doOverlay(GEN_ID, Vector2(0.0, 100.0), Vector2(800.0, 400.0), Color(0,0,0), 0.3);

	imgui.doText(GEN_ID, Vector2(400.0, 140.0), TextManager::NET_LOGIN, TF_ALIGN_CENTER);

	imgui.doText(GEN_ID, Vector2(290.0, 200.0), TextManager::NET_NAME, TF_ALIGN_RIGHT);
	imgui.doText(GEN_ID, Vector2(290.0, 260.0), TextManager::NET_PASSWORD, TF_ALIGN_RIGHT);

	imgui.doEditbox(GEN_ID, Vector2(300.0, 200.0), 15, mUsername, mUsernamePos);
	imgui.doEditbox(GEN_ID, Vector2(300.0, 260.0), 15, mPassword, mPasswordPos, TF_OBFUSCATE);

	if (imgui.doButton(GEN_ID, Vector2(224.0, 350.0), TextManager::LBL_OK))
	{
		std::async(std::launch::async, [this](){ login(mUsername, mPassword); });
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 350.0), TextManager::LBL_CANCEL))
	{
		switchState(new MainMenuState());
	}
	if (imgui.doButton(GEN_ID, Vector2(750.0, 470.0), TextManager::NET_RESET_PASSWORD, TF_ALIGN_RIGHT))
	{
		SDL_OpenURL("http://blobbyvolley.de/passwordforgotten.php");
	}
	if (imgui.doButton(GEN_ID, Vector2(750.0, 530.0), TextManager::NET_REGISTER, TF_ALIGN_RIGHT))
	{
		SDL_OpenURL("http://blobbyvolley.de/register.php");
	}

	if(mShowLoginFailed)
	{
		imgui.doInactiveMode(false);
		imgui.doOverlay(GEN_ID, Vector2(0, 220), Vector2(800, 400));
		imgui.doText(GEN_ID, Vector2(400, 270), TextManager::NET_LOGIN_FAILED, TF_ALIGN_CENTER);

		if (imgui.doButton(GEN_ID, Vector2(400, 340), TextManager::LBL_OK, TF_ALIGN_CENTER))
		{
			mShowLoginFailed = false;
		}
		else
		{
			imgui.doInactiveMode(true);
		}
	}
}

const char* LoginState::getStateName() const
{
	return "LoginState";
}

void LoginState::login(std::string username, std::string password)
{
	bool success = false;

	try
	{
		// Get data from server
		BlobNet::Layer::Http http("blobbyvolley.de", 80);
		auto httpRequestHeader = BlobNet::Layer::HttpRequestHeader();
		httpRequestHeader.addAuthorization(username, password);
		std::stringstream loginResult;
		http.request("api/login.php", loginResult, httpRequestHeader);

		// parse data
		auto xml = std::make_shared<tinyxml2::XMLDocument>();
		xml->Parse(loginResult.str().c_str());

		if (auto responseXml = xml->FirstChildElement("response")) 
		{
			if (auto successXml = responseXml->FirstChildElement("success")) 
			{
				success = std::string(successXml->GetText()) == "True";
			}
		}
	} catch (std::exception& e) 
	{
		std::cout << "Can't get data from server: " << e.what() << std::endl;
	} catch (...) 
	{
		std::cout << "Can't get data from server: Unknown error." << std::endl;
	}

	if (success) 
	{
		switchState(new RankedState());
	}
	else
	{
		mShowLoginFailed = true;
	}
}