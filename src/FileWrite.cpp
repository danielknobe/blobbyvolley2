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
#include "FileWrite.h"

/* includes */
#include <cassert>

#include <physfs.h>

#include "Global.h"


/* implementation */

FileWrite::FileWrite()
{
}

FileWrite::FileWrite(const std::string& filename, bool no_override) : File(filename, File::OPEN_WRITE, no_override)
{
}

FileWrite::~FileWrite()
{
	// no more actions than what ~File already does
}

void FileWrite::open(const std::string& filename, bool no_override)
{
	File::open(filename, File::OPEN_WRITE, no_override);
}

void FileWrite::writeByte(char c)
{
	write(&c, sizeof(c));
}

void FileWrite::writeUInt32(uint32_t v)
{
	check_file_open();
	
	if( !PHYSFS_writeULE32( reinterpret_cast<PHYSFS_file*>(mHandle), v) )
	{
		BOOST_THROW_EXCEPTION( PhysfsFileException(mFileName) );
	}
}

void FileWrite::writeFloat(float fl)
{
	write(reinterpret_cast<char*>(&fl), sizeof(fl));
}

void FileWrite::write(const std::string& data)
{
	write(data.data(), data.size());
}

void FileWrite::writeNullTerminated(const std::string& data)
{
	write(data.c_str(), data.size() + 1);
}

void FileWrite::write(const char* data, std::size_t length)
{	
	check_file_open();
	
	if( PHYSFS_write(reinterpret_cast<PHYSFS_file*>(mHandle), data, 1, length) != length ) 
	{
		BOOST_THROW_EXCEPTION( PhysfsFileException(mFileName) );
	}
}
