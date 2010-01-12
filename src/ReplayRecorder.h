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

#include <iostream>
#include <list>
#include <vector>

#include "Global.h"
#include "InputSource.h"


const char validHeader[4] = { 'B', 'V', '2', 'R' };	//header of replay file

enum RecorderMode
{
	MODE_RECORDING_DUEL,
	MODE_REPLAY_DUEL
};

//enumerated types of replay "packets"
//(should never be higher than 2 bits!)
enum PacketType
{
	ID_INPUT = 0,	//this packet contains inupt data of both players
	ID_COMMAND = 1,	//this packet contains command identificator
					//and next packet/packets are arguments of this
					//command
	ID_ERROR = 2,	//handles EOF
};

struct ChecksumException : public std::exception
{
	ChecksumException(std::string filename, uint32_t expected, uint32_t real);
	~ChecksumException() throw();

	virtual const char* what() const throw();

	std::string error;
};

class ReplayRecorder
{
public:
	ReplayRecorder(RecorderMode mode);
	~ReplayRecorder();

	// This rewinds to the start when replaying
	void reset();

	void save(const std::string& filename);
	void load(const std::string& filename);

	// This reports whether the record is played to the end, so the
	// blobbys don't have to stand around bored after an incomplete
	// input record. This returns true if ReplayRecorder is not in
	// replay mode, so the method can be used directly as exit condition
	bool endOfFile();

	void record(const PlayerInput* input);
	void setPlayerNames(const std::string& left, const std::string& right);

	std::string getPlayerName(const PlayerSide side);

	PlayerInput* getInput();

	PlayerSide getServingPlayer();
	void setServingPlayer(PlayerSide side);

	PacketType getPacketType();

private:
	std::string readString();
	int readInt();
	char readChar();

	PlayerInput getInput(PlayerSide side);

	RecorderMode mRecordMode;
	char* mReplayData;
	uint32_t mReplayOffset;
	int mBufferSize;
	std::vector<uint8_t> mSaveData;

	std::string mPlayerNames[MAX_PLAYERS];
	PlayerSide mServingPlayer;
	PlayerSide mOwnSide;
};
