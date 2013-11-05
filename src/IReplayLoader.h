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
#include <ctime>	// for time_t
#include <boost/shared_ptr.hpp>

#include "Global.h"
#include "ReplayDefs.h"
#include "BlobbyDebug.h"
#include "GenericIOFwd.h"

struct PlayerInput;

/// \class IReplayLoader
/// \brief Base class for replay loaders.
/// \details \todo add detailed description.
class IReplayLoader : public ObjectCounter<IReplayLoader>
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
		/// gets blob color of a player
		virtual Color getBlobColor(PlayerSide player) const = 0;
		/// get final score of a player
		virtual int getFinalScore(PlayerSide player) const = 0;
		
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
		virtual void getInputAt(int step, InputSource* left, InputSource* right) = 0;
		
		
		/// \brief checks wether the specified position is a savepoint
		/// \param position[in] position to check for savepoint
		/// \param save_position[out] if \t position is a savepoint, this int
		///					contains the index of the savepoint
		/// \return true, if \t position is a savepoint
		virtual bool isSavePoint(int position, int& save_position) const = 0 ;
		
		/// \brief gets the save point at position targetPosition
		/// \details returns the index of the last safepoint before targetPosition,
		///			so the game status at targetPosition can be calculated
		///			by simulating as few steps as possible.
		/// \param targetPosition[in] which position should be reached
		/// \param save_position[out] which position the safepoint has
		/// \return index of the savepoint, or -1 if none found.
		virtual int getSavePoint(int targetPosition, int& save_position) const = 0;
		
		/// \brief reads the specified savepoint
		/// \param index[in] index of the savepoint, as returned by getSavePoint
		/// \param state[out] the read savepoint is written there
		virtual void readSavePoint(int index, ReplaySavePoint& state) const = 0;
		
	protected:
		/// \brief protected constructor. 
		/// \details Create IReplayLoaders with createReplayLoader functions.
		/// \exception none
		IReplayLoader() {};
		
	private:
		/// \todo add documentation
		virtual void initLoading(boost::shared_ptr<GenericIn> file_handle, int minor_version) = 0;
};
