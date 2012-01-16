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

#include "FileRead.h"

#include <physfs.h>
#include "Global.h"

#include <cassert>

FileRead::FileRead() 
{
}

FileRead::FileRead(const std::string& filename) : File(filename, File::OPEN_READ)
{
}

FileRead::~FileRead()
{
	// no more actions than what ~File already does
}

void FileRead::open(const std::string& filename)
{
	File::open(filename, File::OPEN_READ);
}

uint32_t FileRead::readRawBytes( char* target, std::size_t num_of_bytes )
{
	check_file_open();
	
	PHYSFS_sint64 num_read = PHYSFS_read(reinterpret_cast<PHYSFS_file*> (handle), target, 1, num_of_bytes);
	
	// -1 indicates that reading was not possible
	if( num_read == -1) 
	{
		throw( PhysfsException(name) );
	}
	/// \todo use expection error handling here, assert does not fit!
	if( num_read != num_of_bytes )
	{
		throw ( EOFException(name) );
	}
	
	return num_read;
}

boost::shared_array<char> FileRead::readRawBytes( std::size_t num_of_bytes )
{
	// creates the buffer
	boost::shared_array<char> buffer ( new char[num_of_bytes] );
	
	readRawBytes( buffer.get(), num_of_bytes );
	return buffer;
}

uint32_t FileRead::readUInt32()
{
	check_file_open();
	
	PHYSFS_uint32 ret;
	if(!PHYSFS_readULE32( reinterpret_cast<PHYSFS_file*>(handle),	&ret))
	{
		throw( PhysfsException(name) );
	}
	
	return ret; 
}

std::string FileRead::readString()
{
	char buffer[32]; 		// thats our read buffer
	std::string read = "";	// thats what we read so far
	uint32_t len = length();
	
	while(true)	// check that we can read as much as want
	{
		int maxread = std::min(sizeof(buffer), len - tell());
		readRawBytes( buffer, maxread );	// read into buffer
		
		for(int i=0; i < maxread; ++i)
		{
			if(buffer[i] == 0) 
			{
				seek( tell() - 32 + i + 1);
				return read;
			} 
			else 
			{
				read += buffer[i];	// this might not be the most efficient way...
			}
		}
		
		// when we reached the end of file
		if(maxread < 32)
			break;
	}
	
	assert(0);	// did not find zero-terminated-string
}


