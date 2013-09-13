/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 *
 * @brief Implementation of BitStream class
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
/*
 * Uncomment this to check that read and writes match with the same
 * type and in the case of streams, size.  Useful during debugging
 */
//#define TYPE_CHECKING

#ifdef TYPE_CHECKING
#ifndef _DEBUG
#ifdef _WIN32
#pragma message("Warning: TYPE_CHECKING is defined in BitStream.cpp when not in _DEBUG mode" )
#endif
#endif
#endif

// This should be on by default for speed.  Turn it off if you actually need endian swapping
#define __BITSTREAM_NATIVE_END

#include "BitStream.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/*if defined ( __APPLE__ ) || defined ( __APPLE_CC__ ) || defined ( __MACOSX__ )
	#include <malloc.h>
  #endif*/

#ifdef __BITSTREAM_BIG_END
// Set up the read/write routines to produce Big-End network streams.
#define B16_1 0
#define B16_0 1

#define B32_3 0
#define B32_2 1
#define B32_1 2
#define B32_0 3

#define B64_7 0
#define B64_6 1
#define B64_5 2
#define B64_4 3
#define B64_3 4
#define B64_2 5
#define B64_1 6
#define B64_0 7

#else
// Default to producing Little-End network streams.
#define B16_1 1
#define B16_0 0

#define B32_3 3
#define B32_2 2
#define B32_1 1
#define B32_0 0

#define B64_7 7
#define B64_6 6
#define B64_5 5
#define B64_4 4
#define B64_3 3
#define B64_2 2
#define B64_1 1
#define B64_0 0
#endif

using namespace RakNet;

BitStream::BitStream()
{
	numberOfBitsUsed = 0;
	numberOfBitsAllocated = BITSTREAM_STACK_ALLOCATION_SIZE * 8;
	readOffset = 0;
	data = stackData;
	copyData = true;
}

BitStream::BitStream( int initialBytesToAllocate )
{
	numberOfBitsUsed = 0;
	readOffset = 0;
	if (initialBytesToAllocate <= BITSTREAM_STACK_ALLOCATION_SIZE)
	{
		data = ( unsigned char* ) stackData;
		numberOfBitsAllocated = BITSTREAM_STACK_ALLOCATION_SIZE * 8;
	}
	else
	{
		data = ( unsigned char* ) malloc( initialBytesToAllocate );
		numberOfBitsAllocated = initialBytesToAllocate << 3;
	}
#ifdef _DEBUG
	assert( data );
#endif
	// memset(data, 0, initialBytesToAllocate);
	copyData = true;
}

BitStream::BitStream( char* _data, unsigned int lengthInBytes, bool _copyData )
{
	numberOfBitsUsed = lengthInBytes << 3;
	readOffset = 0;
	copyData = _copyData;
	numberOfBitsAllocated = lengthInBytes << 3;

	if ( copyData )
	{
		if ( lengthInBytes > 0 )
		{
			if (lengthInBytes < BITSTREAM_STACK_ALLOCATION_SIZE)
			{
				data = ( unsigned char* ) stackData;
				numberOfBitsAllocated = BITSTREAM_STACK_ALLOCATION_SIZE << 3;
			}
			else
			{
				data = ( unsigned char* ) malloc( lengthInBytes );
			}
#ifdef _DEBUG
			assert( data );
#endif
			memcpy( data, _data, lengthInBytes );
		}
		else
			data = 0;
	}
	else
		data = ( unsigned char* ) _data;
}

// Use this if you pass a pointer copy to the constructor (_copyData==false) and want to overallocate to prevent reallocation
void BitStream::SetNumberOfBitsAllocated( const unsigned int lengthInBits )
{
#ifdef _DEBUG
	assert( lengthInBits >= ( unsigned int ) numberOfBitsAllocated );
#endif
	numberOfBitsAllocated = lengthInBits;
}

BitStream::~BitStream()
{
	if ( copyData && numberOfBitsAllocated > BITSTREAM_STACK_ALLOCATION_SIZE << 3)
		free( data );  // Use realloc and free so we are more efficient than delete and new for resizing
}

