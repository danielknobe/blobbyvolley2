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

#include "File.h"
#include <physfs.h>
#include "global.h"

#include <cassert>

File::File() : handle(0)
{
	
}

File::File(const std::string& filename, OpenMode mode) : handle(0)
{
	open(filename, mode);
}

File::~File() 
{
	// make sure we close this!
	close();
}

void File::open(const std::string& filename, OpenMode mode)
{
	// check that we don't have anything opened!
	/// \todo maybe we could just close the old file here... but
	///		  then, this could also lead to errors...
	assert(handle == 0);
	
	// open depending on mode
	if( mode == OPEN_WRITE ) 
	{
		handle = PHYSFS_openWrite(filename.c_str());
	} 
		else  
	{
		handle = PHYSFS_openRead(filename.c_str());
	}
	
	if (!handle)
	{
		throw FileLoadException(filename);
	}
}

void File::close() 
{
	// if handle is 0, no file is currently opened, so close does not do anything
	// maybe we could assert this, but i'm not sure that that is necessary.
	// we cannot assert this, because this function is run in the destrucor!
	if(handle) 
	{
		if (PHYSFS_close( reinterpret_cast<PHYSFS_file*> (handle) ) )
		{
			/// we can't throw an error here, as this function gets called
			/// in the destructor and therefore might be called while another
			/// excpetion is active so we cant throw.
		};
		handle = 0;
	}
}

void* File::getPHYSFS_file()
{
	return handle;
}

bool File::is_open() const
{
	return handle;
}

uint32_t File::length() const
{
	check_file_open();
	
	PHYSFS_sint64 len = PHYSFS_fileLength( reinterpret_cast<PHYSFS_file*> (handle) );
	if( len == -1 )
	{
		throw( PhysfsException() );
	}
	
	return len;
}

uint32_t File::tell() const
{
	check_file_open();
	
	PHYSFS_sint64 tp = PHYSFS_tell( reinterpret_cast<PHYSFS_file*> (handle) );
	if(tp == -1) 
		throw( PhysfsException() );
	
	return tp;
}

uint32_t File::readRawBytes( char* target, std::size_t num_of_bytes )
{
	check_file_open();
	
	PHYSFS_sint64 num_read = PHYSFS_read(reinterpret_cast<PHYSFS_file*> (handle), target, 1, num_of_bytes);
	
	// -1 indicates that reading was not possible
	if( num_read == -1) 
	{
		throw( PhysfsException() );
	}
	/// \todo use expection error handling here, assert does not fit!
	assert(num_read == num_of_bytes );
	return num_read;
}

boost::shared_array<char> File::readRawBytes( std::size_t num_of_bytes )
{
	// creates the buffer
	boost::shared_array<char> buffer ( new char[num_of_bytes] );
	
	readRawBytes( buffer.get(), num_of_bytes );
	return buffer;
}

uint32_t File::readUInt32()
{
	check_file_open();
	
	PHYSFS_uint32 ret;
	if(!PHYSFS_readULE32( reinterpret_cast<PHYSFS_file*>(handle),	&ret))
	{
		throw( PhysfsException() );
	}
	
	return ret; 
}

std::string File::readString()
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

void File::seek(uint32_t target)
{
	check_file_open();
	
	if(!PHYSFS_seek( reinterpret_cast<PHYSFS_file*>(handle), target)) 
	{
		throw( PhysfsException() );
	}
}

void File::writeByte(char c)
{
	write(&c, sizeof(c));
}

void File::writeUInt32(uint32_t v)
{
	check_file_open();
	
	if( !PHYSFS_writeULE32( reinterpret_cast<PHYSFS_file*>(handle), v) )
	{
		throw( PhysfsException() );
	}
}

void File::write(const std::string& data)
{
	write(data.data(), data.size());
}

void File::writeNullTerminated(const std::string& data)
{
	write(data.c_str(), data.size() + 1);
}

void File::write(const char* data, std::size_t length)
{	
	check_file_open();
	
	if( PHYSFS_write(reinterpret_cast<PHYSFS_file*>(handle), data, 1, length) != length ) 
	{
		throw( PhysfsException() );
	}
}

void  File::check_file_open() const
{
	// check that we have a handle
	if(!handle) {
		throw( NoFileOpenedException() );
	}		
}




PhysfsException::PhysfsException() : physfsErrorMsg( PHYSFS_getLastError() )
{
}

