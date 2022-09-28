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
#include "io/FileRead.h"

/* includes */
#include <cassert>

#include <physfs.h>

#include <boost/crc.hpp>
#include <boost/algorithm/string.hpp>

#include "tinyxml2.h"
#include "lua.hpp"


/* implementation */

FileRead::FileRead() = default;

FileRead::FileRead(const std::string& filename) : File(filename, File::OPEN_READ)
{
}

FileRead::~FileRead() = default;

void FileRead::open(const std::string& filename)
{
	File::open(filename, File::OPEN_READ);
}

uint32_t FileRead::readRawBytes( char* target, std::size_t num_of_bytes )
{
	check_file_open();

	PHYSFS_sint64 num_read = PHYSFS_readBytes(reinterpret_cast<PHYSFS_file*> (mHandle), target, num_of_bytes);

	// -1 indicates that reading was not possible
	if( num_read == -1)
	{
		BOOST_THROW_EXCEPTION( PhysfsFileException(mFileName) );
	}

	if( num_read != (PHYSFS_sint64)num_of_bytes )
	{
		BOOST_THROW_EXCEPTION ( EOFException(mFileName) );
	}

	return num_read;
}

std::vector<char> FileRead::readRawBytes( std::size_t num_of_bytes )
{
	// creates the buffer
	std::vector<char> buffer ( num_of_bytes );

	readRawBytes( buffer.data(), num_of_bytes );
	return buffer;
}

char FileRead::readByte()
{
	check_file_open();

	char ret;
	readRawBytes(reinterpret_cast<char*>(&ret), sizeof(ret));

	return ret;
}

uint32_t FileRead::readUInt32()
{
	check_file_open();

	if ( length() - tell() < 4)
	{
		BOOST_THROW_EXCEPTION( EOFException(mFileName) );
	}

	PHYSFS_uint32 ret;
	if(!PHYSFS_readULE32( reinterpret_cast<PHYSFS_file*>(mHandle),	&ret))
	{
		BOOST_THROW_EXCEPTION( PhysfsFileException(mFileName) );
	}

	return ret;
}

float FileRead::readFloat()
{
	check_file_open();

	float ret;
	readRawBytes(reinterpret_cast<char*>(&ret), sizeof(ret));

	return ret;
}


std::string FileRead::readString()
{
	char buffer[32]; 		// that's our read buffer
	std::string read;	    // that's what we read so far
	size_t len = length();

	while(true)	// check that we can read as much as want
	{
		int max_read = std::min( sizeof(buffer), len - tell());
		readRawBytes( buffer, max_read );	// read into buffer

		for( int i = 0; i < max_read; ++i)
		{
			if(buffer[i] == 0)
			{
				seek( tell() - max_read + i + 1);
				return read;
			}
			else
			{
				read += buffer[i];	// this might not be the most efficient way...
			}
		}

		// when we reached the end of file
		if( max_read < 32)
			break;
	}

	BOOST_THROW_EXCEPTION(EOFException(mFileName));
}


uint32_t FileRead::calcChecksum(uint32_t start)
{
	uint32_t old_position = tell();
	seek(start);

	// buffered reading
	char buffer[128];
	size_t len = length();

	boost::crc_32_type crc;

	while(true)
	{
		int max_read = std::min( sizeof(buffer), len - tell());
		readRawBytes( buffer, max_read );	// read into buffer

		for( int i = 0; i < max_read; ++i)
		{
			crc.process_bytes( buffer, max_read);
		}

		if( max_read < 32)
			break;
	}

	// return read pointer back to old position
	seek( old_position);

	return crc();
}




// reading lua script

struct ReaderInfo
{
	FileRead file;
	char buffer[2048];
};

static const char* chunkReader(lua_State* state, void* data, size_t *size)
{
	ReaderInfo* info = (ReaderInfo*) data;

	int bytesRead = 2048;
	if(info->file.length() - info->file.tell() < 2048)
	{
		bytesRead = info->file.length() - info->file.tell();
	}

	info->file.readRawBytes(info->buffer, bytesRead);
	// if this doesn't throw, bytesRead is the actual number of bytes read
	/// \todo we must do sth about this code, its just plains awful.
	/// 		File interface has to be improved to support such buffered reading.
	*size = bytesRead;
	if (bytesRead == 0)
	{
		return nullptr;
	}
	else
	{
		return info->buffer;
	}
}

int FileRead::readLuaScript(const std::string& filename, lua_State* mState)
{
	ReaderInfo info;
	info.file.open(makeLuaFilename(filename));
	return lua_load(mState, chunkReader, &info, filename.c_str(), nullptr);
}

std::string FileRead::makeLuaFilename(std::string filename)
{
	if( !boost::ends_with(filename, ".lua") )
		filename += ".lua";
	return filename;
}

XMLDocumentPtr FileRead::readXMLDocument(const std::string& filename)
{
	// create and load file
	FileRead file(filename);

	// that's quite ugly
	int fileLength = file.length();
	std::vector<char> fileBuffer(fileLength + 1);
	file.readRawBytes( fileBuffer.data(), fileLength );
	// null-terminate
	fileBuffer[fileLength] = 0;

	// parse file
    XMLDocumentPtr xml = std::make_shared<tinyxml2::XMLDocument>();
	xml->Parse(fileBuffer.data());

	/// \todo do error handling here?

	return xml;
}
