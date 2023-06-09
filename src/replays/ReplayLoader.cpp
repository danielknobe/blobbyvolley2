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
#include "IReplayLoader.h"

/* includes */
#include <cassert>
#include <vector>
#include <ctime>
#include <memory>
#include <iostream> // debugging

#include "tinyxml2.h"

#include "raknet/BitStream.h"

#include "InputSource.h"
#include "FileRead.h"
#include "GenericIO.h"
#include "base64.h"
#include "ReplayDefs.h"
#include "Color.h"

/* implementation */
std::unique_ptr<IReplayLoader> IReplayLoader::createReplayLoader(const std::string& filename)
{
	auto loader = createReplayLoader(0);
	loader->initLoading(filename);

	return loader;
}

//
// -------------------------------------------------------------------------------------------------
//

/***************************************************************************************************
			              R E P L A Y   L O A D E R    V 2.x
***************************************************************************************************/


/*! \class ReplayLoader_V2X
	\brief Replay Loader V 2.x
	\details Replay Loader for 2.0 replays
*/
class ReplayLoader_V2X: public IReplayLoader
{
	public:
		ReplayLoader_V2X() = default;

		~ReplayLoader_V2X() override = default;

		int getVersionMajor() const override { return 2; };
		int getVersionMinor() const override { return 0; };

		std::string getPlayerName(PlayerSide player) const override
		{
			if(player == LEFT_PLAYER)
				return mLeftPlayerName;
			if(player == RIGHT_PLAYER)
				return mRightPlayerName;

			assert(0);
		}

		Color getBlobColor(PlayerSide player) const override
		{
			if(player == LEFT_PLAYER)
				return mLeftColor;
			if(player == RIGHT_PLAYER)
				return mRightColor;

			assert(0);
		}


		int getFinalScore(PlayerSide player) const override
		{
			if(player == LEFT_PLAYER)
				return mLeftFinalScore;
			if(player == RIGHT_PLAYER)
				return mRightFinalScore;

			assert(0);
		}

		int getSpeed() const override
		{
			return mGameSpeed;
		};

		int getDuration() const override
		{
			return mGameDuration;
		};

		int getLength()  const override
		{
			return mGameLength;
		};

		std::time_t getDate() const override
		{
			return mGameDate;
		};

		std::string getRules() const override
		{
			return mRules;
		}


		void getInputAt(int step, InputSource* left, InputSource* right) override
		{
			assert( step  < mGameLength );

			// for now, we have only a linear sequence of InputPackets, so finding the right one is just
			// a matter of address arithmetics.

			// each packet has size 1 byte for now
			// so we find step at mReplayOffset + step
			char packet = mBuffer[mReplayOffset + step];

			// now read the packet data
			left->setInput(PlayerInput((bool)(packet & 32), (bool)(packet & 16), (bool)(packet & 8)));
			right->setInput(PlayerInput((bool)(packet & 4), (bool)(packet & 2), (bool)(packet & 1)));
		}

		bool isSavePoint(int position, int& save_position) const override
		{
			int foundPos;
			save_position = getSavePoint(position, foundPos);
			return save_position != -1 && foundPos == position;
		}

		// TODO: add optional argument: int previous = 0;
		// 		so we can start from it when calling
		// 		getSavePoint in a row (without "jumping").
		// 		we can save this parameter in ReplayPlayer
		int getSavePoint(int targetPosition, int& savepoint) const override
		{
			// desired index can't be lower that this value,
			// cause additional savepoints could shift it only right
			int index = targetPosition / REPLAY_SAVEPOINT_PERIOD;

			if(index >= static_cast<int>(mSavePoints.size()))
				return -1;

			savepoint = mSavePoints[index].step;

			// watch right from initial index,
			// cause best savepoint could be there.
			// we have no much additional savepoints,
			// so this cycle would be fast,
			// maybe even faster than binary search.
			do
			{
				int nextIndex = index + 1;

				if (nextIndex >= static_cast<int>(mSavePoints.size()))
					break;

				int nextPos = mSavePoints[nextIndex].step;

				if (nextPos > targetPosition)
					break;

				index = nextIndex;
				savepoint = nextPos;
			} while (true);

			return index;
		}

