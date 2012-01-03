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
		PHYSFS_close( reinterpret_cast<PHYSFS_file*> (handle) );
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
	assert(handle);
	return PHYSFS_fileLength( reinterpret_cast<PHYSFS_file*> (handle) );
}

uint32_t File::tell() const
{
	assert(handle);
	return PHYSFS_tell( reinterpret_cast<PHYSFS_file*> (handle) );
}

uint32_t File::readRawBytes( char* target, std::size_t num_of_bytes )
{
	assert(handle);
	uint32_t num_read = PHYSFS_read(reinterpret_cast<PHYSFS_file*> (handle), target, 1, num_of_bytes);
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

void File::write(const std::string& data)
{
	write(data.data(), data.size());
}

void File::write(const char* data, std::size_t length)
{	
	assert(handle);
	PHYSFS_write(reinterpret_cast<PHYSFS_file*>(handle), data, 1, length);
}
