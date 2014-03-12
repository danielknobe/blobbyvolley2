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

#include "Global.h"
#include <boost/scoped_ptr.hpp>

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
public:
	virtual ~State() {}

	// step function defines the steps actual work
	virtual void step_impl() = 0;
	virtual const char* getStateName() const = 0;

	// static functions
	/// performs a step in the current state
	static void step();

	/// gets the currently active state
	static boost::scoped_ptr<State>& getCurrentState();
	/// gets the name of the currently active state
	static const char* getCurrenStateName();

protected:
	State();
	static void switchState(State* newState);

private:
	static boost::scoped_ptr<State> mCurrentState;
	static boost::scoped_ptr<State> mStateToSwitchTo;

};

// state classes
/*! \class GameState
	\brief base class for any game related state (Local, Network, Replay)
*/

class GameState : public State
{
public:
	virtual ~GameState() {}

	// step function defines the steps actual work
	virtual void step_impl() = 0;

protected:

	/// static protected helper function that
	/// draws the game. It is in State because
	/// this functionality is shared by
	/// LocalGameState, NetworkGameState and ReplayState
	static void presentGame(const DuelMatch& match);

	/// this draws the ui in the game, i.e. clock, score and player names
	static void presentGameUI(const DuelMatch& match);

};

/*! \class MainMenuState
	\brief state for main menu
*/
class MainMenuState : public State
{
public:
	MainMenuState();
	virtual ~MainMenuState();
	virtual void step_impl();
	virtual const char* getStateName() const;
};

/*! \class CreditsState
	\brief State for credits screen
*/
class CreditsState : public State
{
public:
	CreditsState();
	virtual void step_impl();
	virtual const char* getStateName() const;
private:
	float mYPosition;
};


