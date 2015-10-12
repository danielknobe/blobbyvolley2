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
#include <algorithm>
#include <iostream>
#include <sstream>
#include <ctime>

#include <boost/crc.hpp>

#include "tinyxml/tinyxml.h"

#include "raknet/BitStream.h"

#include <SDL2/SDL.h>

#include "Global.h"
#include "IReplayLoader.h"
#include "PhysicState.h"
#include "GenericIO.h"
#include "FileRead.h"
#include "FileWrite.h"
#include "base64.h"

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
template<class T>
void writeAttribute(FileWrite& file, const char* name, const T& value)
{
	std::stringstream stream;
	stream << "\t<var name=\"" << name << "\" value=\"" << value << "\" />\n";
	file.write( stream.str() );
}

void ReplayRecorder::save( boost::shared_ptr<FileWrite> file) const
{
	constexpr const char* xmlHeader = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n<replay>\n";
	constexpr const char* xmlFooter = "</replay>\n\n";

	file->write(xmlHeader);

	char writeBuffer[256];
	int charsWritten = snprintf(writeBuffer, 256,
			"\t<version major=\"%i\" minor=\"%i\"/>\n",
			REPLAY_FILE_VERSION_MAJOR, REPLAY_FILE_VERSION_MINOR);
	file->write(writeBuffer, charsWritten);

	writeAttribute(*file, "game_speed", mGameSpeed);
	writeAttribute(*file, "game_length", mSaveData.size());
	writeAttribute(*file, "game_duration", mSaveData.size() / mGameSpeed);
	writeAttribute(*file, "game_date", std::time(0));

	writeAttribute(*file, "score_left", mEndScore[LEFT_PLAYER]);
	writeAttribute(*file, "score_right", mEndScore[RIGHT_PLAYER]);

	writeAttribute(*file, "name_left", mPlayerNames[LEFT_PLAYER]);
	writeAttribute(*file, "name_right", mPlayerNames[RIGHT_PLAYER]);

	/// \todo would be nice if we could write the actual colors instead of integers
	writeAttribute(*file, "color_left", mPlayerColors[LEFT_PLAYER].toInt());
	writeAttribute(*file, "color_right", mPlayerColors[RIGHT_PLAYER].toInt());

	// bow comes the actual replay data
	file->write("\t<input>\n");
	std::string binary = encode(mSaveData, 80);
	file->write(binary);
	file->write("\n\t</input>\n");

	// finally, write the save points
	// first, convert them into a POD
	file->write("\t<states>\n");
	RakNet::BitStream stream;
	auto convert = createGenericWriter(&stream);
	convert->generic<std::vector<ReplaySavePoint> > (mSavePoints);

	binary = encode((char*)stream.GetData(), (char*)stream.GetData() + stream.GetNumberOfBytesUsed(), 80);
	file->write(binary);
	file->write("\n\t</states>\n");

	file->write(xmlFooter);
	file->close();
}
void ReplayRecorder::send(boost::shared_ptr<GenericOut> target) const
{
	target->string(mPlayerNames[LEFT_PLAYER]);
	target->string(mPlayerNames[RIGHT_PLAYER]);

	target->generic<Color> (mPlayerColors[LEFT_PLAYER]);
	target->generic<Color> (mPlayerColors[RIGHT_PLAYER]);

	target->uint32( mGameSpeed );
	target->uint32( mEndScore[LEFT_PLAYER] );
	target->uint32( mEndScore[RIGHT_PLAYER] );

	target->generic<std::vector<unsigned char> >(mSaveData);
	target->generic<std::vector<ReplaySavePoint> > (mSavePoints);
}

void ReplayRecorder::receive(boost::shared_ptr<GenericIn> source)
{
	source->string(mPlayerNames[LEFT_PLAYER]);
	source->string(mPlayerNames[RIGHT_PLAYER]);

	source->generic<Color> (mPlayerColors[LEFT_PLAYER]);
	source->generic<Color> (mPlayerColors[RIGHT_PLAYER]);

	source->uint32( mGameSpeed );
	source->uint32( mEndScore[LEFT_PLAYER] );
	source->uint32( mEndScore[RIGHT_PLAYER] );

	source->generic<std::vector<unsigned char> >(mSaveData);
	source->generic<std::vector<ReplaySavePoint> > (mSavePoints);
}

void ReplayRecorder::record(const DuelMatchState& state)
{
	// save the state every REPLAY_SAVEPOINT_PERIOD frames
	// or when something interesting occurs
	if(mSaveData.size() % REPLAY_SAVEPOINT_PERIOD == 0 ||
		mEndScore[LEFT_PLAYER] != state.logicState.leftScore ||
		mEndScore[RIGHT_PLAYER] != state.logicState.rightScore)
	{
		ReplaySavePoint sp;
		sp.state = state;
		sp.step = mSaveData.size();
		mSavePoints.push_back(sp);
	}

	// we save this 1 here just for compatibility
	// set highest bit to 1
	unsigned char packet = 1 << 7;
	packet |= (state.playerInput[LEFT_PLAYER].getAll() & 7) << 3;
	packet |= (state.playerInput[RIGHT_PLAYER].getAll() & 7) ;
	mSaveData.push_back(packet);

	// update the score
	mEndScore[LEFT_PLAYER] = state.logicState.leftScore;
	mEndScore[RIGHT_PLAYER] = state.logicState.rightScore;
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

void ReplayRecorder::finalize(unsigned int left, unsigned int right)
{
	mEndScore[LEFT_PLAYER] = left;
	mEndScore[RIGHT_PLAYER] = right;

	// fill with one second of do nothing
	for(int i = 0; i < 75; ++i)
	{
		unsigned char packet = 0;
		mSaveData.push_back(packet);
	}
}
