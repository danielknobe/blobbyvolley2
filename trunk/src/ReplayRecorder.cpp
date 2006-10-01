#include "ReplayRecorder.h"
#include "ReplayInputSource.h"
#include "physfs.h"
#include "tinyxml/tinyxml.h"

#include <SDL/SDL.h>


std::string& operator<< (std::string& out, const std::string& string)
{
	out.append(string);
	return out;
}


TwoPlayerInput::TwoPlayerInput(PlayerInput linput, PlayerInput rinput)
{
	input[0] = linput;
	input[1] = rinput;
}
std::string TwoPlayerInput::getXMLString()
{
	std::string buffer = "";
	buffer << "<chunk ";
	buffer << "lplayer=\"";
	buffer << input[0];
	buffer << "\" ";
	buffer << "rplayer=\"";
	buffer << input[1];
	buffer << "\" />\n";
	return buffer;
}

void TwoPlayerInput::parseXMLValues(const std::string& lvalue,
				const std::string& rvalue)
{
	input[0] = PlayerInput(lvalue);
	input[1] = PlayerInput(rvalue);
}

ReplayRecorder::ReplayRecorder(GameMode mode)
{
	mRecordMode = mode;
	mReachedEOF = true;
}

InputSource* ReplayRecorder::createReplayInputSource(
			PlayerSide side, InputSource* source)
{
	ReplayInputSource *newSource = new ReplayInputSource;
	newSource->mRecorder = this;
	newSource->mSide = side;
	newSource->mRealSource = source;
	return newSource;
}

void ReplayRecorder::pushInput(PlayerInput input, PlayerSide side)
{
	if (mInputStoredInBuffer == NO_PLAYER)
	{
		mInputStoredInBuffer = side;
		mInputBuffer = input;
	}
	else
	{
		if (mInputStoredInBuffer == side)
		{
			std::cerr << "Warning: Recorder only filled from "
				<< "one source! Skipping input." << std::endl;
			return;
		}
		if (mInputStoredInBuffer == LEFT_PLAYER)
		{
			TwoPlayerInput chunk(mInputBuffer, input);
			mRecordData.push_back(chunk);
		}
		if (mInputStoredInBuffer == RIGHT_PLAYER)
		{
			TwoPlayerInput chunk(input, mInputBuffer);
			mRecordData.push_back(chunk);
		}
		mInputStoredInBuffer = NO_PLAYER;
	}
}


bool ReplayRecorder::doPushInput()
{
	if (mRecordMode == MODE_RECORDING_DUEL)
		return true;
	else
		return false;
}

bool ReplayRecorder::doGetInput()
{
	if (mRecordMode == MODE_REPLAY_DUEL)
		return true;
	else
		return false;
}

PlayerInput ReplayRecorder::getInput(PlayerSide side)
{
	if (mReachedEOF)
		return PlayerInput(false, false, false);
	PlayerInput input = mInputCounter[side]->input[side];
	++mInputCounter[side];
	if (mInputCounter[side] == mRecordData.end())
		mReachedEOF = true;
	return input;
}

bool ReplayRecorder::endOfFile()
{
	if (mRecordMode == MODE_REPLAY_DUEL)
		return mReachedEOF;
	else
		return false;
}

void ReplayRecorder::save(const std::string& filename)
{
	const char xmlHeader[] =
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n"
		"<replayrecord>\n";
	const char xmlFooter[] = "</replayrecord>\n\n";
	const char xmlIndent[] = "\t";
	
	PHYSFS_file* fileHandle = PHYSFS_openWrite(filename.c_str());
	if (!fileHandle)
	{
		std::cerr << "Warning: Unable to write to ";
		std::cerr << PHYSFS_getWriteDir() << filename;
		std::cerr << std::endl;
		return;
	}
	
	PHYSFS_write(fileHandle, xmlHeader, 1, sizeof(xmlHeader) - 1);
	for (InputListIterator iter = mRecordData.begin();
			iter != mRecordData.end(); ++iter)
	{
		std::string tag = iter->getXMLString();
		PHYSFS_write (fileHandle, xmlIndent, 1, sizeof(xmlIndent) - 1);
		PHYSFS_write (fileHandle, tag.c_str(), 1, tag.length());
	}
	PHYSFS_write(fileHandle, xmlFooter, 1, sizeof(xmlFooter) - 1);
	PHYSFS_close(fileHandle);
}


void ReplayRecorder::load(const std::string& filename)
{
	PHYSFS_file* fileHandle = PHYSFS_openRead(filename.c_str());
	if (!fileHandle)
		return;
	int fileLength = PHYSFS_fileLength(fileHandle);
	char* fileBuffer = new char[fileLength];
	PHYSFS_read(fileHandle, fileBuffer, 1, fileLength);
	TiXmlDocument recordDoc;
	recordDoc.Parse(fileBuffer);
	delete[] fileBuffer;
	PHYSFS_close(fileHandle);
	if (recordDoc.Error())
	{
		std::cout << "Warning: Parse error in replay " <<
			filename << " !" << std::endl;
	}
	
	TiXmlElement* recordConfigElem =
		recordDoc.FirstChildElement("replayrecord");
	if (!recordConfigElem)
		return;
		
	for (TiXmlElement* chunkElem =
		recordConfigElem->FirstChildElement("chunk");
		chunkElem != NULL;
		chunkElem = chunkElem->NextSiblingElement("chunk"))
	{
		TwoPlayerInput chunk;
		std::string lattr = chunkElem->Attribute("lplayer");
		std::string rattr = chunkElem->Attribute("rplayer");
		if (lattr == "" && rattr == "")
			continue;
		chunk.parseXMLValues(lattr, rattr);
		mRecordData.push_back(chunk);
	}
	if (!mRecordData.empty())
		mReachedEOF = false;

	mInputStoredInBuffer = NO_PLAYER;
	mInputCounter[LEFT_PLAYER] = mRecordData.begin();
	mInputCounter[RIGHT_PLAYER] = mRecordData.begin();
}
