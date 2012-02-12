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

/* header include */
#include "ReplayRecorder.h"

/* includes */
#include "IReplayLoader.h"
#include "FileWrite.h"
#include "tinyxml/tinyxml.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <boost/crc.hpp>
#include <SDL/SDL.h>
#include "Global.h"
#include <ctime>

/* implementation */
ChecksumException::ChecksumException(std::string filename, uint32_t expected, uint32_t real)
{
	std::stringstream errorstr;

	errorstr << "Error: Corrupted replay file: " << filename <<
		std::endl << "real crc: " << real <<
		" crc in file: " << expected;
	error = errorstr.str();
}

ChecksumException::~ChecksumException() throw()
{
}

const char* ChecksumException::what() const throw()
{
	return error.c_str();
}

VersionMismatchException::VersionMismatchException(const std::string& filename, uint8_t major, uint8_t minor)
{
	std::stringstream errorstr;

	errorstr << "Error: Outdated replay file: " << filename <<
		std::endl << "expected version: " << (int)REPLAY_FILE_VERSION_MAJOR << "." 
				<< (int)REPLAY_FILE_VERSION_MINOR << 
		std::endl << "got: " << (int)major << "." << (int)minor << " instead!" << std::endl;
	error = errorstr.str();
}

VersionMismatchException::~VersionMismatchException() throw()
{
}

const char* VersionMismatchException::what() const throw()
{
	return error.c_str();
}



ReplayRecorder::ReplayRecorder()
{
	mGameSpeed = -1;
}

ReplayRecorder::~ReplayRecorder()
{
}

void ReplayRecorder::save(const std::string& filename) const
{
	/// this may throw FileLoadException
	FileWrite file(filename);
	
	boost::crc_32_type crc;
	crc = std::for_each(mPlayerNames[LEFT_PLAYER].begin(), mPlayerNames[LEFT_PLAYER].end(), crc);
	crc = std::for_each(mPlayerNames[RIGHT_PLAYER].begin(), mPlayerNames[RIGHT_PLAYER].end(), crc);
	crc = std::for_each(mSaveData.begin(), mSaveData.end(), crc);

	uint32_t checksum = crc.checksum();
	
	writeFileHeader(file, checksum);
	
	uint32_t replayHeaderStart = file.tell();
	file.seek(replayHeaderStart + 7*sizeof(uint32_t));
	writeAttributesSection(file);
	writeJumpTable(file);
	writeDataSection(file);
	
	// the last thing we write is the header again, so
	// we can fill in all data we gathered during the
	// rest of the writing process
	file.seek(replayHeaderStart);
	writeReplayHeader(file);
	
	file.close();
}

void ReplayRecorder::writeFileHeader(FileWrite& file, uint32_t checksum) const
{
	file.write(validHeader, sizeof(validHeader));
	
	// after the header, we write the replay version
	// first, write zero. leading zero indicates that the following value
	// really is a version number (and not a checksum of an older replay!)
	file.writeByte(0);
	file.writeByte(REPLAY_FILE_VERSION_MAJOR);
	file.writeByte(REPLAY_FILE_VERSION_MINOR);
	file.writeByte(0);
	
	file.writeUInt32(checksum);
}

void ReplayRecorder::writeReplayHeader(FileWrite& file) const
{
	/// for now, this are fixed numbers
	/// we have to make sure they are right! 
	uint32_t header_ptr = file.tell();
	uint32_t header_size =  7*sizeof(header_ptr);
	
	uint32_t attr_size = 128;	/// for now, we reserve 128 bytes!
	uint32_t jptb_size = 128;	/// for now, we reserve 128 bytes!
	uint32_t data_size = mSaveData.size();	/// assumes 1 byte per data record!
	
	
	file.writeUInt32(header_size);
	file.writeUInt32(attr_ptr);
	file.writeUInt32(attr_size);
	file.writeUInt32(jptb_ptr);
	file.writeUInt32(jptb_size);
	file.writeUInt32(data_ptr);
	file.writeUInt32(data_size);
	
	// check that we really needed header_size space
	assert( file.tell() - header_size );
}

