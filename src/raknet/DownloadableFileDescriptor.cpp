/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @ingroup RAKNET_AUTOPATCHER
 * @brief Autopatcher File Description Implementation. 
 *
 * Copyright (c) 2003, Rakkarsoft LLC and Kevin Jenkins
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