		void readSavePoint(int index, ReplaySavePoint& state) const override
		{
			state = mSavePoints.at(index);
		}

	private:
		void initLoading(std::string filename) override
		{
			auto configDoc = FileRead::readXMLDocument(filename);

			if (configDoc->Error())
			{
				std::cerr << "Warning: Parse error in " << filename << "!" << std::endl;
				throw( std::runtime_error("") );
			}

			const auto* userConfigElem = configDoc->FirstChildElement("replay");
			if (userConfigElem == nullptr)
				throw(std::runtime_error("No <replay> node found!"));

			const auto* varElem = userConfigElem->FirstChildElement("version");
			// the first element we find is expected to be the version
			if(!varElem)
			{
				throw( std::runtime_error("Could not find <version> tag") );
			}
			else
			{
				const char* major = varElem->Attribute("major");
				const char* minor = varElem->Attribute("minor");
				if(!minor || !major)
					throw(std::runtime_error(""));
				assert(std::stoi(major) == 2);
				mReplayFormatVersion = std::stoi(minor);
			}


			for (; varElem; varElem = varElem->NextSiblingElement("var"))
			{
				std::string name, value;
				if (auto c = varElem->Attribute("name"))
					name = c;
				else
					continue;

				if (auto c = varElem->Attribute("value"))
					value = c;
				else
					continue;

				// find valid attribute
				if( name == "game_speed" )
					mGameSpeed = std::stoi(value);
				else if( name == "game_length" )
					mGameLength = std::stoi(value);
				else if( name == "game_duration" )
					mGameDuration = std::stoi(value);
				else if( name == "game_date" )
					mGameDate = std::stoi(value);
				else if( name == "score_left" )
					mLeftFinalScore = std::stoi(value);
				else if( name == "score_right" )
					mRightFinalScore = std::stoi(value);
				else if( name == "name_left" )
					mLeftPlayerName = value;
				else if( name == "name_right" )
					mRightPlayerName = value;
				else if( name == "color_left" )
					mLeftColor = Color(std::stoi(value));
				else if( name == "color_right" )
					mRightColor = Color(std::stoi(value));
			}

			// load rules
			varElem = userConfigElem->FirstChildElement("rules");
			if(!varElem)
				throw(std::runtime_error(""));
			auto content = varElem->FirstChild();
			if(!content)
				throw(std::runtime_error(""));

			mRules = content->Value();

			// now load buffer and savepoints
			varElem = userConfigElem->FirstChildElement("input");
			if(!varElem)
				throw(std::runtime_error(""));
			content = varElem->FirstChild();
			if(!content)
				throw(std::runtime_error(""));

			mBuffer = decode(content->Value());

			varElem = userConfigElem->FirstChildElement("states");
			if(!varElem)
				throw(std::runtime_error(""));
			content = varElem->FirstChild();
			if(!content)
				throw(std::runtime_error(""));

			// get save points
			auto sp = decode( content->Value() );
			RakNet::BitStream temp( sp.data(), sp.size(), false );
			auto convert = createGenericReader(&temp);
			convert->generic<std::vector<ReplaySavePoint> > (mSavePoints);
		}


		std::vector<uint8_t> mBuffer;
		uint32_t mReplayOffset;

		std::vector<ReplaySavePoint> mSavePoints;

		// specific data
		std::string mLeftPlayerName;
		std::string mRightPlayerName;
		unsigned int mLeftFinalScore;
		unsigned int mRightFinalScore;
		unsigned int mGameSpeed;
		unsigned int mGameDate;
		int mGameLength;
		unsigned int mGameDuration;
		Color mLeftColor;
		Color mRightColor;

		std::string mRules;

		unsigned char mReplayFormatVersion;
};


std::unique_ptr<IReplayLoader> IReplayLoader::createReplayLoader(int major)
{
	return std::make_unique<ReplayLoader_V2X>();
}
