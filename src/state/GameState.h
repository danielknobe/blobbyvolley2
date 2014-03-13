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

#pragma once

#include "State.h"

/*! \class GameState
	\brief base class for any game related state (Local, Network, Replay)
*/

class GameState : public State
{
public:
	GameState(DuelMatch* match = nullptr);
	virtual ~GameState();

	// step function defines the steps actual work
	virtual void step_impl() = 0;

protected:

	/// static protected helper function that
	/// draws the game. It is in State because
	/// this functionality is shared by
	/// LocalGameState, NetworkGameState and ReplayState
	void presentGame();

	/// this draws the ui in the game, i.e. clock, score and player names
	void presentGameUI();

	// ui helpers
	/// this function draws the save replay ui
	/// it returns true if the player clicks on the OK button
	bool displaySaveReplayPrompt();

	/// this function draws the error message box
	/// it returns true if the player clicks on the OK button
	bool displayErrorMessageBox();

	/// this function draws the winning player screen
	/// does not display any buttons because they depend on the game type
	bool displayWinningPlayerScreen(PlayerSide winner);


	/// calculates the default name for a replay file
	void setDefaultReplayName(const std::string& left, const std::string& right);

	/// saves the replay to the desired file
	void saveReplay(ReplayRecorder& recorder);


	boost::scoped_ptr<DuelMatch> mMatch;

	// ui helper variable for storing a filename
	bool mSaveReplay;

	std::string mErrorMessage;
private:
	std::string mFilename;
};