void BitStream::Reset( void )
{
	// Note:  Do NOT reallocate memory because BitStream is used
	// in places to serialize/deserialize a buffer. Reallocation
	// is a dangerous operation (may result in leaks).

	if ( numberOfBitsUsed > 0 )
	{
		//  memset(data, 0, BITS_TO_BYTES(numberOfBitsUsed));
	}

	// Don't free memory here for speed efficiency
	//free(data);  // Use realloc and free so we are more efficient than delete and new for resizing
	numberOfBitsUsed = 0;

	//numberOfBitsAllocated=8;
	readOffset = 0;

	//data=(unsigned char*)malloc(1);
	// if (numberOfBitsAllocated>0)
	//  memset(data, 0, BITS_TO_BYTES(numberOfBitsAllocated));
}

// Write the native types to the end of the buffer
void BitStream::Write( const bool input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 0;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

	if ( input )
		Write1();
	else
		Write0();
}

void BitStream::Write( const unsigned char input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 1;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

	WriteBits( ( unsigned char* ) & input, sizeof( input ) * 8, true );
}

void BitStream::Write( const char input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 2;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

	WriteBits( ( unsigned char* ) & input, sizeof( input ) * 8, true );
}

void BitStream::Write( const unsigned short input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 3;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

#ifdef __BITSTREAM_NATIVE_END
	WriteBits( ( unsigned char* ) & input, sizeof( input ) * 8, true );
#else
	static unsigned char uint16w[2];
	uint16w[B16_1] =  (input >> 8)&(0xff);
	uint16w[B16_0] = input&(0xff);

	WriteBits( uint16w, sizeof( input ) * 8, true );
#endif
}

void BitStream::Write( const short input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 4;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

#ifdef __BITSTREAM_NATIVE_END
	WriteBits( ( unsigned char* ) & input, sizeof( input ) * 8, true );
#else
	static unsigned char int16w[2];
	int16w[B16_1] =  (input >> 8)&(0xff);
	int16w[B16_0] = input&(0xff);

	WriteBits( int16w, sizeof( input ) * 8, true );
#endif
}

void BitStream::Write( const unsigned int input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 5;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

#ifdef __BITSTREAM_NATIVE_END
	WriteBits( ( unsigned char* ) & input, sizeof( input ) * 8, true );
#else
	static unsigned char uint32w[4];
	uint32w[B32_3] = (input >> 24)&(0x000000ff);
	uint32w[B32_2] = (input >> 16)&(0x000000ff);
	uint32w[B32_1] = (input >> 8)&(0x000000ff);
	uint32w[B32_0] = (input)&(0x000000ff);

	WriteBits( uint32w, sizeof( input ) * 8, true );
#endif
}

void BitStream::Write( const int input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 6;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

#ifdef __BITSTREAM_NATIVE_END
	WriteBits( ( unsigned char* ) & input, sizeof( input ) * 8, true );
#else
	static unsigned char int32w[4];
	int32w[B32_3] = (input >> 24)&(0x000000ff);
	int32w[B32_2] = (input >> 16)&(0x000000ff);
	int32w[B32_1] = (input >> 8)&(0x000000ff);
	int32w[B32_0] = (input)&(0x000000ff);

	WriteBits( int32w, sizeof( input ) * 8, true );
#endif
}

void BitStream::Write( const float input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 9;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

#ifndef __BITSTREAM_NATIVE_END
	unsigned int intval = *((unsigned int *)(&input));
	Write(intval);
#else
	WriteBits( ( unsigned char* ) & input, sizeof( input ) * 8, true );
#endif
}

void BitStream::Write( const double input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 10;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

	WriteBits( ( unsigned char* ) & input, sizeof( input ) * 8, true );
}

// Write an array or casted stream
void BitStream::Write( const char* input, const int numberOfBytes )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 11;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
	WriteBits( ( unsigned char* ) & numberOfBytes, sizeof( int ) * 8, true );
#endif

	WriteBits( ( unsigned char* ) input, numberOfBytes * 8, true );
}

// Write the native types with simple compression.
// Best used with  negatives and positives close to 0
void BitStream::WriteCompressed( const unsigned char input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 12;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

	WriteCompressed( ( unsigned char* ) & input, sizeof( input ) * 8, true );
}

void BitStream::WriteCompressed( const char input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 13;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

	WriteCompressed( ( unsigned char* ) & input, sizeof( input ) * 8, false );
}

