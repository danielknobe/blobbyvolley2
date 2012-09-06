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

#include <string>

#include <boost/scoped_ptr.hpp>

#include "Global.h"
#include "ReplayDefs.h"
#include "InputSource.h"

class DuelMatch;
class IReplayLoader;

/// \class ReplayPlayer
/// \brief Manages playing of replays.
/// \details This class is responsible for actually replaying the replays.
///			It uses the data the IReplayLoader gets from files and uses them to create a full replay.
/// \todo maybe we should rename this class. It might be confused with Player (a blob) and NetworkPlayer 
///			(a network connection on server).
/// \todo For now, this is only a copy of the playing related functions of ReplayRecorder.
class ReplayPlayer
{
	public:
		ReplayPlayer();
		~ReplayPlayer();
		
		void load(const std::string& filename);
		
		// -----------------------------------------------------------------------------------------
		// 							Replay Attributes
		// -----------------------------------------------------------------------------------------
		
		std::string getPlayerName(const PlayerSide side) const;
		Color getBlobColor(const PlayerSide side) const;
		int getGameSpeed() const;
		
		// -----------------------------------------------------------------------------------------
		// 							Status information
		// -----------------------------------------------------------------------------------------
		
		/// \brief Replaying finished
		/// \details This reports whether the record is played to the end, so the
		/// blobbys don't have to stand around bored after an incomplete
		/// input record. 
		bool endOfFile() const;
		
		/// \brief Replay Progress in precent
		/// \details returns (an estimate for) the replay progress in percent. Depending on
		///			replay file version, this is either exact or a guess of the system (when we
		///				don't know how long the replay is).
		float getPlayProgress() const; 
		
		/// \brief current replay position
		/// \details returns the current position in replay in steps. 
		int getReplayPosition() const;
		
		/// \brief length of replay
		/// \details returns the replay length in steps.
		int getReplayLength() const;
		
		// -----------------------------------------------------------------------------------------
		// 							replaying interface
		// -----------------------------------------------------------------------------------------
		
		bool play(DuelMatch* virtual_match);
		
		/// \brief Jumps to a position in replay.
		/// \details Goes to a certain position in replay. Simulates at most 100 steps per call
		///			to prevent visual lags, so it is possible that this function has to be called
		///			several times to reach the target.
		/// \param rep_position target position in number of physic steps.
		/// \return True, if desired position could be reached. 
		bool gotoPlayingPosition(int rep_position, DuelMatch* virtual_match);
		
	private:
	
		int mPosition;
		int mLength;
		boost::scoped_ptr<IReplayLoader> loader;
		
		std::string mPlayerNames[MAX_PLAYERS];
};
