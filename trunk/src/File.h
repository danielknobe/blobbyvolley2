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

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/shared_array.hpp>

/**
	\class File
	\brief convenience wrapper around PHYSFS_file
	\details This class provides object oriented access to physfs files. Furthermore,
			it automatically closes opened files on destruction to prevent leaks. 
			For now, it is defined as noncopyable, because we don't have implemented
			any copying behaviour yet.
	\exception PhysfsException When any physfs function call reported an error, this
							exception is thrown. Its what() string contains the error
							message physfs created.
	\todo 	write tests for this class.
*/
class File : boost::noncopyable 
{
	public:
		enum OpenMode {
			OPEN_READ,
			OPEN_WRITE
		};
	
		/// \brief default ctor
		/// \details File has to be opended with open()
		/// \throw nothing
		explicit File();
		
		/// \brief constructor which opens a file.
		/// \param filename File to be opened
		/// \param mode Should this file be opened in reading or in writing mode.
		/// \throw FileLoadException, if the file could not be loaded
		File(const std::string& filename, OpenMode mode);
		
		/// \brief opens a file.
		/// \param filename File to be opened
		/// \param mode Should this file be opened in reading or in writing mode.
		/// \throw FileLoadException, if the file could not be loaded
		/// \pre No file is currently opened.
		void open(const std::string& filename, OpenMode mode);
		
		/// \brief closes the file
		/// \details This function does nothing when the no file is opened.
		///			This behaviour is necessary as this function is called in the destructor.
		/// \throw nothing
		void close();
		
		/// destructor, closes the file (if any open)
		/// \sa close()
		/// \throw nothing
		~File();
		
		/// \brief gets the PHYSFS_file as void*
		/// \details We don't return a PHYSFS_file* here, as this type is a typedef so it cannot
		///			 be forward declared, which would require us to include <physfs.h> here. 
		/// \throw nothing
		/// \depreciated don't use this method if it is not absolutely neccessary. If there is 
		/// 			physfs functionality that cannot be accessed without the use of this method,
		///				consider to add these functions together with proper tests and checks to this
		///				class instead of using this function. 
		///				You bypass all the security that this File class offers by using the direct
		///				Physfs_file.
		void* getPHYSFS_file();
		
		// ------------------------------------
		// information querying interface
		// ------------------------------------
		
		/// \brief checks if a file is opened
		/// \throw nothing
		bool is_open() const;
		
		/// \todo check if these return types fit!
		
		/// gets the length of the currently opened file.
		/// \throw PhysfsException when Physfs reports an error
		/// \throw NoFileOpenedException when called while no file is opened.
		uint32_t length() const;
		
		/// gets the current reading position.
		/// \throw PhysfsException when Physfs reports an error.
		/// \throw NoFileOpenedException when called while no file is opened.
		uint32_t tell() const;
		
		// ------------------------------------
		//  reading interface
		// ------------------------------------
		/// reads bytes into a buffer
		/// \param target buffer to read into
		/// \param num_of_bytes number of bytes to read
		/// \throw PhysfsException when nothing could be read
		/// \throw NoFileOpenedException when called while no file is opened.
		uint32_t readRawBytes( char* target, std::size_t num_of_bytes );
		
		
		/// reads bytes and returns a safe-pointed buffer
		/// the buffer is allocated by this function and has a size of \p num_of_bytes
		/// \param num_of_bytes Number of bytes to read; size of buffer
		/// \throw PhysfsException when nothing could be read
		/// \throw NoFileOpenedException when called while no file is opened.
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
		
		/// moves the read/write cursor to the desired position
		/// \throw PhysfsException when Physfs reports an error
		/// \throw NoFileOpenedException when called while no file is opened.
		void seek(uint32_t target);
		
		// ------------------------------------
		//  writing interface
		// ------------------------------------
		
		/// \brief writes one character
		/// \details writes exactly the one character supplied.
		/// \throw PhysfsException when Physfs reports an error.
		/// \throw NoFileOpenedException when called while no file is opened.
		void writeByte(char c);
		
		/// \brief writes 32 bit integer
		/// \details writes an integer, converted to little endian if necessary
		/// \throw PhysfsException when Physfs reports an error.
		/// \throw NoFileOpenedException when called while no file is opened.
		void writeUInt32(uint32_t v);
		
		/// \brief writes a std::string
		/// \details writes the content of the string to the file.
		/// 		does not write a null-termination character
		/// \throw PhysfsException when Physfs reports an error
		/// \throw NoFileOpenedException when called while no file is opened.
		void write(const std::string& data);
		
		/// \brief writes null-terminated std::string
		/// \details writes the content of the string to the file.
		/// 		ends the string with a null terminator.
		/// \throw NoFileOpenedException when called while no file is opened.
		void writeNullTerminated(const std::string& data);
		
		/// \brief writes a sequence of characters
		/// \details writes \p length characters from \p data to the file
		/// \throw PhysfsException when Physfs reports an error
		void write(const char* data, std::size_t length);
		
	private:
		/// we use void* instead of PHYSFS_file here, because we can't forward declare it
		///	as it is a typedef.
		void* handle;
		
		/// helper function which checks if a file is opened and throws 
		/// an exception otherwise.
		/// this is called in every function that directly invokes physfs calls
		/// because physfs does not check that istelf (just crashes)
		/// doing an assert would be acceptable from a coders point of view
		/// as trying to read/write/etc. from a closed file is clearly a
		/// programming error. But we don't do anything performance critical with
		/// files, and even then woudl hard-disk speed be the limiting factor, not
		/// not the few additional cycles needed for this exception handling.
		/// so, i think. doing an exception is the best choice here, as it allows
		/// better error recorvery and error reporting.
		
		void check_file_open() const;
};

/*!	\class FileException
	\brief common base type for file exceptions
*/
class FileException: public std::exception {
};


/*! class PhysfsException
	\brief signales errors reported by physfs
	\details Exceptions of this type are thrown when calls of physfs functions
			report errors. what() gets the error string reportet by PHYSFS_getLastError()
*/
class PhysfsException : public FileException
{
	public:
		PhysfsException();
		
		~PhysfsException() throw() { };
		
		virtual const char* what() const throw()
		{
			return physfsErrorMsg.c_str();
		}
	private:
	
		/// this string saves the error message
		std::string physfsErrorMsg;
};

/*!	\class NoFileOpenedException
	\brief signals operations on closed files
	\details Exceptions of this type are thrown when any file modifying or information querying
				functions are called without a file beeing opened. These are serious errors
				and generally, exceptions of this type should not occur, as it indicates logical 
				errors in the code. Still, this allows us to handle this situation without having
				to crash or exit.
	\sa File::check_file_open()
*/
class NoFileOpenedException : public FileException
{
	public:
		NoFileOpenedException()  { };
		
		~NoFileOpenedException() throw() { };
		
		virtual const char* what() const throw()
		{
			// default error message for now
			return "trying to perform a file operation when no file was opened.";
		}
};