void BitStream::WriteCompressed( const unsigned short input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 14;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

#ifdef __BITSTREAM_NATIVE_END
	WriteCompressed( ( unsigned char* ) & input, sizeof( input ) * 8, true );
#else
	static unsigned char uint16wc[2];
	uint16wc[B16_1] =  (input >> 8)&(0xff);
	uint16wc[B16_0] = input&(0xff);

	WriteCompressed( uint16wc, sizeof( input ) * 8, true );
#endif
}

void BitStream::WriteCompressed( const short input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 15;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

#ifdef __BITSTREAM_NATIVE_END
	WriteCompressed( ( unsigned char* ) & input, sizeof( input ) * 8, true );
#else
	static unsigned char int16wc[2];
	int16wc[B16_1] =  (input >> 8)&(0xff);
	int16wc[B16_0] = input&(0xff);

	WriteCompressed( int16wc, sizeof( input ) * 8, false );
#endif
}

void BitStream::WriteCompressed( const unsigned int input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 16;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

#ifdef __BITSTREAM_NATIVE_END
	WriteCompressed( ( unsigned char* ) & input, sizeof( input ) * 8, true );
#else
	static unsigned char uint32wc[4];
	uint32wc[B32_3] = (input >> 24)&(0x000000ff);
	uint32wc[B32_2] = (input >> 16)&(0x000000ff);
	uint32wc[B32_1] = (input >> 8)&(0x000000ff);
	uint32wc[B32_0] = (input)&(0x000000ff);

	WriteCompressed( uint32wc, sizeof( input ) * 8, true );
#endif
}

void BitStream::WriteCompressed( const int input )
{
#ifdef TYPE_CHECKING
	unsigned char ID = 17;
	WriteBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8, true );
#endif

#ifdef __BITSTREAM_NATIVE_END
	WriteCompressed( ( unsigned char* ) & input, sizeof( input ) * 8, true );
#else
	static unsigned char int32wc[4];
	int32wc[B32_3] = (input >> 24)&(0x000000ff);
	int32wc[B32_2] = (input >> 16)&(0x000000ff);
	int32wc[B32_1] = (input >> 8)&(0x000000ff);
	int32wc[B32_0] = (input)&(0x000000ff);

	WriteCompressed( int32wc, sizeof( input ) * 8, false );
#endif
}

// Read the native types from the front of the buffer
// Write the native types to the end of the buffer
bool BitStream::Read( bool& output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

#ifdef _DEBUG

	assert( ID == 0 );

#endif
#endif

	//assert(readOffset+1 <=numberOfBitsUsed); // If this assert is hit the stream wasn't long enough to read from
	if ( readOffset + 1 > numberOfBitsUsed )
		return false;

	//if (ReadBit()) // Check that bit
	if ( data[ readOffset >> 3 ] & ( 0x80 >> ( readOffset % 8 ) ) )   // Is it faster to just write it out here?
		output = true;
	else
		output = false;

	readOffset++;
	return true;
}

bool BitStream::Read( unsigned char &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 1 );

#endif

	return ReadBits( ( unsigned char* ) & output, sizeof( output ) * 8 );
}

bool BitStream::Read( char &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 2 );

#endif

	return ReadBits( ( unsigned char* ) & output, sizeof( output ) * 8 );
}

bool BitStream::Read( unsigned short &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 3 );

#endif

#ifdef __BITSTREAM_NATIVE_END
	return ReadBits( ( unsigned char* ) & output, sizeof( output ) * 8 );
#else
	static unsigned char uint16r[2];
	if (ReadBits( uint16r, sizeof( output ) * 8 ) != true) return false;
	output = (((unsigned short) uint16r[B16_1])<<8)|((unsigned short)uint16r[B16_0]);
	return true;
#endif
}

bool BitStream::Read( short &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 4 );

#endif

#ifdef __BITSTREAM_NATIVE_END
	return ReadBits( ( unsigned char* ) & output, sizeof( output ) * 8 );
#else
	static unsigned char int16r[2];
	if (ReadBits( int16r, sizeof( output ) * 8 ) != true) return false;
	output = (((unsigned short) int16r[B16_1])<<8)|((unsigned short)int16r[B16_0]);
	return true;
#endif
}

bool BitStream::Read( unsigned int &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 5 );

#endif

#ifdef __BITSTREAM_NATIVE_END
	return ReadBits( ( unsigned char* ) & output, sizeof( output ) * 8 );
