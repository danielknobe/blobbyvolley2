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
#include "File.h"

/* includes */
#include <cassert>

#include <physfs.h>

#include "Global.h"
#include "FileSystem.h"

/* implementation */



File::File() : mHandle(0)
{
	
}

File::File(const std::string& filename, OpenMode mode, bool no_override) : mHandle(0), mFileName("")
{
	open(filename, mode, no_override);
}

File::~File() 
{
	// make sure we close this!
	close();
}

void File::open(const std::string& filename, OpenMode mode, bool no_override)
{
	// check that we don't have anything opened!
	/// \todo maybe we could just close the old file here... but
	///		  then, this could also lead to errors...
	assert(mHandle == 0);
	
	// open depending on mode
	if( mode == OPEN_WRITE ) 
	{
		if(no_override && FileSystem::getSingleton().exists(filename))
		{
			throw FileAlreadyExistsException(filename);
		}
		
		mHandle = PHYSFS_openWrite(filename.c_str());
	} 
	 else  
	{
		mHandle = PHYSFS_openRead(filename.c_str());
	}
	
	if (!mHandle)
	{
		throw FileLoadException(filename);
	}
	
	mFileName = filename;
}

void File::close() 
{
	// if handle is 0, no file is currently opened, so close does not do anything
	// maybe we could assert this, but i'm not sure that that is necessary.
	// we cannot assert this, because this function is run in the destrucor!
	if(mHandle) 
	{
		if (PHYSFS_close( reinterpret_cast<PHYSFS_file*> (mHandle) ) )
		{
			/// we can't throw an error here, as this function gets called
			/// in the destructor and therefore might be called while another
			/// excpetion is active so we cant throw.
		};
		mHandle = 0;
		mFileName = "";
	}
}

void* File::getPHYSFS_file()
{
	return mHandle;
}

bool File::is_open() const
{
	return mHandle;
}

uint32_t File::length() const
{
	check_file_open();
	
	PHYSFS_sint64 len = PHYSFS_fileLength( reinterpret_cast<PHYSFS_file*> (mHandle) );
	if( len == -1 )
	{
		throw( PhysfsFileException(mFileName) );
	}
	
	return len;
}

uint32_t File::tell() const
{
	check_file_open();
	
	PHYSFS_sint64 tp = PHYSFS_tell( reinterpret_cast<PHYSFS_file*> (mHandle) );
	
	if(tp == -1) 
		throw( PhysfsFileException(mFileName) );
	
	return tp;
}

std::string File::getFileName() const
{
	return mFileName;
}

void File::seek(uint32_t target)
{
	check_file_open();
	
	if(!PHYSFS_seek( reinterpret_cast<PHYSFS_file*>(mHandle), target)) 
	{
		throw( PhysfsFileException(mFileName) );
	}
}


void  File::check_file_open() const
{
	// check that we have a handle
	if( !mHandle ) 
	{
		throw( NoFileOpenedException() );
	}		
}