void ReplayRecorder::writeAttributesSection(FileWrite& file) const
{
	attr_ptr = file.tell();
	
	// we have to check that we are at attr_ptr!
	char attr_header[4] = {'a', 't', 'r', '\n'};
	uint32_t gamespeed = mGameSpeed;
	uint32_t gamelength = mSaveData.size();	/// \attention 1 byte = 1 step is assumed here
	uint32_t gameduration = gamelength / gamespeed;
	uint32_t gamedat = std::time(0);
	uint32_t leftcol = mPlayerColors[LEFT_PLAYER].toInt();
	uint32_t rightcol = mPlayerColors[RIGHT_PLAYER].toInt();
	// check that we can really safe time in gamedat. ideally, we should use a static assertion here
	//static_assert (sizeof(uint32_t) >= sizeof(time_t), "time_t does not fit into 32bit" );
	
	file.write(attr_header, sizeof(attr_header));
	file.writeUInt32(gamespeed);
	file.writeUInt32(gameduration);
	file.writeUInt32(gamelength);
	file.writeUInt32(gamedat);
	
	// write blob colors
	file.writeUInt32(leftcol);
	file.writeUInt32(rightcol);
	
	// write names
	file.writeNullTerminated(mPlayerNames[LEFT_PLAYER]);
	file.writeNullTerminated(mPlayerNames[RIGHT_PLAYER]);
	
	// we need to check that we don't use more space than we got!
	
	// set up writing for next section. not good!
	file.seek(attr_ptr + 128);
}

void ReplayRecorder::writeJumpTable(FileWrite& file) const
{
	jptb_ptr = file.tell();
	
	// we have to check that we are at attr_ptr!
	char jtbl_header[4] = {'j', 'p', 't', '\n'};
	
	file.write(jtbl_header, sizeof(jtbl_header));	
	
	file.seek(jptb_ptr + 128);
}

void ReplayRecorder::writeDataSection(FileWrite& file) const
{
	data_ptr = file.tell();
	
	// we have to check that we are at attr_ptr!
	char data_header[4] = {'d', 'a', 't', '\n'};
	file.write(data_header, sizeof(data_header));
	
	uint32_t recordcount = mSaveData.size();
	
	file.writeUInt32(recordcount);
	
	/// \todo shouldn't we write more than 1 char per step?
	/// \todo why don't we zip it? even though it's quite compact, 
	/// 		we still save a lot of redundant information.
	for (int i=0; i<mSaveData.size(); i++)
		file.writeByte(mSaveData[i]);

}

void ReplayRecorder::record(const PlayerInput* input)
{
	unsigned char packet = 0;
	packet = ID_INPUT; packet <<= 1;
	packet += (input[LEFT_PLAYER].left & 1); packet <<= 1;
	packet += (input[LEFT_PLAYER].right & 1); packet <<= 1;
	packet += (input[LEFT_PLAYER].up & 1); packet <<= 1;
	packet += (input[RIGHT_PLAYER].left & 1); packet <<= 1;
	packet += (input[RIGHT_PLAYER].right & 1); packet <<= 1;
	packet += (input[RIGHT_PLAYER].up & 1);
	mSaveData.push_back(packet);
}

void ReplayRecorder::setPlayerNames(const std::string& left, const std::string& right)
{
	mPlayerNames[LEFT_PLAYER] = left;
	mPlayerNames[RIGHT_PLAYER] = right;
}

void ReplayRecorder::setPlayerColors(Color left, Color right)
{
	mPlayerColors[LEFT_PLAYER] = left;
	mPlayerColors[RIGHT_PLAYER] = right;
}

void ReplayRecorder::setGameSpeed(int fps)
{
	mGameSpeed = fps;
}
