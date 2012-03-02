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

#include "FileSystem.h"

#include <physfs.h>

std::vector<std::string> enumerateFiles(const std::string& directory, const std::string& extension)
{
	std::vector<std::string> files;
	char** filenames = PHYSFS_enumerateFiles(directory.c_str());
	
	// now test which files have type extension
	/// \todo this does not ensure that extension really is at the end of the string
	for (int i = 0; filenames[i] != 0; ++i)
	{
		std::string tmp = filenames[i];
		if (tmp.find(extension) != std::string::npos)
		{
			files.push_back(std::string(tmp.begin(), tmp.end() - extension.length()));
		}
	}
	
	// free the file list
	PHYSFS_freeList(filenames);
	
	return files;
}

bool deleteFile(const std::string& filename)
{
	return PHYSFS_delete(filename.c_str());
}
