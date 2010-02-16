/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Provide implementation of the CheckSum class 
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
 *
 * From http://www.flounder.com/checksum.htm
 * 
 */

#include "CheckSum.h"

/****************************************************************************
*        CheckSum::add
* Inputs:
*   unsigned int d: word to add
* Result: void
* 
* Effect: 
*   Adds the bytes of the unsigned int to the CheckSum
****************************************************************************/

void CheckSum::add ( unsigned int value )
{
	union
	{
		unsigned int value;
		unsigned char bytes[ 4 ];
	}

	data;
	data.value = value;

	for ( unsigned int i = 0; i < sizeof( data.bytes ); i++ )
		add ( data.bytes[ i ] )

		;
} // CheckSum::add(unsigned int)

/****************************************************************************
*       CheckSum::add
* Inputs:
*   unsigned short value:
* Result: void
* 
* Effect: 
*   Adds the bytes of the unsigned short value to the CheckSum
****************************************************************************/

void CheckSum::add ( unsigned short value )
{
	union
	{
		unsigned short value;
		unsigned char bytes[ 2 ];
	}

	data;
	data.value = value;

	for ( unsigned int i = 0; i < sizeof( data.bytes ); i++ )
		add ( data.bytes[ i ] )

		;
} // CheckSum::add(unsigned short)

/****************************************************************************
*       CheckSum::add
* Inputs:
*   unsigned char value:
* Result: void
* 
* Effect: 
*   Adds the byte to the CheckSum
****************************************************************************/

void CheckSum::add ( unsigned char value )
{
	unsigned char cipher = (unsigned char)( value ^ ( r >> 8 ) );
	r = ( cipher + r ) * c1 + c2;
	sum += cipher;
} // CheckSum::add(unsigned char)


/****************************************************************************
*       CheckSum::add
* Inputs:
*   LPunsigned char b: pointer to byte array
*   unsigned int length: count
* Result: void
* 
* Effect: 
*   Adds the bytes to the CheckSum
****************************************************************************/

void CheckSum::add ( unsigned char *b, unsigned int length )
{
	for ( unsigned int i = 0; i < length; i++ )
		add ( b[ i ] )

		;
} // CheckSum::add(LPunsigned char, unsigned int)
