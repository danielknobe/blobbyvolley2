/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @ingroup RAKNET_DNO 
* @file EncodeClassName.cpp 
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
#include "EncodeClassName.h"
#include "BitStream.h"
#include <memory.h>

void EncodeClassName( char *name, char *identifier )
{
	RakNet::BitStream bitStream;
	int index = 0;
	unsigned char byte;

	while ( index < MAXIMUM_CLASS_IDENTIFIER_LENGTH - 1 )
	{
		if ( name[ index ] == 0 )
			break;

		// This should generate a unique identifier for any realistic class name that is 5/8th the length of the actual name and weakly encrypts and compresses it
		if ( name[ index ] >= 'a' && name[ index ] <= 'z' )
			byte = name[ index ] - 'a';
		else
			if ( name[ index ] >= 'A' && name[ index ] <= 'Z' )
				byte = name[ index ] - 'A';
			else
				if ( name[ index ] >= '0' && name[ index ] <= '9' )
					byte = name[ index ] - '0';
				else
					byte = name[ index ] << 3;

		bitStream.WriteBits( ( unsigned char* ) & byte, 5 );

		index++;
	}

#ifdef _DEBUG
	memset( identifier, 0, MAXIMUM_CLASS_IDENTIFIER_LENGTH );

#endif

	identifier[ 0 ] = ( char ) ( bitStream.GetNumberOfBytesUsed() );

	memcpy( identifier + 1, bitStream.GetData(), bitStream.GetNumberOfBytesUsed() );
}
