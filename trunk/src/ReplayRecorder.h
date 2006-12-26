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

// This structure holds two player input chunks
// which can be converted from and to a XML tag

struct TwoPlayerInput
{
	TwoPlayerInput()
	{
	}
	
	TwoPlayerInput(PlayerInput linput, PlayerInput rinput);

	PlayerInput input[2];
	
	std::string getXMLString();
	void parseXMLValues(const std::string& lvalue,
					const std::string& rvalue);
};

// This class is the backend of ReplayInputSource. It combines two inputs
// in one stream and can save and load them through physfs
class ReplayInputSource;

class ReplayRecorder
{
	friend class ReplayInputSource;
	
public:
	// The mode parameter decides whether to replay or to record to
	// the given filename. If other values of the GameMode enumeration
	// or completely invalid values are given, the filename is replayed
	// to damage nothing. If an error occurs on loading, the endOfFile()
	// returns true and nothing is replayed. If an saving error occurs,
	// nothing is recorded
	ReplayRecorder(GameMode mode);
	
	// This rewinds to the start when replaying
	void reset();

	void save(const std::string& filename);
	void load(const std::string& filename);
	
	// This reports whether the record is played to the end, so the 
	// blobbys don't have to stand around bored after an incomplete
	// input record. This returns true if ReplayRecorder is not in
	// replay mode, so the method can be used directly as exit condition
	bool endOfFile();
	
	// This creates a ReplayInputSource as a hook to a real input source
	// If it is used only for replaying, the source is ignored and may
	// be 0
	InputSource* createReplayInputSource(PlayerSide side,
					InputSource* source);

private:
	typedef std::list<TwoPlayerInput> InputList;
	typedef std::list<TwoPlayerInput>::iterator InputListIterator;
	
	// These function tell ReplayInputSource how it should behave
	
	bool doPushInput();
	bool doGetInput();

	void pushInput(PlayerInput input, PlayerSide side);
	PlayerInput getInput(PlayerSide side);

	GameMode mRecordMode;

	PlayerInput mInputBuffer;
	PlayerSide mInputStoredInBuffer;
	
	InputList mRecordData;
	InputListIterator mInputCounter[MAX_PLAYERS];
	
	bool mReachedEOF;
};
