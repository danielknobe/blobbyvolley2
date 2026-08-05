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

#include <string>
#include <vector>
#include <memory>

#include "GameLogic.h"
#include "Vector.h"
#include "PlayerInput.h"
#include "PlayerIdentity.h"
#include "BlobbyDebug.h"
#include "MatchEvents.h"

class InputSource;
struct DuelMatchState;
class PhysicWorld;

/*! \class DuelMatch
	\brief class representing a blobby game.
	\details
	This class represents a single game between two players
	It applies the rules itself and provides an interface for querying
	different parameters. For this purpose it is designed as something
	similar to a singleton, but it can be instantiated
	multiple times on a server or be completely unavailable
*/
class DuelMatch : public ObjectCounter<DuelMatch>
{
	public:
		// If remote is true, only physical responses will be calculated
		// but hit events and score events are received from network

		DuelMatch(bool remote, const std::string& rules, int score_to_win = 0);

		void setPlayers(PlayerIdentity left_player, PlayerIdentity right_player);
		void setInputSources(std::shared_ptr<InputSource> left_input, std::shared_ptr<InputSource> right_input );

		~DuelMatch();

		void setRules(const std::string& rulesFile, int score_to_win = 0);

		void reset();

		// This steps through one frame
		void step();

		// these methods allow external input
		// events triggered by the network
		void setScore(int left, int right);
		void resetBall(PlayerSide side);

		// This reports the index of the winning player and -1 if the
		// game is still running
		PlayerSide winningPlayer() const;

		// These methods report the current game state and a useful for
		// the input manager, which needs information about the blob
		// positions and for lua export, which makes them accessible
		// for scripted input sources

		int getScore(PlayerSide player) const;
		int getScoreToWin() const;
		PlayerSide getServingPlayer() const;
		int getTouches(PlayerSide player) const;

		int getHitcount(PlayerSide player) const;

		Vector2 getBallPosition() const;
		Vector2 getBallVelocity() const;
		float getBallRotation() const;
		Vector2 getBlobPosition(PlayerSide player) const;
		Vector2 getBlobVelocity(PlayerSide player) const;
		float getBlobState(PlayerSide player) const;

		const PhysicWorld& getWorld() const{ return *mPhysicWorld; };

		// Timing
		const std::string& getTimeString() const;
		void setMatchTimeMs(int milliseconds);

		bool getBallDown() const;
		bool getBallActive() const;
		bool canStartRound(PlayerSide servingPlayer) const;

		void pause();
		void unpause();

		bool isPaused() const{ return mPaused; }

		// This functions returns true if the player launched
		// and is jumping at the moment
		bool getBlobJump(PlayerSide player) const;

		/// Set a new state using a saved DuelMatchState
		void setState(const DuelMatchState& state);
		/// trigger an external event
		void trigger( const MatchEvent& event );

		/// gets the current state
		DuelMatchState getState() const;

		//Input stuff for recording and playing replays
		std::shared_ptr<InputSource> getInputSource(PlayerSide player) const;

		PlayerIdentity getPlayer(PlayerSide player) const;
		PlayerIdentity& getPlayer(PlayerSide player);

		void setServingPlayer(PlayerSide side);

		const std::vector<MatchEvent>& getEvents() const { return mLastEvents; }
		std::vector<MatchEvent> fetchEvents() {
			return std::move(mLastEvents);
		}
		// this function will move all events into mLastEvents, so they will be returned by get events.
		// use this if no match step is performed, but external events have to be processed.
		void updateEvents();

	private:

		std::unique_ptr<PhysicWorld> mPhysicWorld;

		std::shared_ptr<InputSource> mInputSources[MAX_PLAYERS];
		PlayerInput mTransformedInput[MAX_PLAYERS];

		PlayerIdentity mPlayers[MAX_PLAYERS];

		GameLogicPtr mLogic;

		bool mPaused;

		// accumulation of physic events since last event processing
		std::vector<MatchEvent> mNewEvents;
		std::vector<MatchEvent> mLastEvents;	// events that were generated in the last processed frame

		bool mRemote;
};
