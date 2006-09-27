/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @ingroup RAKNET_AUTOPATCHER
 * @brief Autopatcher File Description. 
 *  
 * This file is part of RakNet Copyright 2003, 2004
 * Rakkarsoft LLC and Kevin Jen kins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the
 * shareware license found at
 * http://www.rakkarsoft.com/shareWareLicense.html which you agreed to
 * upon purchase of a "Shareware license" "Commercial" Licensees with
 * Rakkarsoft LLC are subject to the commercial license found at
 * http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed
 * to upon purchase of a "Commercial license" All other users are
 * subject to the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Refer to the appropriate license agreement for distribution,
 * modification, an d warranty rights.
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
