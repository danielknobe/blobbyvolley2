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

#include <boost/scoped_ptr.hpp>

struct lua_State;
class DuelMatchState;

/*! \class IScriptableComponent
	\brief Base class for lua scripted objects.
	\details Use this class as base class for objects that support lua scripting. It defines some commonly used functions to make
			coding easier. Does not define any public methods.
*/
class IScriptableComponent
{
public:
	struct Access;
protected:
	IScriptableComponent();
	virtual ~IScriptableComponent();

	void openScript(std::string file);
	void setLuaGlobal(const char* name, double value);
	bool getLuaFunction(const char* name) const;

	// calls a lua function that is on the stack and performs error handling
	void callLuaFunction(int arg_count = 0);

	// load lua functions
	void setGameConstants();
	void setGameFunctions();

	void updateGameState(const DuelMatchState& state);

	lua_State* mState;

private:
	boost::scoped_ptr<DuelMatchState> mGameState;
};

