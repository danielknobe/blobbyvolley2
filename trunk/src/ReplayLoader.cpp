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

#include "IReplayLoader.h"
#include "InputSource.h"

#include <cassert>
#include <algorithm>
#include <ctime>
#include <iostream> // debugging

#include <boost/crc.hpp>

#include "FileRead.h"

IReplayLoader* IReplayLoader::createReplayLoader(const std::string& filename)
{
	// do some generic loading stuff:
	// first, try to open the file
	FileRead file(filename);
	
	// then, check the file length. We need at least 12 bytes.
	int fileLength = file.length();	
	if (fileLength < 12)
	{
		/// \todo add some error handling here!
		return 0;
	}
	
	// check if file contains a valid BV2 header
	char header[4];
	file.readRawBytes(header, sizeof(header));
	if (memcmp(&header, &validHeader, 4) != 0)
	{
		/// \todo add some error handling here!
		return 0;
	}
	
	// now, find out which version we need!
	char version[4];
	file.readRawBytes(version, sizeof(version));
	
	// now we got our version number.
	int major = version[1];
	int minor = version[2];
	
	// read checksum
	uint32_t checksum;
	file.readRawBytes((char*)&checksum, 4);
	//PHYSFS_read(fileHandle, &checksum, 1, 4);
	
	IReplayLoader* loader = createReplayLoader(major);
	loader->initLoading(file, minor, checksum);
	
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
			              R E P L A Y   L O A D E R    V 1.0
***************************************************************************************************/


/*! \class ReplayLoader_V10
	\brief Replay Loader V 1.0
	\details Replay Loader for 1.0 replays
*/
class ReplayLoader_V10: public IReplayLoader
{
	public:
		ReplayLoader_V10() {};
		
		virtual ~ReplayLoader_V10() { };
		
		virtual int getVersionMajor() const { return 1; };
		virtual int getVersionMinor() const { return 0; };
		
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
		
	private:
		virtual void initLoading(FileRead& file, int minor_version, uint32_t checksum)
		{
			/// \todo check if minor_version < getVersionMinor, otherwise issue a warning
			/// \todo add checksum checking. We should think about our checksum too!
			
			int fileLength = file.length();
			
			// we start with the replay header.
			uint32_t header_size, attr_ptr , attr_size ,
					jptb_ptr, jptb_size , data_ptr , data_size;
			
			header_size = file.readUInt32();
			attr_ptr = file.readUInt32();
			attr_size = file.readUInt32();
			jptb_ptr = file.readUInt32();
			jptb_size = file.readUInt32();
			data_ptr = file.readUInt32();
			data_size = file.readUInt32();
			
			// now, we read the attributes section
			//  jump over the attr - marker
			file.seek(attr_ptr + 4);
			// copy attributes into buffer
			
			// read the attributes
			mGameSpeed = file.readUInt32();
			mGameDuration = file.readUInt32();
			mGameLength = file.readUInt32();
			mGameDate = file.readUInt32();
			
			mLeftColor = file.readUInt32();
			mRightColor = file.readUInt32();
			
			mLeftPlayerName = file.readString();
			mRightPlayerName = file.readString();
			
			// now, read the raw data
			file.seek(data_ptr + 8);		// jump over the dat marker and over the length value
			/// \todo why do we set mBufferSize again? should we check if these two are identical
			// read into buffer
			std::cout << "read into buffer\n";
			mBuffer = file.readRawBytes(data_size);
			mReplayOffset = 0;

			/*
			boost::crc_32_type realcrc;
			realcrc(mServingPlayer);
			realcrc = std::for_each(mLeftPlayerName.begin(), mLeftPlayerName.end(), realcrc);
			realcrc = std::for_each(mRightPlayerName.begin(), mRightPlayerName.end(), realcrc);
			realcrc.process_bytes(mBuffer + mReplayOffset, mBufferSize - mReplayOffset);

			if (realcrc.checksum() != checksum)
			{
				/// \todo here, we don't know the filename anymore
				//throw ChecksumException(file, checksum, realcrc.checksum());
			}
			*/
		}
		
		boost::shared_array<char> mBuffer;
		uint32_t mReplayOffset;
		
		// specific data
		std::string mLeftPlayerName;
		std::string mRightPlayerName;
		int mGameSpeed;
		int mGameDate;
		int mGameLength;
		int mGameDuration;
		Color mLeftColor;
		Color mRightColor;
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
			return new ReplayLoader_V10();
			break;
	}
	
	// fallback
	return 0;
}