#else
	static unsigned char uint32r[4];
	if(ReadBits( uint32r, sizeof( output ) * 8 ) != true)
		return false;
	output = (((unsigned int) uint32r[B32_3])<<24)|
		(((unsigned int) uint32r[B32_2])<<16)|
		(((unsigned int) uint32r[B32_1])<<8)|
		((unsigned int) uint32r[B32_0]);
	return true;
#endif
}

bool BitStream::Read( int &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 6 );

#endif

#ifdef __BITSTREAM_NATIVE_END
	return ReadBits( ( unsigned char* ) & output, sizeof( output ) * 8 );
#else
	static unsigned char int32r[4];
	if(ReadBits( int32r, sizeof( output ) * 8 ) != true)
		return false;
	output = (((unsigned int) int32r[B32_3])<<24)|
		(((unsigned int) int32r[B32_2])<<16)|
		(((unsigned int) int32r[B32_1])<<8)|
		((unsigned int) int32r[B32_0]);
	return true;
#endif
}

bool BitStream::Read( float &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 9 );

#endif

#ifdef __BITSTREAM_NATIVE_END
	return ReadBits( ( unsigned char* ) & output, sizeof( output ) * 8 );
#else
	unsigned int val;
	if (Read(val) == false) return false;
	output = *((float *)(&val));
	return true;
#endif
}

bool BitStream::Read( double &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 10 );

#endif

	return ReadBits( ( unsigned char* ) & output, sizeof( output ) * 8 );

}

// Read an array or casted stream
bool BitStream::Read( char* output, const int numberOfBytes )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 11 );

	int NOB;

	ReadBits( ( unsigned char* ) & NOB, sizeof( int ) * 8 );

	assert( NOB == numberOfBytes );

#endif

	return ReadBits( ( unsigned char* ) output, numberOfBytes * 8 );
}

// Read the types you wrote with WriteCompressed
bool BitStream::ReadCompressed( unsigned char & output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 12 );

#endif

	return ReadCompressed( ( unsigned char* ) & output, sizeof( output ) * 8, true );
}

bool BitStream::ReadCompressed( char &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 13 );

#endif

	return ReadCompressed( ( unsigned char* ) & output, sizeof( output ) * 8, false );
}

bool BitStream::ReadCompressed( unsigned short &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 14 );

#endif

#ifdef __BITSTREAM_NATIVE_END
	return ReadCompressed( ( unsigned char* ) & output, sizeof( output ) * 8, true );
#else
	static unsigned char uint16rc[2];
	if (ReadCompressed( uint16rc, sizeof( output ) * 8, true ) != true) return false;
	output = (((unsigned short) uint16rc[B16_1])<<8)|
		((unsigned short)uint16rc[B16_0]);
	return true;
#endif
}

bool BitStream::ReadCompressed( short &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 15 );

#endif

#ifdef __BITSTREAM_NATIVE_END
	return ReadCompressed( ( unsigned char* ) & output, sizeof( output ) * 8, true );
#else
	static unsigned char int16rc[2];
	if (ReadCompressed( int16rc, sizeof( output ) * 8, false ) != true) return false;
	output = (((unsigned short) int16rc[B16_1])<<8)|((unsigned short)int16rc[B16_0]);
	return true;
#endif
}

bool BitStream::ReadCompressed( unsigned int &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 16 );

#endif

#ifdef __BITSTREAM_NATIVE_END
	return ReadCompressed( ( unsigned char* ) & output, sizeof( output ) * 8, true );
#else
	static unsigned char uint32rc[4];
	if(ReadCompressed( uint32rc, sizeof( output ) * 8, true ) != true)
		return false;
	output = (((unsigned int) uint32rc[B32_3])<<24)|
		(((unsigned int) uint32rc[B32_2])<<16)|
		(((unsigned int) uint32rc[B32_1])<<8)|
		((unsigned int) uint32rc[B32_0]);
	return true;
#endif
}

bool BitStream::ReadCompressed( int &output )
{
#ifdef TYPE_CHECKING
	unsigned char ID;

	if ( ReadBits( ( unsigned char* ) & ID, sizeof(unsigned char) * 8 ) == false )
		return false;

	assert( ID == 17 );

#endif

#ifdef __BITSTREAM_NATIVE_END
	return ReadCompressed( ( unsigned char* ) & output, sizeof( output ) * 8, true );
#else
	static unsigned char int32rc[4];
	if(ReadCompressed( int32rc, sizeof( output ) * 8, false ) != true)
		return false;
	output = (((unsigned int) int32rc[B32_3])<<24)|
		(((unsigned int) int32rc[B32_2])<<16)|
		(((unsigned int) int32rc[B32_1])<<8)|
		((unsigned int) int32rc[B32_0]);
	return true;
#endif
}

