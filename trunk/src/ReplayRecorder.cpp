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
#include "ReplayInputSource.h"
#include "physfs.h"
#include "tinyxml/tinyxml.h"

#include <SDL/SDL.h>


unsigned long incremental_crc32(unsigned long crc, std::vector<char> vec)
{
	crc ^= 0xFFFFFFFF;
	for (int i=0; i < vec.size(); i++)
		crc = crctable[(crc ^ vec[i]) & 0xFFL] ^ (crc >> 8);
	return crc ^ 0xFFFFFFFF;
}

unsigned long incremental_crc32(unsigned long crc, const char* buf, int size = 1)
{
	crc ^= 0xFFFFFFFF;
	for (int i=0; i < size; i++, buf++)
		crc = crctable[(crc ^ *buf) & 0xFFL] ^ (crc >> 8);
	return crc ^ 0xFFFFFFFF;
}

ReplayRecorder::ReplayRecorder(GameMode mode)
{
	mRecordMode = mode;
	mGameFPS = 60.0;
	mGameSpeed = 1.0;
}

bool ReplayRecorder::endOfFile()
{
	if (mRecordMode == MODE_REPLAY_DUEL)
		return (mBufferPtr >= mReplayData + mBufferSize);
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

	unsigned long checksum = 0;
	checksum = incremental_crc32(checksum, (char*)&mServingPlayer, sizeof(mServingPlayer));
	checksum = incremental_crc32(checksum, mPlayerNames[LEFT_PLAYER].c_str(), mPlayerNames[LEFT_PLAYER].size()+1);
	checksum = incremental_crc32(checksum, mPlayerNames[RIGHT_PLAYER].c_str(), mPlayerNames[RIGHT_PLAYER].size()+1);
	checksum = incremental_crc32(checksum, (char*)&mGameSpeed, sizeof(mGameSpeed));
	checksum = incremental_crc32(checksum, mSaveData);

	PHYSFS_write(fileHandle, &checksum, 1, sizeof(checksum));

	PHYSFS_write(fileHandle, &mServingPlayer, 1, sizeof(mServingPlayer));
	PHYSFS_write(fileHandle, mPlayerNames[LEFT_PLAYER].c_str(), 1, mPlayerNames[LEFT_PLAYER].size()+1);
	PHYSFS_write(fileHandle, mPlayerNames[RIGHT_PLAYER].c_str(), 1, mPlayerNames[RIGHT_PLAYER].size()+1);
	PHYSFS_write(fileHandle, &mGameSpeed, 1, sizeof(mGameSpeed));
	for (int i=0; i<mSaveData.size(); i++)
		PHYSFS_write(fileHandle, &mSaveData[i], 1, sizeof(char));

	PHYSFS_close(fileHandle);
}

std::string ReplayRecorder::readString()
{
	std::string str;
	while (*mBufferPtr != '\0')
	{
		str.append(1, *mBufferPtr++);
	}
	mBufferPtr++;
	return str;
}

int ReplayRecorder::readInt()
{
	int* temp = ((int*)mBufferPtr);
	mBufferPtr += sizeof(int);
	return *temp;
}

char ReplayRecorder::readChar()
{
	return *mBufferPtr++;
}

float ReplayRecorder::readFloat()
{
	float* temp = ((float*)mBufferPtr);
	mBufferPtr += sizeof(float);
	return *temp;
}

void ReplayRecorder::load(const std::string& filename)
{
	PHYSFS_file* fileHandle = PHYSFS_openRead(filename.c_str());
	if (!fileHandle)
		return;
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
	unsigned long checksum;
	unsigned long realChecksum = 0;
	PHYSFS_read(fileHandle, &checksum, 1, 4);
	mBufferSize = fileLength-8;
	mReplayData = new char[mBufferSize];
	if (mReplayData == NULL)
	{
		std::cout << "Error: Cannot load file to memory: " <<
			filename << std::endl;
		return;
	}
	PHYSFS_read(fileHandle, mReplayData, 1, mBufferSize);
	PHYSFS_close(fileHandle);
	realChecksum = incremental_crc32(realChecksum, mReplayData, mBufferSize);
	if (realChecksum != checksum)
	{
		std::cout << "Error: Corrupted replay file: " <<
			filename << "\nreal crc: " << realChecksum <<
			" crc in file: " << checksum << std::endl;
		return;
	}

	mBufferPtr = mReplayData;	//prepare access pointer to mReplayData buffer

	mServingPlayer = (PlayerSide)readInt();
	mPlayerNames[LEFT_PLAYER] = readString();
	mPlayerNames[RIGHT_PLAYER] = readString();
	mGameSpeed = readFloat();
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
	if (!endOfFile())
		return (PacketType)((unsigned char)*mBufferPtr >> 6);
	return ID_ERROR;
}

PlayerInput* ReplayRecorder::getInput()
{
	PlayerInput* input = new PlayerInput[MAX_PLAYERS];
	input[LEFT_PLAYER].set((bool)(*mBufferPtr & 32), (bool)(*mBufferPtr & 16), (bool)(*mBufferPtr & 8));
	input[RIGHT_PLAYER].set((bool)(*mBufferPtr & 4), (bool)(*mBufferPtr & 2), (bool)(*mBufferPtr & 1));
	mBufferPtr++;
	return input;
}
