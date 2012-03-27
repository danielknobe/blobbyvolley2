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

#pragma once

#include "Global.h"
#include "Player.h"

class DuelMatch;
class InputSource;
class ReplayRecorder;

/*! \class State
	\brief Base class for all programme states.
	\details A programm state describes which state a programme is in
			( e.g. MainMenu, OptionsMenu, SingleplayerGame etc. ). It defines
			an abstract step function which is called each frame for the 
			currently active state.
			Switching to a new state is a little cumbersome right now, as 
			it requires deleting the current State (deleteCurrentState()) 
			and setting the new state afterwards. This approach is
			very error prone and generally not nice, so I hope we can
			replace it someday with something better ;)
*/
class State
{
private:
	static State* mCurrentState;
protected:
	State();
	void deleteCurrentState();
	void setCurrentState(State* newState);
	void switchState(State* newState);
	
	/// static protected helper function that 
	/// draws the game. It is in State because
	/// this functionality is shared by 
	/// LocalGameState, NetworkGameState and ReplayState
	static void presentGame(const DuelMatch& match);
public:
	virtual ~State() {}
	virtual void step() = 0;
	static State* getCurrentState();
	
	virtual const char* getStateName() const = 0;
	static const char* getCurrenStateName();
};

/*! \class MainMenuState
	\brief state for main menu
*/
class MainMenuState : public State
{
private:
public:
	MainMenuState();
	virtual ~MainMenuState();
	virtual void step();
	virtual const char* getStateName() const;
};

/*! \class CreditsState
	\brief State for credits screen
*/
class CreditsState : public State
{
public:
	CreditsState();
	virtual void step();
	virtual const char* getStateName() const;
private:
	float mYPosition;
};


