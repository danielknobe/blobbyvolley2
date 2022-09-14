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

#include <exception>
#include <string>

#include <boost/exception/all.hpp>
#include <utility>

/*! \class FileSystemException
	\brief common base class of all file system related errors
*/
class FileSystemException : public std::exception
{
	public:
		FileSystemException() = default;
		~FileSystemException() noexcept override = default;

		const char* what() const noexcept override
		{
			return mMessage.c_str();
		}

	protected:
		std::string mMessage;
};

/*! \class PhysfsException
	\brief signals errors reported by physfs
	\details base class for all exceptions that report physfs errors;
*/
class PhysfsException : public virtual FileSystemException
{
	public:
		// implementation in FileSystem.cpp
		PhysfsException();
		explicit PhysfsException(std::string prefix);

		std::string getPhysfsMessage() const { return mPhysfsError; }
	private:
		/// this string saves the error as reported by physfs
		std::string mPhysfsError;
};

/*! \class PhysfsInitException
	\brief signals an error during the physfs initialisation phase
*/
class PhysfsInitException : public PhysfsException
{
	public:
		explicit PhysfsInitException(std::string path);
		std::string getPath() const { return mPath; }
	private:
		std::string mPath;
};

/*!	\class FileException
	\brief common base type for file exceptions.
	\details does not override what(), so there is
				no default error message for `FileException`s
*/
class FileException: public virtual FileSystemException {
	public:
		explicit FileException(std::string f) : mFileName( std::move(f) )
		{
		}

		/// get the name of the file of the exception
		const std::string& getFileName() const { return mFileName; }

	private:
		std::string mFileName;	///!< name of the file which caused the exception
};

/*! \class FileLoadException
	\brief error thrown when a file could not be opened or created.
	\todo use a better name as FileLoadException does only fit for
			the open for reading case.
*/
class FileLoadException : public FileException, public PhysfsException
{
	public:
		explicit FileLoadException(const std::string& name);
};

/*! \class FileAlreadyExistsException
	\brief error thrown when trying to create a new file even though this file exists already.
*/
class FileAlreadyExistsException : public FileException
{
	public:
		explicit FileAlreadyExistsException(std::string name);
};



/*! \class PhysfsFileException
	\brief combines FileException with PhysfsException
*/
class PhysfsFileException : public FileException, public PhysfsException
{
	public:
		explicit PhysfsFileException(const std::string& filename) ;
};

/*!	\class NoFileOpenedException
	\brief signals operations on closed files
	\details Exceptions of this type are thrown when any file modifying or information querying
				functions are called without a file being opened. These are serious errors
				and generally, exceptions of this type should not occur, as it indicates logical
				errors in the code. Still, this allows us to handle this situation without having
				to crash or exit.
	\sa File::check_file_open()
*/
class NoFileOpenedException : public FileException
{
	public:
		NoFileOpenedException();
};

/*!	\class EOFException
	\brief thrown when trying to read over eof
	\details Exceptions of this type are issued when a reading operation tries to read
			behind a files eof.
*/
class EOFException : public FileException
{
	public:
		explicit EOFException(std::string file);
};
