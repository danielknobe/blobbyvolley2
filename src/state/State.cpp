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

#include "LocalGameState.h"
#include "ReplaySelectionState.h"
#include "NetworkState.h"
#include "NetworkSearchState.h"
#include "OptionsState.h"

#include "BlobbyApp.h"
#include "SoundManager.h"
#include "IMGUI.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "InputManager.h"
#include "IScriptableComponent.h"

/* implementation */

State::State() = default;

void State::playSound(const std::string& sound, float volume) {
	assert(m_App);
	m_App->getSoundManager().playSound(sound, volume);
}

bool State::is_exiting() const {
	return getInputMgr().exit();
}

bool State::clicked() const {
	return getInputMgr().click();
}

void State::setApp(BlobbyApp* app) {
	assert(app);
	m_App = app;
}

void State::switchState(State* newState)
{
	assert(m_App);
	m_App->switchToState(std::unique_ptr<State>(newState));
}

BlobbyApp& State::getApp() const
{
	assert(m_App);
	return *m_App;
}

IMGUI& State::getIMGUI() const {
	return getApp().getIMGUI();
}

InputManager& State::getInputMgr() const {
	return getApp().getInputManager();
}

// --------------------------------------------------------------------------------------------------------------------------------
//  concrete implementation of MainMenuState and CreditsState
// -----------------------------------------------------------

MainMenuState::MainMenuState()
{
	// set main menu fps
	SpeedController::getMainInstance()->setGameSpeed(75);
}

MainMenuState::~MainMenuState() = default;

void MainMenuState::step_impl()
{
	IMGUI& imgui = getIMGUI();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doImage(GEN_ID, Vector2(400.0, 175.0), "gfx/titel.bmp");
	if (imgui.doButton(GEN_ID, Vector2(34, 300.0), TextManager::MNU_LABEL_ONLINE))
	{
		switchState( new OnlineSearchState() );
	}
	if (imgui.doButton(GEN_ID, Vector2(34, 340.0), TextManager::MNU_LABEL_LAN))
	{
		switchState( new LANSearchState() );
	}
	if (imgui.doButton(GEN_ID, Vector2(34.0, 380.0), TextManager::MNU_LABEL_START))
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

	if (imgui.doButton(GEN_ID, Vector2(34.0, 420.0), TextManager::MNU_LABEL_OPTIONS))
	{
		switchState(new OptionState());
	}

	if (imgui.doButton(GEN_ID, Vector2(34.0, 460.0), TextManager::MNU_LABEL_REPLAY))
	{
		switchState(new ReplaySelectionState());
	}

	if (imgui.doButton(GEN_ID, Vector2(34.0, 500.0), TextManager::MNU_LABEL_CREDITS))
	{
		switchState(new CreditsState());
	}

	if (imgui.doButton(GEN_ID, Vector2(34.0, 540.0), TextManager::MNU_LABEL_EXIT))
	{
		getInputMgr().setEndBlobby();
	}
}

const char* MainMenuState::getStateName() const
{
	return "MainMenuState";
}

CreditsState::CreditsState()
{
	mYPosition = 600;
}

void CreditsState::step_impl()
{
	IMGUI& imgui = getIMGUI();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	const float xPosition = 50;

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition), TextManager::CRD_PROGRAMMERS);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+30), "Daniel Knobe");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+60), "  (daniel-knobe(at)web.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+85), "Jonathan Sieber");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+115), "  (jonathan_sieber(at)yahoo.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+140), "Erik Schultheis");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+170), "  (erik-schultheis(at)freenet.de)", TF_SMALL_FONT);

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+210), TextManager::CRD_GRAPHICS);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+240), "Silvio Mummert");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+270), "  (mummertathome(at)t-online.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+295), "Richard Bertrand");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+325), "  (ricbertrand(at)hotmail.com)", TF_SMALL_FONT);

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+365), TextManager::CRD_THX);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+395), "Daniel Skoraszewsky");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+425), "  (skoraszewsky(at)t-online.de)", TF_SMALL_FONT);

	if (mYPosition > 20)
		mYPosition -= 2.5;

	if (imgui.doButton(GEN_ID, Vector2(74.0, 530.0), TextManager::LBL_OK))
	{
		switchState(new MainMenuState());
	}

	if (SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) >= SDL_VERSIONNUM(2, 0, 14))
	{
		if (imgui.doButton(GEN_ID, Vector2(294.0, 530.0), "www.blobbyvolley.de"))
		{
			SDL_OpenURL("https://www.blobbyvolley.de");
		}
	}
}

const char* CreditsState::getStateName() const
{
	return "CreditsState";
}

