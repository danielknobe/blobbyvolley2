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

/// \file ReplayRecorder.h
/// \brief contains replay recorder and associated enums

#pragma once

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "Global.h"
#include "ReplaySavePoint.h"
#include "PlayerInput.h"
#include "BlobbyDebug.h"
#include "GenericIOFwd.h"

namespace RakNet
{
	class BitStream;
}

class FileWrite;

/*! \class VersionMismatchException
	\brief thrown when replays of incompatible version are loaded.
*/
struct VersionMismatchException : public std::exception, public ObjectCounter<VersionMismatchException>
{
	VersionMismatchException(const std::string& filename, uint8_t major, uint8_t minor);
	~VersionMismatchException() throw();

	virtual const char* what() const throw();

	std::string error;
	uint8_t major;
	uint8_t minor;

};

/// \brief recording game
/// \todo we safe replays in continuous storeage (array or std::vector)
///			which might be ineffective for huge replays (1h ~ 270kb)
class ReplayRecorder : public ObjectCounter<ReplayRecorder>
{
	public:
		ReplayRecorder();
		~ReplayRecorder();

		void save(boost::shared_ptr<FileWrite> target) const;

		void send(boost::shared_ptr<GenericOut> stream) const;
		void receive(boost::shared_ptr<GenericIn>stream);

		// recording functions
		void record(const DuelMatchState& input);

		// saves the final score
		void finalize(unsigned int left, unsigned int right);

		// game setup setting
		void setPlayerNames(const std::string& left, const std::string& right);
		void setPlayerColors(Color left, Color right);
		void setGameSpeed(int fps);

	private:
		std::vector<uint8_t> mSaveData;
		std::vector<ReplaySavePoint> mSavePoints;

		// general replay attributes
		std::string mPlayerNames[MAX_PLAYERS];
		Color mPlayerColors[MAX_PLAYERS];
		unsigned int mEndScore[MAX_PLAYERS];
		unsigned int mGameSpeed;
};
