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
#include <ctime>	// for time_t

#include "Global.h"
#include "ReplayDefs.h"

class FileRead;
class PlayerInput;

/// \class IReplayLoader
/// \brief Base class for replay loaders.
/// \details \todo add detailed description.
class IReplayLoader
{
	public:
		
		/// \todo maybe we should use a safeptr here.
		/// \brief create IReplayLoader by major version
		/// \details creates a replay loader for replays with a 
		///			certain major version. The minor version of the
		///			created loader is the highest one possible.
		///	\return the replay loader or 0 is no sufficient loader has been found.
		static IReplayLoader* createReplayLoader(int major);
		
		/// \brief Creates an IReplayLoader for a certain file.
		/// \details Determines the version of the file and creates a
		///			corresponding IReplayLoader.
		///  \exception \todo we have to add and document the exceptions
		static IReplayLoader* createReplayLoader(const std::string& file);
		
		/// \brief virtual destructor
		/// \details
		/// \exception none
		virtual ~IReplayLoader() {};
		
		// General Interface
		
		/// \brief returns the major version of replay files the loader can load.
		/// \details
		/// \exception none
		virtual int getVersionMajor() const = 0;
		/// \brief returns the maximum minor version this loader can load.
		/// \details As minor indicates downward compatibility, this indicates
		/// that all replays with lower minor version can be loaded, too.
		/// \exception none
		virtual int getVersionMinor() const = 0;
		
		// Attributes Interface
		
		/// gets the name of a player
		virtual std::string getPlayerName(PlayerSide player) const = 0;
		/// gets the player side which started with serving;
		/// \todo this is alway LEFT_PLAYER, so it seems a little useless!
		virtual PlayerSide getServingPlayer() const = 0;
		
		/// gets the speed this game was played
		virtual int getSpeed() const = 0;
		/// gets the duration of the game in seconds
		virtual int getDuration() const = 0;
		/// gets the length of the replay in physic steps
		virtual int getLength()  const = 0;
		/// gets the date this replay was recorded
		virtual std::time_t getDate() const = 0;
		
		// Replay data interface
		
		/// \brief gets the player input at the moment step
		///	\param step Timestep from when the player input should be received.
		///				Has to be in range 0 ... getLength();
		/// \param left[out] target where left player input is stored
		/// \param right[out] target where right player input is stored
		virtual void getInputAt(int step, PlayerInput& left, PlayerInput& right) = 0;
		
		
	protected:
		/// \brief protected constructor. 
		/// \details Create IReplayLoaders with createReplayLoader functions.
		/// \exception none
		IReplayLoader() {};
		
	private:
		/// \todo add documentation
		virtual void initLoading(FileRead& file_handle, int minor_version, uint32_t checksum) = 0;
};
