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
#include "replays/ReplayRecorder.h"

/* includes */
#include <iostream>
#include <ctime>

#include <boost/algorithm/string/trim_all.hpp>

#include "tinyxml2.h"

#include "raknet/BitStream.h"

#include "Global.h"
#include "replays/ReplayDefs.h"
#include "replays/IReplayLoader.h"
#include "io/GenericIO.h"
#include "io/FileRead.h"
#include "io/FileWrite.h"
#include "base64.h"

#ifdef __SWITCH__
#define SIZET_TYPE unsigned int
#else
#define SIZET_TYPE std::uint64_t
#endif

/* implementation */
VersionMismatchException::VersionMismatchException(const std::string& filename, uint8_t major, uint8_t minor)
{
	std::stringstream errorstr;

	errorstr << "Error: Outdated replay file: " << filename <<
		std::endl << "expected version: " << (int)REPLAY_FILE_VERSION_MAJOR << "."
				<< (int)REPLAY_FILE_VERSION_MINOR <<
		std::endl << "got: " << (int)major << "." << (int)minor << " instead!" << std::endl;
	error = errorstr.str();
}

VersionMismatchException::~VersionMismatchException() noexcept = default;

const char* VersionMismatchException::what() const noexcept
{
	return error.c_str();
}



ReplayRecorder::ReplayRecorder()
{
	mGameSpeed = -1;
}

ReplayRecorder::~ReplayRecorder() = default;
template<class T>
void writeAttribute(tinyxml2::XMLPrinter& printer, const char* name, const T& value)
{
	printer.OpenElement("var");
	printer.PushAttribute("name", name);
	printer.PushAttribute("value", value);
	printer.CloseElement();
}

void ReplayRecorder::save( const std::shared_ptr<FileWrite>& file) const
{
	tinyxml2::XMLPrinter printer;
	printer.PushHeader(false, true);
	printer.OpenElement("replay");
	printer.OpenElement("version");
	printer.PushAttribute("major", REPLAY_FILE_VERSION_MAJOR);
	printer.PushAttribute("minor", REPLAY_FILE_VERSION_MINOR);
	printer.CloseElement();

	// the explicit template arguments below are to prevent ambiguities on OSX where
	// size_t = unsigned long != unsigned int != uint64_t; same for time_t
	writeAttribute(printer, "game_speed", mGameSpeed);
	writeAttribute<SIZET_TYPE>(printer, "game_length", mSaveData.size());
	writeAttribute<SIZET_TYPE>(printer, "game_duration", mSaveData.size() / mGameSpeed);
	writeAttribute<std::int64_t>(printer, "game_date", std::time(nullptr));

	writeAttribute(printer, "score_left", mEndScore[LEFT_PLAYER]);
	writeAttribute(printer, "score_right", mEndScore[RIGHT_PLAYER]);

	writeAttribute(printer, "name_left", mPlayerNames[LEFT_PLAYER].c_str());
	writeAttribute(printer, "name_right", mPlayerNames[RIGHT_PLAYER].c_str());

	/// \todo would be nice if we could write the actual colors instead of integers
	writeAttribute(printer, "color_left", mPlayerColors[LEFT_PLAYER].toInt());
	writeAttribute(printer, "color_right", mPlayerColors[RIGHT_PLAYER].toInt());

	// write the game rules
	printer.OpenElement("rules");
	printer.PushText(mGameRules.c_str());
	printer.CloseElement();

	// now comes the actual replay data
	printer.OpenElement("input");
	std::string binary = encode(mSaveData, 80);
	printer.PushText(binary.c_str());
	printer.CloseElement();

	// finally, write the save points
	// first, convert them into a POD
	printer.OpenElement("states");
	RakNet::BitStream stream;
	auto convert = createGenericWriter(&stream);
	convert->generic<std::vector<ReplaySavePoint> > (mSavePoints);

	binary = encode((char*)stream.GetData(), (char*)stream.GetData() + stream.GetNumberOfBytesUsed(), 80);
	printer.PushText(binary.c_str());
	printer.CloseElement();

	printer.CloseElement();  // </replay>
	file->write(printer.CStr(), printer.CStrSize() - 1); // do not save the terminating \0 character
}

void ReplayRecorder::send(const std::shared_ptr<GenericOut>& target) const
{
	target->string(mPlayerNames[LEFT_PLAYER]);
	target->string(mPlayerNames[RIGHT_PLAYER]);

	target->generic<Color> (mPlayerColors[LEFT_PLAYER]);
	target->generic<Color> (mPlayerColors[RIGHT_PLAYER]);

	target->uint32( mGameSpeed );
	target->uint32( mEndScore[LEFT_PLAYER] );
	target->uint32( mEndScore[RIGHT_PLAYER] );

	target->string(mGameRules);

	target->generic<std::vector<unsigned char> >(mSaveData);
	target->generic<std::vector<ReplaySavePoint> > (mSavePoints);
}

void ReplayRecorder::receive(const std::shared_ptr<GenericIn>& source)
{
	source->string(mPlayerNames[LEFT_PLAYER]);
	source->string(mPlayerNames[RIGHT_PLAYER]);

	source->generic<Color> (mPlayerColors[LEFT_PLAYER]);
	source->generic<Color> (mPlayerColors[RIGHT_PLAYER]);

	source->uint32( mGameSpeed );
	source->uint32( mEndScore[LEFT_PLAYER] );
	source->uint32( mEndScore[RIGHT_PLAYER] );

	source->string(mGameRules);

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
	unsigned char packet = 1u << 7u;
	packet |= (state.playerInput[LEFT_PLAYER].getAll() & 7u) << 3u;
	packet |= (state.playerInput[RIGHT_PLAYER].getAll() & 7u) ;
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

void ReplayRecorder::setGameRules( const std::string& rules )
{
	FileRead file(FileRead::makeLuaFilename("rules/"+rules));
	mGameRules.resize( file.length() );
	file.readRawBytes(&*mGameRules.begin(), file.length());
	boost::algorithm::trim_all(mGameRules);
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
