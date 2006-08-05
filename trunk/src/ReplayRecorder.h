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
	ReplayRecorder(GameMode mode, const std::string& filename);
	
	// If the destructor is called, records are automatically saved
	~ReplayRecorder();
	
	// This rewinds to the start when replaying
	void reset();
	
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
	
	void save();
	void load();
	
	// These function tell ReplayInputSource how it should behave
	
	bool doPushInput();
	bool doGetInput();

	void pushInput(PlayerInput input, PlayerSide side);
	PlayerInput getInput(PlayerSide side);

	GameMode mRecordMode;
	std::string mRecordFilename;

	PlayerInput mInputBuffer;
	PlayerSide mInputStoredInBuffer;
	
	InputList mRecordData;
	InputListIterator mInputCounter[MAX_PLAYERS];
	
	bool mReachedEOF;
};
