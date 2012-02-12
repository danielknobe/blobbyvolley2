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

/// \file ReplayRecorder.h
/// \brief contains replay recorder and associated enums

#pragma once

#include <string>
#include <vector>

#include "Global.h"
#include "ReplayDefs.h"
#include "InputSource.h"

class FileWrite;

/*! \class ChecksumException
	\brief thrown when actual and expected file checksum mismatch
*/
struct ChecksumException : public std::exception
{
	ChecksumException(std::string filename, uint32_t expected, uint32_t real);
	~ChecksumException() throw();

	virtual const char* what() const throw();

	std::string error;
};

/*! \class VersionMismatchException
	\brief thrown when replays of incompatible version are loaded.
*/
struct VersionMismatchException : public std::exception
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
class ReplayRecorder
{
public:
	ReplayRecorder();
	~ReplayRecorder();

	void save(const std::string& filename) const;
	void record(const PlayerInput* input);
	
	void setPlayerNames(const std::string& left, const std::string& right);
	void setPlayerColors(Color left, Color right);
	void setGameSpeed(int fps);
private:

	void writeFileHeader(FileWrite&, uint32_t checksum) const;
	void writeReplayHeader(FileWrite&) const;
	void writeAttributesSection(FileWrite&) const;
	void writeJumpTable(FileWrite&) const;
	void writeDataSection(FileWrite&) const;

	std::vector<uint8_t> mSaveData;

	// general replay attributes
	std::string mPlayerNames[MAX_PLAYERS];
	Color mPlayerColors[MAX_PLAYERS];
	int mGameSpeed;
	
	
	// here we save the information needed to create the header
	//  pointers  to replay sections
	mutable uint32_t attr_ptr;
	mutable uint32_t jptb_ptr;
	mutable uint32_t data_ptr;
};