// Sets the read pointer back to the beginning of your data.
void BitStream::ResetReadPointer( void )
{
	readOffset = 0;
}

// Sets the write pointer back to the beginning of your data.
void BitStream::ResetWritePointer( void )
{
	numberOfBitsUsed = 0;
}

// Write a 0
void BitStream::Write0( void )
{
	AddBitsAndReallocate( 1 );

	// New bytes need to be zeroed

	if ( ( numberOfBitsUsed % 8 ) == 0 )
		data[ numberOfBitsUsed >> 3 ] = 0;

	numberOfBitsUsed++; // This ++ was in the line above - but boundschecker didn't like that for some reason.
}

// Write a 1
void BitStream::Write1( void )
{
	AddBitsAndReallocate( 1 );

	int numberOfBitsMod8 = numberOfBitsUsed % 8;

	if ( numberOfBitsMod8 == 0 )
		data[ numberOfBitsUsed >> 3 ] = 0x80;
	else
		data[ numberOfBitsUsed >> 3 ] |= 0x80 >> ( numberOfBitsMod8 ); // Set the bit to 1

	numberOfBitsUsed++; // This ++ was in the line above - but boundschecker didn't like that for some reason.
}

// Returns true if the next data read is a 1, false if it is a 0
bool BitStream::ReadBit( void )
{
#pragma warning( disable : 4800 )
	readOffset++;
	return ( bool ) ( data[ readOffset-1 >> 3 ] & ( 0x80 >> ( (readOffset-1) % 8 ) ) );
#pragma warning( default : 4800 )
}

// Align the bitstream to the byte boundary and then write the specified number of bits.
// This is faster than WriteBits but wastes the bits to do the alignment and requires you to call
// SetReadToByteAlignment at the corresponding read position
void BitStream::WriteAlignedBytes( const unsigned char* input,
	const int numberOfBytesToWrite )
{
#ifdef _DEBUG
	assert( numberOfBytesToWrite > 0 );
#endif

	AlignWriteToByteBoundary();
	// Allocate enough memory to hold everything
	AddBitsAndReallocate( numberOfBytesToWrite << 3 );

	// Write the data
	memcpy( data + ( numberOfBitsUsed >> 3 ), input, numberOfBytesToWrite );

	numberOfBitsUsed += numberOfBytesToWrite << 3;
}

// Read bits, starting at the next aligned bits. Note that the modulus 8 starting offset of the
// sequence must be the same as was used with WriteBits. This will be a problem with packet coalescence
// unless you byte align the coalesced packets.
bool BitStream::ReadAlignedBytes( unsigned char* output,
	const int numberOfBytesToRead )
{
#ifdef _DEBUG
	assert( numberOfBytesToRead > 0 );
#endif

	if ( numberOfBytesToRead <= 0 )
		return false;

	// Byte align
	AlignReadToByteBoundary();

	if ( readOffset + ( numberOfBytesToRead << 3 ) > numberOfBitsUsed )
		return false;

	// Write the data
	memcpy( output, data + ( readOffset >> 3 ), numberOfBytesToRead );

	readOffset += numberOfBytesToRead << 3;

	return true;
}

// Align the next write and/or read to a byte boundary.  This can be used to 'waste' bits to byte align for efficiency reasons
void BitStream::AlignWriteToByteBoundary( void )
{
	if ( numberOfBitsUsed )
		numberOfBitsUsed += 8 - ( ( numberOfBitsUsed - 1 ) % 8 + 1 );
}

// Align the next write and/or read to a byte boundary.  This can be used to 'waste' bits to byte align for efficiency reasons
void BitStream::AlignReadToByteBoundary( void )
{
	if ( readOffset )
		readOffset += 8 - ( ( readOffset - 1 ) % 8 + 1 );
}

