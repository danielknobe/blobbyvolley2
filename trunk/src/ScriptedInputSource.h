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

#include <boost/circular_buffer.hpp>

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
		/// this variable saves the current match
		/// it is set each step in getInput
		/// as in current design, only in single match mode bots are allowed
		/// it would even be enough to set it once, but we may change this
		/// for making bot tournaments^^, so the idea of setting it for each
		/// bot seems better to me
		static const DuelMatch* mMatch;
		static ScriptedInputSource* mCurrentSource;

		// helpers
		static int debug(lua_State* state);
		static void setflags(lua_State* state);
		// coordinate system conversion
		template<class T>
		struct coordinate;

		// commands
		static int jump(lua_State* state);
		static int left(lua_State* state);
		static int right(lua_State* state);
		static int moveto(lua_State* state);

		// ball information
		// internals
		static const Vector2& getBallPosition();
		static const Vector2& getBallVelocity();
		static int touches(lua_State* state);

		// blob information
		static int launched(lua_State* state);

		// game status
		static int getScore(lua_State* state);
		static int getOppScore(lua_State* state);
		static int getScoreToWin(lua_State* state);
		static int getGameTime(lua_State* state);

		unsigned int mStartTime;

		// ki strength values
		unsigned int mMaxDelay;
		unsigned int mCurDelay;

		// which functions are available
		bool mOnBounce;

		// ball position and velocity in adapted coordinate system
		boost::circular_buffer<Vector2> mBallPositions;
		boost::circular_buffer<Vector2> mBallVelocities;

		float mLastBallSpeed;
		float mLastBallSpeedVirtual;

		PlayerSide mSide;

		// input data
		bool mLeft, mRight, mJump;
};
