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

#pragma once

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

#include "FileExceptions.h"
#include "BlobbyDebug.h"

// some convenience wrappers around physfs

class FileSystem : public boost::noncopyable, public ObjectCounter<FileSystem>
{
	public:
		FileSystem(const std::string& path);
		~FileSystem();

		/// \brief gets the file system
		/// \details throws an error when file system has
		/// not been initialised.
		static FileSystem& getSingleton();

		/// \brief enumerates files
		/// \details searches for all files in a certain directory with a given extension. The found files
		///			are returned as a vector containing the filenames. The extension is cuttet from the filenames.
		/// \param directory where to search
		///	\param extension file types to search for.
		/// \param keepExtension If true, the return vector contains the full filenames, if false [default behaviour], 
		///						only the filenames without the extensions are saved.
		std::vector<std::string> enumerateFiles(const std::string& directory, const std::string& extension, bool keepExtension = false);

		/// \brief deletes a file
		bool deleteFile(const std::string& filename);

		/// \brief tests whether a file exists
		bool exists(const std::string& filename) const;

		/// \brief tests wether given path is a directory
		bool isDirectory(const std::string& dirname) const;

		/// \brief creates a directory and reports success/failure
		/// \return true, if the directory could be created
		bool mkdir(const std::string& dirname);


		// general setup methods
		void addToSearchPath(const std::string& dirname, bool append = true);
		void removeFromSearchPath(const std::string& dirname);
		/// \details automatically registers this directory as primary read directory!
		void setWriteDir(const std::string& dirname);

		/// \todo this method is currently only copied code. it needs some review and a spec what it really should 
		/// do. also, its uses should be looked at again.
		void probeDir(const std::string& dir);

		/// \todo ideally, this method would never be needed by client code!!
		std::string getDirSeparator();

		/// \todo ideally, this method would never be needed by client code!!
		std::string getUserDir();
};
