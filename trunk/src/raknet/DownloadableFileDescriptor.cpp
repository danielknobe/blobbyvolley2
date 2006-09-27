/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @ingroup RAKNET_AUTOPATCHER
 * @brief Autopatcher File Description Implementation. 
 *  
 * This file is part of RakNet Copyright 2003 Rakkarsoft LLC and Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the
 * shareware license found at
 * http://www.rakkarsoft.com/shareWareLicense.html which you agreed to
 * upon purchase of a "Shareware license" "Commercial" Licensees with
 * Rakkarsoft LLC are subject to the commercial license found at
 * http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed
 * to upon purchase of a "Commercial license"
 * Custom license users are subject to the terms therein.
 * All other users are
 * subject to the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Refer to the appropriate license agreement for distribution,
 * modification, and warranty rights.
 */
#include <string.h>
#include <assert.h>

#include "DownloadableFileDescriptor.h"
#include "BitStream.h"

using namespace RakNet;

DownloadableFileDescriptor::DownloadableFileDescriptor()
{
	filename = fileData = 0;
	fileLength = compressedFileLength = 0;
}

DownloadableFileDescriptor::~DownloadableFileDescriptor()
{
	Clear();
}

void DownloadableFileDescriptor::Clear( void )
{
	if ( filename )
		delete [] filename;
		
	filename = 0;
	
	if ( fileData )
		delete [] fileData;
		
	fileData = 0;
	
	fileLength = compressedFileLength = 0;
}

void DownloadableFileDescriptor::SerializeHeader( BitStream *out )
{
	unsigned char filenameLength;
	assert( filename && filename[ 0 ] );
	filenameLength = ( unsigned char ) strlen( filename );
	out->Write( filenameLength );
	out->Write( filename, filenameLength );
	out->WriteCompressed( fileLength );
	out->Write( fileDataIsCompressed );
	
	if ( fileDataIsCompressed )
		out->WriteCompressed( compressedFileLength );
}

bool DownloadableFileDescriptor::DeserializeHeader( BitStream *in )
{
	unsigned char filenameLength;
	
	if ( in->Read( filenameLength ) == false )
		return false;
		
	assert( filename == 0 );
	
	filename = new char [ filenameLength + 1 ];
	
	if ( in->Read( filename, filenameLength ) == false )
	{
		delete [] filename;
		filename = 0;
		return false;
	}
	
	filename[ filenameLength ] = 0;
	
	if ( in->ReadCompressed( fileLength ) == false )
	{
		delete [] filename;
		filename = 0;
		return false;
	}
	
	if ( in->Read( fileDataIsCompressed ) == false )
	{
		delete [] filename;
		filename = 0;
		return false;
	}
	
	if ( fileDataIsCompressed )
	{
		if ( in->ReadCompressed( compressedFileLength ) == false )
		{
			delete [] filename;
			filename = 0;
			return false;
		}
	}
	
	return true;
}

void DownloadableFileDescriptor::SerializeFileData( BitStream *out )
{
	assert( fileData );
	
	if ( fileDataIsCompressed )
		out->Write( fileData, compressedFileLength );
	else
		out->Write( fileData, fileLength );
}

bool DownloadableFileDescriptor::DeserializeFileData( BitStream *in )
{
	assert( fileData == 0 );
	
	if ( fileDataIsCompressed )
	{
		fileData = new char [ compressedFileLength ];
		
		if ( in->Read( fileData, compressedFileLength ) == false )
		{
			delete [] fileData;
			fileData = 0;
			return false;
		}
	}
	
	else
	{
		fileData = new char [ fileLength ];
		
		if ( in->Read( fileData, fileLength ) == false )
		{
			delete [] fileData;
			fileData = 0;
			return false;
		}
	}
	
	return true;
}

void DownloadableFileDescriptor::SerializeSHA1( BitStream *out )
{
	out->Write( SHA1Code, SHA1_LENGTH * sizeof( char ) );
}

bool DownloadableFileDescriptor::DeserializeSHA1( BitStream *in )
{
	if ( in->Read( SHA1Code, SHA1_LENGTH * sizeof( char ) ) == false )
		return false;
		
	return true;
}