// Write numberToWrite bits from the input source
void BitStream::WriteBits( const unsigned char *input,
	int numberOfBitsToWrite, const bool rightAlignedBits )
{
	// if (numberOfBitsToWrite<=0)
	//  return;

	AddBitsAndReallocate( numberOfBitsToWrite );
	int offset = 0;
	unsigned char dataByte;
	int numberOfBitsUsedMod8;

	numberOfBitsUsedMod8 = numberOfBitsUsed % 8;

	// Faster to put the while at the top surprisingly enough
	while ( numberOfBitsToWrite > 0 )
		//do
	{
		dataByte = *( input + offset );

		if ( numberOfBitsToWrite < 8 && rightAlignedBits )   // rightAlignedBits means in the case of a partial byte, the bits are aligned from the right (bit 0) rather than the left (as in the normal internal representation)
			dataByte <<= 8 - numberOfBitsToWrite;  // shift left to get the bits on the left, as in our internal representation

		// Writing to a new byte each time
		if ( numberOfBitsUsedMod8 == 0 )
			* ( data + ( numberOfBitsUsed >> 3 ) ) = dataByte;
		else
		{
			// Copy over the new data.
			*( data + ( numberOfBitsUsed >> 3 ) ) |= dataByte >> ( numberOfBitsUsedMod8 ); // First half

			if ( 8 - ( numberOfBitsUsedMod8 ) < 8 && 8 - ( numberOfBitsUsedMod8 ) < numberOfBitsToWrite )   // If we didn't write it all out in the first half (8 - (numberOfBitsUsed%8) is the number we wrote in the first half)
			{
				*( data + ( numberOfBitsUsed >> 3 ) + 1 ) = (unsigned char) ( dataByte << ( 8 - ( numberOfBitsUsedMod8 ) ) ); // Second half (overlaps byte boundary)
			}
		}

		if ( numberOfBitsToWrite >= 8 )
			numberOfBitsUsed += 8;
		else
			numberOfBitsUsed += numberOfBitsToWrite;

		numberOfBitsToWrite -= 8;

		offset++;
	}
	// } while(numberOfBitsToWrite>0);
}

// Set the stream to some initial data.  For internal use
void BitStream::SetData( const unsigned char* input, const int numberOfBits )
{
#ifdef _DEBUG
	assert( numberOfBitsUsed == 0 ); // Make sure the stream is clear
#endif

	if ( numberOfBits <= 0 )
		return ;

	AddBitsAndReallocate( numberOfBits );

	memcpy( data, input, BITS_TO_BYTES( numberOfBits ) );

	numberOfBitsUsed = numberOfBits;
}

// Assume the input source points to a native type, compress and write it
void BitStream::WriteCompressed( const unsigned char* input,
	const int size, const bool unsignedData )
{
	int currentByte = ( size >> 3 ) - 1; // PCs

	unsigned char byteMatch;

	if ( unsignedData )
	{
		byteMatch = 0;
	}

	else
	{
		byteMatch = 0xFF;
	}

	// Write upper bytes with a single 1
	// From high byte to low byte, if high byte is a byteMatch then write a 1 bit. Otherwise write a 0 bit and then write the remaining bytes
	while ( currentByte > 0 )
	{
		if ( input[ currentByte ] == byteMatch )   // If high byte is byteMatch (0 of 0xff) then it would have the same value shifted
		{
			bool b = true;
			Write( b );
		}
		else
		{
			// Write the remainder of the data after writing 0
			bool b = false;
			Write( b );

			WriteBits( input, ( currentByte + 1 ) << 3, true );
			//  currentByte--;


			return ;
		}

		currentByte--;
	}

	// If the upper half of the last byte is a 0 (positive) or 16 (negative) then write a 1 and the remaining 4 bits.  Otherwise write a 0 and the 8 bites.
	if ( ( unsignedData && ( ( *( input + currentByte ) ) & 0xF0 ) == 0x00 ) ||
		( unsignedData == false && ( ( *( input + currentByte ) ) & 0xF0 ) == 0xF0 ) )
	{
		bool b = true;
		Write( b );
		WriteBits( input + currentByte, 4, true );
	}

	else
	{
		bool b = false;
		Write( b );
		WriteBits( input + currentByte, 8, true );
	}
}

