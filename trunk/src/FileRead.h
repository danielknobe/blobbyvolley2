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

#pragma once

#include "File.h"

/**
	\class FileRead
*/
class FileRead : public File
{
	public:
	
		/// \brief default ctor
		/// \details File has to be opended with open()
		/// \throw nothing
		explicit FileRead() nothrow(true);
		
		/// \brief constructor which opens a file.
		/// \param filename File to be opened for reading
		/// \throw FileLoadException, if the file could not be loaded
		FileRead(const std::string& filename);
		
		/// \brief opens a file.
		/// \param filename File to be opened for reading
		/// \throw FileLoadException, if the file could not be loaded
		/// \pre No file is currently opened.
		void open(const std::string& filename);
		
		/// destructor, closes the file (if any open)
		/// \sa close()
		/// \throw nothing
		~FileRead() nothrow(true);
		
		// ------------------------------------
		//  reading interface
		// ------------------------------------
		/// reads bytes into a buffer
		/// \param target buffer to read into
		/// \param num_of_bytes number of bytes to read
		/// \throw PhysfsException when nothing could be read
		/// \throw NoFileOpenedException when called while no file is opened.
		/// \throw EOFException when cless than \p num_of_bytes bytes are available.
		uint32_t readRawBytes( char* target, std::size_t num_of_bytes );
		
		
		/// reads bytes and returns a safe-pointed buffer
		/// the buffer is allocated by this function and has a size of \p num_of_bytes
		/// \param num_of_bytes Number of bytes to read; size of buffer
		/// \throw PhysfsException when nothing could be read
		/// \throw NoFileOpenedException when called while no file is opened.
		/// \throw EOFException when cless than \p num_of_bytes bytes are available.
		boost::shared_array<char> readRawBytes( std::size_t num_of_bytes );
		
		/// reads an unsinged 32 bit integer from the next four bytes in the file
		/// the integer is expected to be in little-endian-order and is converted
		/// to the native format.
		/// \throw PhysfsException when Physfs reports an error
		/// \throw NoFileOpenedException when called while no file is opened.
		uint32_t readUInt32();
		
		/// reads a null-terminated string from the file
		/// \throw PhysfsException when Physfs reports an error
		/// \throw NoFileOpenedException when called while no file is opened.
		std::string readString();
};
