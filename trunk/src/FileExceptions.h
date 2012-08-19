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

#include <exception>

/*! \class FileSystemException
	\brief common base class of all file system related errors
*/
class FileSystemException : public std::exception
{
	public:
		FileSystemException() { };
		~FileSystemException() throw() { };
		
		virtual const char* what() const throw()
		{
			return "a file system related exception occured!";
		}
};

/*! \class PhysfsException
	\brief signales errors reported by physfs
	\details base class for all exceptions that report physfs errors;
*/
class PhysfsException : public FileSystemException
{
	public:
		// implementation in FileSystem.cpp
		PhysfsException();
		
		~PhysfsException() throw() { };
		
		virtual const char* what() const throw()
		{
			return ("physfs reported an error: " + getPhysfsMessage()).c_str();
		}
		
		std::string getPhysfsMessage() const
		{
			return mPhysfsErrorMsg;
		}
	private:
	
		/// this string saves the error message
		std::string mPhysfsErrorMsg;
};

/*! \class PhysfsInitException
	\brief signals an error during the physfs initialisation phase
*/
class PhysfsInitException : public PhysfsException
{
	public:
		PhysfsInitException(const std::string& path) : mPath(path)
		{
			
		}
		
		~PhysfsInitException() throw()
		{
			
		}
		
		virtual const char* what() const throw()
		{
			return ("could not initialise physfs to path " + getPath() + ": " + getPhysfsMessage()).c_str();
		}
		
		std::string getPath() const
		{
			return mPath;
		}
		
	private:
		std::string mPath;
};

/*!	\class FileException
	\brief common base type for file exceptions.
	\details does not owerride what(), so there is
				no default error message for FileException s
*/
class FileException: public FileSystemException {
	public:
		FileException(const std::string& f) : filename(f) 
		{
		}
		
		virtual ~FileException() throw() {	}
		
		/// get the name of the file of the exception
		const std::string& getFileName() const 
		{
			return filename;
		} 
		
	private:
		std::string filename;	///!< name of the file which caused the exception
};

/*! \class FileLoadException
	\brief error thrown when a file could not be opened or created.
	\todo use a better name as FileLoadException does only fit for 
			the open for reading case.
*/
class FileLoadException : public FileException
{
	public:
		FileLoadException(std::string name) : FileException(name)
		{
			/// \todo do we really need to do this? std::exception already
			/// provides the functionality for setting exception messages, i think.
			error = "Couldn't load " + name;
		}
		
		virtual ~FileLoadException() throw() {}

		virtual const char* what() const throw()
		{
			return error.c_str();
		}
		
	private:
		std::string error;	///< saves the error message
};

/*! \class FileAlreadyExistsException
	\brief error thrown when trying to create a new file even though this file exists already.
*/
class FileAlreadyExistsException : public FileException
{
	public:
		FileAlreadyExistsException(std::string name) : FileException(name)
		{
		}
		
		virtual ~FileAlreadyExistsException() throw() { }

		virtual const char* what() const throw()
		{
			return ("File " + getFileName() + " already exists.").c_str();
		}
};



/*! \class PhysfsFileException
	\brief combines FileException with PhysfsException
*/
class PhysfsFileException : public FileException, public PhysfsException
{
	public:
		PhysfsFileException(const std::string& filename) : FileException(filename)
		{
		};
		
		~PhysfsFileException() throw() { };
		
		virtual const char* what() const throw()
		{
			return (getFileName() + ": " + getPhysfsMessage()).c_str();
		}
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
		NoFileOpenedException() : FileException("") { };
		
		~NoFileOpenedException() throw() { };
		
		virtual const char* what() const throw()
		{
			// default error message for now
			return "trying to perform a file operation when no file was opened.";
		}
};

/*!	\class EOFException
	\brief thrown when trying to read over eof
	\details Exceptions of this type are issued when a reading operation tries to read
			behind a files eof.
*/
class EOFException : public FileException
{
	public:
		EOFException(const std::string& file) : FileException( file ) { };
		
		~EOFException() throw() { };
		
		virtual const char* what() const throw()
		{
			// default error message for now
			return (getFileName() + " trying to read after eof.").c_str();
		}
};