// Read numberOfBitsToRead bits to the output source
// alignBitsToRight should be set to true to convert internal bitstream data to userdata
// It should be false if you used WriteBits with rightAlignedBits false
bool BitStream::ReadBits( unsigned char* output,
	int numberOfBitsToRead, const bool alignBitsToRight )
{
#ifdef _DEBUG
	assert( numberOfBitsToRead > 0 );
#endif
	// if (numberOfBitsToRead<=0)
	//  return false;

	if ( readOffset + numberOfBitsToRead > numberOfBitsUsed )
		return false;

	int readOffsetMod8;

	int offset = 0;

	memset( output, 0, BITS_TO_BYTES( numberOfBitsToRead ) );

	readOffsetMod8 = readOffset % 8;

	// do
	// Faster to put the while at the top surprisingly enough
	while ( numberOfBitsToRead > 0 )
	{
		*( output + offset ) |= *( data + ( readOffset >> 3 ) ) << ( readOffsetMod8 ); // First half

		if ( readOffsetMod8 > 0 && numberOfBitsToRead > 8 - ( readOffsetMod8 ) )   // If we have a second half, we didn't read enough bytes in the first half
			*( output + offset ) |= *( data + ( readOffset >> 3 ) + 1 ) >> ( 8 - ( readOffsetMod8 ) ); // Second half (overlaps byte boundary)

		numberOfBitsToRead -= 8;

		if ( numberOfBitsToRead < 0 )   // Reading a partial byte for the last byte, shift right so the data is aligned on the right
		{

			if ( alignBitsToRight )
				* ( output + offset ) >>= -numberOfBitsToRead;

			readOffset += 8 + numberOfBitsToRead;
		}
		else
			readOffset += 8;

		offset++;

	}

	//} while(numberOfBitsToRead>0);

	return true;
}

// Assume the input source points to a compressed native type. Decompress and read it
bool BitStream::ReadCompressed( unsigned char* output,
	const int size, const bool unsignedData )
{
	int currentByte = ( size >> 3 ) - 1;


	unsigned char byteMatch, halfByteMatch;

	if ( unsignedData )
	{
		byteMatch = 0;
		halfByteMatch = 0;
	}

	else
	{
		byteMatch = 0xFF;
		halfByteMatch = 0xF0;
	}

	// Upper bytes are specified with a single 1 if they match byteMatch
	// From high byte to low byte, if high byte is a byteMatch then write a 1 bit. Otherwise write a 0 bit and then write the remaining bytes
	while ( currentByte > 0 )
	{
		// If we read a 1 then the data is byteMatch.

		bool b;

		if ( Read( b ) == false )
			return false;

		if ( b )   // Check that bit
		{
			output[ currentByte ] = byteMatch;
			currentByte--;
		}
		else
		{
			// Read the rest of the bytes

			if ( ReadBits( output, ( currentByte + 1 ) << 3 ) == false )
				return false;

			return true;
		}
	}

	// All but the first bytes are byteMatch.  If the upper half of the last byte is a 0 (positive) or 16 (negative) then what we read will be a 1 and the remaining 4 bits.
	// Otherwise we read a 0 and the 8 bytes
	//assert(readOffset+1 <=numberOfBitsUsed); // If this assert is hit the stream wasn't long enough to read from
	if ( readOffset + 1 > numberOfBitsUsed )
		return false;

	bool b;

	if ( Read( b ) == false )
		return false;

	if ( b )   // Check that bit
	{

		if ( ReadBits( output + currentByte, 4 ) == false )
			return false;

		output[ currentByte ] |= halfByteMatch; // We have to set the high 4 bits since these are set to 0 by ReadBits
	}
	else
	{
		if ( ReadBits( output + currentByte, 8 ) == false )
			return false;
	}

	return true;
}

