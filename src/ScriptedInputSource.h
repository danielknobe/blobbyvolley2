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

/**
 * @file ScriptedInputSource.h
 * @brief Contains a class which allows using lua scripted bots
 */

#pragma once

#include <string>
#include <random>

#include "Global.h"
#include "InputSource.h"
#include "Vector.h"
#include "IScriptableComponent.h"

/// \class ScriptedInputSource
/// \brief Bot controller
/// \details ScriptedInputSource provides an implementation of InputSource, which uses
/// Lua scripts to get its input. The given script is automatically initialised
/// and provided with an interface to the game.

/// The API documentation can now be found in doc/ScriptAPI.txt

// The time the bot waits after game start
const int WAITING_TIME = 1500;

struct lua_State;
class DuelMatch;

class ScriptedInputSource : public InputSource, public IScriptableComponent
{
	public:
		/// The constructor automatically loads and initializes the script
		/// with the given filename. The side parameter tells the script
		/// which side is it on.
		ScriptedInputSource(const std::string& filename, PlayerSide side, unsigned int difficulty);
		~ScriptedInputSource();

		virtual PlayerInputAbs getNextInput();

	private:

		unsigned int mStartTime;

		// ki strength values
		int mDifficulty;

		PlayerSide mSide;

		// error data
		bool mLastJump = false;
		double mJumpDelay = 0;
		std::normal_distribution<double> mDelayDistribution;
		std::default_random_engine mRandom;
};
