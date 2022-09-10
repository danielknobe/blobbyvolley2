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
#include "FileSystem.h"

/* includes */
#include <cassert>
#include <iostream> /// \todo remove this? currently needed for that probeDir error messages

#include <physfs.h>

/* implementation */

FileSystem* mFileSystemSingleton = nullptr;

FileSystem::FileSystem(const std::string& path)
{
	assert(mFileSystemSingleton == nullptr);
	PHYSFS_init(path.c_str());
	/// \todo do we need to check if this operation suceeded?
	mFileSystemSingleton = this;
}

FileSystem& FileSystem::getSingleton()
{
	assert(mFileSystemSingleton);
	/// \todo instead of assert, throw exception?
	return *mFileSystemSingleton;
}

FileSystem::~FileSystem()
{
	PHYSFS_deinit();
	mFileSystemSingleton = nullptr;
}

std::vector<std::string> FileSystem::enumerateFiles(const std::string& directory, const std::string& extension, bool keepExtension)
{
	std::vector<std::string> files;
	char** filenames = PHYSFS_enumerateFiles(directory.c_str());
	
	// now test which files have type extension
	for (int i = 0; filenames[i] != nullptr; ++i)
	{
		std::string tmp = filenames[i];
		int position = tmp.length() - extension.length();
		if (position >= 0 && tmp.substr(position) == extension)
		{
			files.emplace_back(tmp.begin(), keepExtension ? (tmp.end()) : (tmp.end() - extension.length()) );
		}
	}
	
	// free the file list
	PHYSFS_freeList(filenames);
	
	return files;
}

bool FileSystem::deleteFile(const std::string& filename)
{
	return PHYSFS_delete(filename.c_str());
}

bool FileSystem::exists(const std::string& filename) const
{
	return PHYSFS_exists(filename.c_str());
}

bool FileSystem::isDirectory(const std::string& dirname) const
{
    if(!exists(dirname)) { return false; }

	PHYSFS_Stat stat;
	if ( !PHYSFS_stat(dirname.c_str(), &stat) )
		BOOST_THROW_EXCEPTION( PhysfsException() );

	return stat.filetype == PHYSFS_FILETYPE_DIRECTORY;
}

bool FileSystem::mkdir(const std::string& dirname)
{
	return PHYSFS_mkdir(dirname.c_str());
}

void FileSystem::addToSearchPath(const std::string& dirname, bool append)
{
	/// \todo check if dir exists?
	/// \todo check return value
	PHYSFS_mount(dirname.c_str(), nullptr, append ? 1 : 0);
}

void FileSystem::removeFromSearchPath(const std::string& dirname)
{
	PHYSFS_unmount(dirname.c_str());
}

void FileSystem::setWriteDir(const std::string& dirname)
{
	if( !PHYSFS_setWriteDir(dirname.c_str()) )
	{
		BOOST_THROW_EXCEPTION( PhysfsException() );
	}
	addToSearchPath(dirname, false);
}

std::string FileSystem::getDirSeparator()
{
	return PHYSFS_getDirSeparator();
}

std::string FileSystem::getUserDir()
{
	return PHYSFS_getUserDir();
}

void FileSystem::probeDir(const std::string& dirname)
{
	if ( !isDirectory(dirname) )
	{
		if (exists(dirname))
		{
			/// \todo simple delete such files without a warning???
			deleteFile(dirname);
		}
		
		if (mkdir(dirname))
		{
			std::cout << PHYSFS_getWriteDir() <<
				dirname << " created" << std::endl;
		}
		else
		{
			std::cout << "Warning: Creation of" << 
				PHYSFS_getWriteDir() << dirname <<
				" failed!" << std::endl;
		}
	}
}


// exception implementations

std::string makeSafePhysfsErrorString()
{
	const char* physfserror = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
	return physfserror != nullptr ? physfserror : "no physfs error message available.";
}


PhysfsException::PhysfsException() : mPhysfsErrorMsg( makeSafePhysfsErrorString() )
{
}