// Reallocates (if necessary) in preparation of writing numberOfBitsToWrite
void BitStream::AddBitsAndReallocate( const int numberOfBitsToWrite )
{
	if ( numberOfBitsToWrite <= 0 )
		return;

	int newNumberOfBitsAllocated = numberOfBitsToWrite + numberOfBitsUsed;

	if ( numberOfBitsToWrite + numberOfBitsUsed > 0 && ( ( numberOfBitsAllocated - 1 ) >> 3 ) < ( ( newNumberOfBitsAllocated - 1 ) >> 3 ) )   // If we need to allocate 1 or more new bytes
	{
#ifdef _DEBUG
		// If this assert hits then we need to specify true for the third parameter in the constructor
		// It needs to reallocate to hold all the data and can't do it unless we allocated to begin with
		assert( copyData == true );
#endif

		// Less memory efficient but saves on news and deletes
		newNumberOfBitsAllocated = ( numberOfBitsToWrite + numberOfBitsUsed ) * 2;
//		int newByteOffset = BITS_TO_BYTES( numberOfBitsAllocated );
		// Use realloc and free so we are more efficient than delete and new for resizing
		int amountToAllocate = BITS_TO_BYTES( newNumberOfBitsAllocated );
		if (data==(unsigned char*)stackData)
		{
			 if (amountToAllocate > BITSTREAM_STACK_ALLOCATION_SIZE)
			 {
				 data = ( unsigned char* ) malloc( amountToAllocate );

				 // need to copy the stack data over to our new memory area too
				 memcpy ((void *)data, (void *)stackData, BITS_TO_BYTES( numberOfBitsAllocated ));
			 }
		}
		else
		{
			data = ( unsigned char* ) realloc( data, amountToAllocate );
		}

#ifdef _DEBUG
		assert( data ); // Make sure realloc succeeded
#endif
		//  memset(data+newByteOffset, 0,  ((newNumberOfBitsAllocated-1)>>3) - ((numberOfBitsAllocated-1)>>3)); // Set the new data block to 0
	}

	if ( newNumberOfBitsAllocated > numberOfBitsAllocated )
		numberOfBitsAllocated = newNumberOfBitsAllocated;
}

// Should hit if reads didn't match writes
void BitStream::AssertStreamEmpty( void )
{
	assert( readOffset == numberOfBitsUsed );
}

void BitStream::PrintBits( void ) const
{
	if ( numberOfBitsUsed <= 0 )
	{
		printf( "No bits\n" );
		return ;
	}

	for ( int counter = 0; counter < BITS_TO_BYTES( numberOfBitsUsed ); counter++ )
	{
		int stop;

		if ( counter == ( numberOfBitsUsed - 1 ) >> 3 )
			stop = 8 - ( ( ( numberOfBitsUsed - 1 ) % 8 ) + 1 );
		else
			stop = 0;

		for ( int counter2 = 7; counter2 >= stop; counter2-- )
		{
			if ( ( data[ counter ] >> counter2 ) & 1 )
				putchar( '1' );
			else
				putchar( '0' );
		}

		putchar( ' ' );
	}

	putchar( '\n' );
}


// Exposes the data for you to look at, like PrintBits does.
// Data will point to the stream.  Returns the length in bits of the stream.
int BitStream::CopyData( unsigned char** _data ) const
{
#ifdef _DEBUG
	assert( numberOfBitsUsed > 0 );
#endif

	*_data = new unsigned char [ BITS_TO_BYTES( numberOfBitsUsed ) ];
	memcpy( *_data, data, sizeof(unsigned char) * ( BITS_TO_BYTES( numberOfBitsUsed ) ) );
	return numberOfBitsUsed;
}

// Ignore data we don't intend to read
void BitStream::IgnoreBits( const int numberOfBits )
{
	readOffset += numberOfBits;
}

void BitStream::IgnoreBytes( const int numberOfBytes )
{
	IgnoreBits( 8 * numberOfBytes );
}


// Move the write pointer to a position on the array.  Dangerous if you don't know what you are doing!
void BitStream::SetWriteOffset( const int offset )
{
	numberOfBitsUsed = offset;
}

// Returns the length in bits of the stream
int BitStream::GetNumberOfBitsUsed( void ) const
{
	return numberOfBitsUsed;
}

// Returns the length in bytes of the stream
int BitStream::GetNumberOfBytesUsed( void ) const
{
	return BITS_TO_BYTES( numberOfBitsUsed );
}

// Returns the number of bits into the stream that we have read
int BitStream::GetReadOffset( void ) const
{
	return readOffset;
}

// Returns the number of bits left in the stream that haven't been read
int BitStream::GetNumberOfUnreadBits( void ) const
{
	return numberOfBitsUsed - readOffset;
}

// Exposes the internal data
unsigned char* BitStream::GetData( void ) const
{
	return data;
}

// If we used the constructor version with copy data off, this makes sure it is set to on and the data pointed to is copied.
void BitStream::AssertCopyData( void )
{
	if ( copyData == false )
	{
		copyData = true;

		if ( numberOfBitsAllocated > 0 )
		{
			unsigned char * newdata = ( unsigned char* ) malloc( BITS_TO_BYTES( numberOfBitsAllocated ) );
#ifdef _DEBUG

			assert( data );
#endif

			memcpy( newdata, data, BITS_TO_BYTES( numberOfBitsAllocated ) );
			data = newdata;
		}

		else
			data = 0;
	}
}
