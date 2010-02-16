/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @ingroup RAKNET_AUTOPATCHER
 * @brief Autopatcher File Description. 
 *
 * Copyright (c) 2003, 2004, Rakkarsoft LLC and Kevin Jenkins
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

#ifndef __DOWNLOADABLE_FILE_DESCRIPTOR
#define __DOWNLOADABLE_FILE_DESCRIPTOR

#include "BitStream.h"
#include "SHA1.h" 
/**
 * @ingroup RAKNET_AUTOPATCHER
 * @brief File information.
 *
 * A file descriptor contains informations used by the autopatcher to 
 * detect whether or not a file need to be updated or not. 
 * 
 */

struct DownloadableFileDescriptor
{
	/**
	 * Default Constructor
	 */
	DownloadableFileDescriptor();
	/**
	 * Destructor
	 */
	~DownloadableFileDescriptor();
	/**
	 * Clear the file descriptor
	 */
	void Clear( void );
	/**
	 * Append to a BitStream the header for this file. 
	 * @param out The BitStream object which will contains the result. 
	 */
	void SerializeHeader( RakNet::BitStream *out );
	/**
	 * Append the SHA1 hash information to the BitStream 
	 * @param out The BitStream object which will contains the result. 
	 */
	void SerializeSHA1( RakNet::BitStream *out );
	/**
	 * Append file content to the BitStream 
	 * @param out The BitStream object which will contains the result. 
	 */
	void SerializeFileData( RakNet::BitStream *out );
	/**
	 * Retrieve Header information from a BitStream 
	 * @param in the incoming data 
	 * @return True on success false otherwise 
	 */
	bool DeserializeHeader( RakNet::BitStream *in );
	/**
	 * Retrieve SHA1 hash 
	 * @param in the incoming data 
	 * @return True on success false otherwise 
	 */
	bool DeserializeSHA1( RakNet::BitStream *in );
	/**
	 * Retrieve File data 
	 * @param in the incoming data 
	 * @return True on success false otherwise 
	 */
	bool DeserializeFileData( RakNet::BitStream *in );
	
	/**
	 * The filename. It is a relative path. 
	 */
	char *filename;
	/**
	 * The size of the file. 
	 */
	unsigned fileLength;
	/**
	 * True if data is compressed using zlib 
	 */
	bool fileDataIsCompressed;
	/**
	 * The size of the compressed file 
	 */
	unsigned compressedFileLength;
	/**
	 * The SHA-1 hash key 
	 */
	char SHA1Code[ SHA1_LENGTH ];
	/**
	 * The data of the file. 
	 */
	char *fileData;
};

#endif
