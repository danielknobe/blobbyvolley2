/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_DNO 
 * @file EncodeClassName.cpp 
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
