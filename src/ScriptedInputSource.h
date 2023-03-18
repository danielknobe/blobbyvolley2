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
#include <deque>

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
		ScriptedInputSource(const std::string& filename, PlayerSide side, unsigned int difficulty,
							const DuelMatch* match);
		~ScriptedInputSource() override;

		void setWaitTime(int wait_in_ms);

		PlayerInputAbs getNextInput() override;

	private:

		void setInputDelay(int delay);
		void setBallError(int duration, float amount);
		int getCurrentDifficulty() const;

		unsigned int mStartTime;
		unsigned int mWaitTime = WAITING_TIME;


		PlayerSide mSide;

		// Difficulty setting of the AI. Small values mean stronger AI
		int mDifficulty;

		// artificial reaction delay data
		int mReactionTime = 0;
		int mRoundStepCounter = 0;
		int mOldOppTouches = 0;
		int mOldOwnTouches = 0;
		float mOldBallVx = 0;

		// artificial ball position error
		Vector2 mBallPosError{0, 0};
		Vector2 mBallVelError{0, 0};
		int mBallPosErrorTimer = 0;

		int mBlobPosError = 0;

		std::default_random_engine mRandom;
		const DuelMatch* mMatch;
};
