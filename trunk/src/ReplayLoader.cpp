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
#include <algorithm>
#include <vector>
#include <ctime>
#include <iostream> // debugging

#include <boost/crc.hpp>
#include <boost/make_shared.hpp>

#include "InputSource.h"
#include "FileRead.h"
#include "GenericIO.h"

/* implementation */
IReplayLoader* IReplayLoader::createReplayLoader(const std::string& filename)
{
	// do some generic loading stuff:
	// first, try to open the file
	boost::shared_ptr<FileRead> file = boost::make_shared<FileRead>(filename);
	
	// then, check the file length. We need at least 12 bytes.
	int fileLength = file->length();	
	if (fileLength < 12)
	{
		/// \todo add some error handling here!
		return 0;
	}
	
	// check if file contains a valid BV2 header
	char header[4];
	file->readRawBytes(header, sizeof(header));
	if (memcmp(&header, &validHeader, 4) != 0)
	{
		/// \todo add some error handling here!
		return 0;
	}
	
	// now, find out which version we need!
	char version[4];
	file->readRawBytes(version, sizeof(version));
	
	// now we got our version number.
	int major = version[1];
	int minor = version[2];
	
	// read checksum
	uint32_t checksum;
	file->readRawBytes((char*)&checksum, 4);
	
	// calculate reference checksum
	uint32_t refchecksum = file->calcChecksum(file->tell());
	
	if(refchecksum != checksum && major > 0 && minor > 0)
	{
		throw(ChecksumException(file->getFileName(), checksum, refchecksum));
	}
	
	IReplayLoader* loader = createReplayLoader(major);
	boost::shared_ptr<GenericIn> in = createGenericReader(file);
	loader->initLoading(in, minor);
	
	return loader;
}

/***************************************************************************************************
			              R E P L A Y   L O A D E R    V 0.1
***************************************************************************************************/

// That version used different physics than we use now, and it does not include any save-points that would
// allow to extrapolate the match, so we could not play these matches, even if we had a loader for them

//
// -------------------------------------------------------------------------------------------------
// 

/***************************************************************************************************
			              R E P L A Y   L O A D E R    V 1.x
***************************************************************************************************/


/*! \class ReplayLoader_V1X
	\brief Replay Loader V 1.x
	\details Replay Loader for 1.0 and 1.1 replays
*/
class ReplayLoader_V1X: public IReplayLoader
{
	public:
		ReplayLoader_V1X() {};
		
		virtual ~ReplayLoader_V1X() { };
		
		virtual int getVersionMajor() const { return 1; };
		virtual int getVersionMinor() const { return 1; };
		
		virtual std::string getPlayerName(PlayerSide player) const
		{
			if(player == LEFT_PLAYER)
				return mLeftPlayerName;
			if(player == RIGHT_PLAYER)
				return mRightPlayerName;
				
			assert(0);
		}
		
		virtual Color getBlobColor(PlayerSide player) const
		{
			if(player == LEFT_PLAYER)
				return mLeftColor;
			if(player == RIGHT_PLAYER)
				return mRightColor;
				
			assert(0);
		}
		
				
		virtual int getFinalScore(PlayerSide player) const
		{
			if(player == LEFT_PLAYER)
				return mLeftFinalScore;
			if(player == RIGHT_PLAYER)
				return mRightFinalScore;
				
			assert(0);
		}
		
		virtual int getSpeed() const
		{
			return mGameSpeed;
		};
		
		virtual int getDuration() const
		{
			return mGameDuration;
		};
		
		virtual int getLength()  const
		{
			return mGameLength;
		};
		
		virtual std::time_t getDate() const
		{
			return mGameDate;
		};
		
		
		virtual void getInputAt(int step, PlayerInput& left, PlayerInput& right)
		{
			assert( step  < mGameLength );
			
			// for now, we have only a linear sequence of InputPackets, so finding the right one is just 
			// a matter of address arithmetics.
			
			// each packet has size 1 byte for now
			// so we find step at mReplayOffset + step
			char packet = mBuffer[mReplayOffset + step];
			
			// now read the packet data
			left.set((bool)(packet & 32), (bool)(packet & 16), (bool)(packet & 8));
			right.set((bool)(packet & 4), (bool)(packet & 2), (bool)(packet & 1));
		}
		
		virtual bool isSavePoint(int position, int& save_position) const
		{
			if(position % 750 == 0 && mReplayFormatVersion != 0)
			{
				save_position = position / 750;
				return true;
			}
			return false;
		}
		
