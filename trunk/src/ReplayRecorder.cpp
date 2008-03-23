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

#include "ReplayRecorder.h"
#include "physfs.h"
#include "tinyxml/tinyxml.h"

#include <sstream>
#include <boost/crc.hpp>
#include <SDL/SDL.h>

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

ReplayRecorder::ReplayRecorder(GameMode mode)
{
	mRecordMode = mode;
	mReplayData = 0;
}

ReplayRecorder::~ReplayRecorder()
{
	if (mReplayData)
	{
		delete[] mReplayData;
	}
}

bool ReplayRecorder::endOfFile()
{
	if (mRecordMode == MODE_REPLAY_DUEL)
		return (mReplayOffset >= mBufferSize);
	else
		return false;
}

void ReplayRecorder::save(const std::string& filename)
{
	PHYSFS_file* fileHandle = PHYSFS_openWrite(filename.c_str());
	if (!fileHandle)
	{
		std::cerr << "Warning: Unable to write to ";
		std::cerr << PHYSFS_getWriteDir() << filename;
		std::cerr << std::endl;
		return;
	}

	PHYSFS_write(fileHandle, validHeader, 1, sizeof(validHeader));
	
	boost::crc_32_type crc;
	crc(mServingPlayer);
	crc = std::for_each(mPlayerNames[LEFT_PLAYER].begin(), mPlayerNames[LEFT_PLAYER].end(), crc);
	crc = std::for_each(mPlayerNames[RIGHT_PLAYER].begin(), mPlayerNames[RIGHT_PLAYER].end(), crc);
	crc = std::for_each(mSaveData.begin(), mSaveData.end(), crc);

	uint32_t checksum = crc.checksum();
	PHYSFS_write(fileHandle, &checksum, 1, sizeof(checksum));

	PHYSFS_write(fileHandle, &mServingPlayer, 1, sizeof(mServingPlayer));
	PHYSFS_write(fileHandle, mPlayerNames[LEFT_PLAYER].c_str(), 1, mPlayerNames[LEFT_PLAYER].size()+1);
	PHYSFS_write(fileHandle, mPlayerNames[RIGHT_PLAYER].c_str(), 1, mPlayerNames[RIGHT_PLAYER].size()+1);
	for (int i=0; i<mSaveData.size(); i++)
		PHYSFS_write(fileHandle, &mSaveData[i], 1, sizeof(char));

	PHYSFS_close(fileHandle);
}

std::string ReplayRecorder::readString()
{
	std::string str;
	char c;
	while ((c = mReplayData[mReplayOffset++]) != '\0')
	{
		str.append(1, c);
	}
	return str;
}

int ReplayRecorder::readInt()
{
	int ret = mReplayData[mReplayOffset];
	mReplayOffset += sizeof(int);
	return ret;
}

char ReplayRecorder::readChar()
{
	return mReplayData[mReplayOffset++];
}

void ReplayRecorder::load(const std::string& filename)
{
	PHYSFS_file* fileHandle = PHYSFS_openRead(filename.c_str());
	if (!fileHandle)
		throw FileLoadException(filename);
	int fileLength = PHYSFS_fileLength(fileHandle);
	if (fileLength < 8)
	{
		std::cout << "Error: Invalid replay file: " <<
			filename << std::endl;
		return;
	}
	char header[4];
	PHYSFS_read(fileHandle, header, 4, 1);
	if (memcmp(&header, &validHeader, 4) != 0)
	{
		std::cout << "Error: Invalid replay file: " <<
			filename << std::endl;
		return;
	}
	uint32_t checksum;
	PHYSFS_read(fileHandle, &checksum, 1, 4);
	mBufferSize = fileLength-8;

	if (mReplayData != 0) {
		delete[] mReplayData;
	}
	mReplayData = new char[mBufferSize];

	mBufferSize = PHYSFS_read(fileHandle, mReplayData, 1, mBufferSize);
	PHYSFS_close(fileHandle);

	mReplayOffset = 0;

	mServingPlayer = (PlayerSide)readInt();
	mPlayerNames[LEFT_PLAYER] = readString();
	mPlayerNames[RIGHT_PLAYER] = readString();

	boost::crc_32_type realcrc;
	realcrc(mServingPlayer);
	realcrc = std::for_each(mPlayerNames[LEFT_PLAYER].begin(), mPlayerNames[LEFT_PLAYER].end(), realcrc);
	realcrc = std::for_each(mPlayerNames[RIGHT_PLAYER].begin(), mPlayerNames[RIGHT_PLAYER].end(), realcrc);
	realcrc.process_bytes(mReplayData + mReplayOffset, mBufferSize - mReplayOffset);

	if (realcrc.checksum() != checksum)
	{
		throw ChecksumException(filename, checksum, realcrc.checksum());
	}
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

void ReplayRecorder::setPlayerNames(const std::string left, const std::string right)
{
	mPlayerNames[LEFT_PLAYER] = left;
	mPlayerNames[RIGHT_PLAYER] = right;
}

std::string ReplayRecorder::getPlayerName(const PlayerSide side)
{
	return mPlayerNames[side];
}

PacketType ReplayRecorder::getPacketType()
{
	assert(mReplayData != 0);
	if (!endOfFile()) {
		uint8_t packet = mReplayData[mReplayOffset];
		return static_cast<PacketType>(packet >> 6);
	} else {
		return ID_ERROR;
	}
}

PlayerInput* ReplayRecorder::getInput()
{
	PlayerInput* input = new PlayerInput[MAX_PLAYERS];
	const uint8_t packet = mReplayData[mReplayOffset++];
	input[LEFT_PLAYER].set((bool)(packet & 32), (bool)(packet & 16), (bool)(packet & 8));
	input[RIGHT_PLAYER].set((bool)(packet & 4), (bool)(packet & 2), (bool)(packet & 1));
	return input;
}

PlayerSide ReplayRecorder::getServingPlayer()
{
	return mServingPlayer;
}

void ReplayRecorder::setServingPlayer(PlayerSide side)
{
	mServingPlayer = side;
}


