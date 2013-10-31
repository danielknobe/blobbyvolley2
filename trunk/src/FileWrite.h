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

#include "File.h"

/**
	\class FileWrite
	\brief File interface extesion for writing.
	\details This class provides methods for writing various data to files. Whenever
			a file is opened for writing, all its previous content is erased, or, in case
			it did not exist beforehand, the file is created.
	\sa FileRead
*/
class FileWrite : public File
{
	public:
	
		/// \brief default ctor
		/// \details File has to be opended with open()
		/// \throw nothing
		explicit FileWrite();
		
		/// \brief constructor which opens a file.
		/// \param filename File to be opened for writing
		/// \param no_override Set to true if you want to forbid writing over existing file.
		/// \throw FileLoadException, if the file could not be loaded
		/// \throw FileAlreadyExistsException in case of trying to write over existing file with no_override = true
		FileWrite(const std::string& filename, bool no_override = false);
		
		/// \brief opens a file.
		/// \param filename File to be opened for writing
		/// \param no_override Set to true if you want to forbid writing over existing file.
		/// \throw FileLoadException, if the file could not be created
		/// \throw FileAlreadyExistsException in case of trying to write over existing file with no_override = true
		/// \pre No file is currently opened.
		void open(const std::string& filename, bool no_override = false);
		
		/// destructor, closes the file (if any open)
		/// \sa close()
		/// \throw nothing
		~FileWrite();
		
		// ------------------------------------
		//  writing interface
		// ------------------------------------
		
		/// \brief writes one character
		/// \details writes exactly the one character supplied.
		/// \throw PhysfsFileException when Physfs reports an error.
		/// \throw NoFileOpenedException when called while no file is opened.
		void writeByte(char c);
		
		/// \brief writes 32 bit integer
		/// \details writes an integer, converted to little endian if necessary
		/// \throw PhysfsFileException when Physfs reports an error.
		/// \throw NoFileOpenedException when called while no file is opened.
		void writeUInt32(uint32_t v);
		
		/// \brief writes a single precision float
		/// \throw PhysfsException when Physfs reports an error.
		/// \throw NoFileOpenedException when called while no file is opened.
		void writeFloat(float fl);
		
		/// \brief writes a std::string
		/// \details writes the content of the string to the file.
		/// 		does not write a null-termination character
		/// \throw PhysfsFileException when Physfs reports an error
		/// \throw NoFileOpenedException when called while no file is opened.
		void write(const std::string& data);
		
		/// \brief writes null-terminated std::string
		/// \details writes the content of the string to the file.
		/// 		ends the string with a null terminator.
		/// \throw NoFileOpenedException when called while no file is opened.
		/// \throw PhysfsFileException when Physfs reports an error
		void writeNullTerminated(const std::string& data);
		
		/// \brief writes a sequence of characters
		/// \details writes \p length characters from \p data to the file
		/// \throw PhysfsFileException when Physfs reports an error
		void write(const char* data, std::size_t length);
};