		virtual int getSavePoint(int targetPosition, int& savepoint) const
		{
			/// \todo detect if replay contains safepoints
			int index = targetPosition / 750;
			savepoint = index * 750;
			return index;
			if(index < mSavePoints.size())
				return index;
			
			return -1;
		}
		
		virtual void readSavePoint(int index, ReplaySavePoint& state) const
		{
			state = mSavePoints.at(index);
		}
		
	private:
		virtual void initLoading(boost::shared_ptr<GenericIn> file, int minor_version)
		{
			mReplayFormatVersion = minor_version;
			mSavePoints.resize(0);
			/// \todo check if minor_version < getVersionMinor, otherwise issue a warning
			
			// we start with the replay header.
			uint32_t header_size, attr_ptr , attr_size ,
					jptb_ptr, jptb_size , data_ptr , data_size,
					states_ptr, states_size;
			
			file->uint32(header_size);
			file->uint32(attr_ptr);
			file->uint32(attr_size);
			file->uint32(jptb_ptr);
			file->uint32(jptb_size);
			file->uint32(data_ptr);
			file->uint32(data_size);
			
			// legacy support for 1.0 RC 1 replays
			if(minor_version != 0)
			{
				file->uint32(states_ptr);
				file->uint32(states_size);
			}
			
			// now, we read the attributes section
			//  jump over the attr - marker
			file->seek(attr_ptr + 4);
			// copy attributes into buffer
			
			// read the attributes
			file->uint32(mGameSpeed);
			file->uint32(mGameDuration);
			file->uint32(mGameLength);
			file->uint32(mGameDate);
			
			file->generic<Color> (mLeftColor);
			file->generic<Color> (mRightColor);
			
			if(minor_version != 0)
			{
				file->uint32(mLeftFinalScore);
				file->uint32(mRightFinalScore);
			
				file->string(mLeftPlayerName);
				file->string(mRightPlayerName);
			} 
			 else
			{
				mLeftPlayerName = "";
				
				unsigned char c;
				do
				{
					file->byte(c);
					mLeftPlayerName += c;
				} while(c);
				
				mRightPlayerName = "";
				
				do
				{
					file->byte(c);
					mRightPlayerName += c;
				} while(c);
			}
			
			// now, read the raw data
			file->seek(data_ptr + 8);		// jump over the dat marker and over the length value
			/// \todo why do we set mBufferSize again? should we check if these two are identical
			// read into buffer
			std::cout << "read into buffer\n";
			std::cout << "length: " << mGameLength << "   " << data_size << "\n";
			mBuffer = boost::shared_array<char>(new char[data_size]);
			file->array(mBuffer.get(), data_size);
			mReplayOffset = 0;
			
			// now read safepoints
			if(minor_version != 0)
			{
				file->seek(states_ptr + 4);		// jump over the sta marker
				file->uint32(mSavePointsCount);
				std::cout << "read " << mSavePointsCount << " safe points\n";
				mSavePoints.reserve(mSavePointsCount);
				for(int i = 0; i < mSavePointsCount; ++i)
				{
				
					DuelMatchState ms;
					
					file->generic<DuelMatchState>(ms);					
					
					ReplaySavePoint sp;
					sp.state = ms;
					sp.step = 750 * i;
					
					mSavePoints.push_back(sp);
				}
			}
			 else
			{
				mSavePointsCount = 0;
			}
			
			/// \todo check that mSafePointsCount and states_size match

		}

		
		boost::shared_array<char> mBuffer;
		uint32_t mReplayOffset;
		
		std::vector<ReplaySavePoint> mSavePoints;
		uint32_t mSavePointsCount;
		
		// specific data
		std::string mLeftPlayerName;
		std::string mRightPlayerName;
		unsigned int mLeftFinalScore;
		unsigned int mRightFinalScore;
		unsigned int mGameSpeed;
		unsigned int mGameDate;
		unsigned int mGameLength;
		unsigned int mGameDuration;
		Color mLeftColor;
		Color mRightColor;
		
		unsigned char mReplayFormatVersion;
};


IReplayLoader* IReplayLoader::createReplayLoader(int major)
{
	// we find a loader depending on major version
	/// \todo throw, when version is too old
	switch(major)
	{
		case 0:
			0;
			break;
		case 1:
			return new ReplayLoader_V1X();
			break;
	}
	
	// fallback
	return 0;
}
