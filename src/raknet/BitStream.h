/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief RakNet::BitStream: packet encoding and decoding 
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
#ifndef  __BITSTREAM_H
#define __BITSTREAM_H

#ifdef _MSC_VER
#if defined (_INTEGRAL_MAX_BITS) &&  _INTEGRAL_MAX_BITS >= 64
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#define HAS_INT64
#endif
#else
#include <stdint.h>
#define HAS_INT64
#endif

// Arbitrary size, just picking something likely to be larger than most packets
#define BITSTREAM_STACK_ALLOCATION_SIZE 256

/**
 * @brief This namespace only contains a  few utility class used in 
 * RakNet. 
 * 
 * RakNet namespace is the not really in use currently. 
 */

/** \note  If you want the default network byte stream to be
    in Network Byte Order (Big Endian) then #define __BITSTREAM_BIG_END
    otherwise the default is 'Little Endian'.   If your CPU has the same
    Byte Order as your network stream, you can cut out some overheads
    using #define __BITSTREAM_NATIVE_END --- if this is defined,
    the __BITSTREAM_BIG_END flag becomes ineffective.
 */

namespace RakNet
{
	/**
	 * This macro transform a bit in byte 
	 * @param x Transform a bit to a byte 
	 */
#define BITS_TO_BYTES(x) (((x)+7)>>3)
	
	/**
	 * @brief Packets encoding and decoding facilities 
	 * 
	 * Helper class to encode and decode packets. 
	 * 
	 */
	
	class BitStream
	{
	
	public:
		/**
		 * Default Constructor 
		 */
		BitStream();
		/**
		 * Preallocate some memory for the construction of the packet 
		 * @param initialBytesToAllocate the amount of byte to pre-allocate. 
		 */
		BitStream( int initialBytesToAllocate );
		
		/**
		 * Initialize the BitStream object using data from the network. 
		 * Set _copyData to true if you want to make an internal copy of
		 * the data you are passing. You can then Write and do all other
		 * operations Set it to false if you want to just use a pointer to
		 * the data you are passing, in order to save memory and speed.
		 * You should only then do read operations.
		 * @param _data An array of bytes.
		 * @param lengthInBytes Size of the @em _data.
		 * @param _copyData Does a copy of the input data.  
		 */
		BitStream( char* _data, unsigned int lengthInBytes, bool _copyData );
		/**
		 * Destructor 
		 */
		~BitStream();
		/**
		 * Reset the bitstream for reuse
		 */
		void Reset( void );
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const bool input );
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const unsigned char input );
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const char input );
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const unsigned short input );
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const short input );
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const unsigned int input );
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const int input );
#if  defined ( __APPLE__ ) || defined (__APPLE_CC__ )||defined ( _WIN32 )
		// These are only provided for MS Windows and
		// Mac OSX (G4 processor) convenience and are
		// equivalent to the (int) versions.
		// The use of 'long' for any network data is
		// a fault since it will not be portable to 64-bit CPUs.
		void Write( const unsigned long input );
		void Write( const long input );
#endif

#ifdef HAS_INT64
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const uint64_t input );
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const int64_t input );
#endif
		
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const float input );
		/**
		 * Write the native types to the end of the buffer
		 * without any compression mecanism. 
		 * @param input The data 
		 */
		void Write( const double input );
		/**
		 * Write an array or casted stream. It is supossed to
		 * be raw data. It is also not possible to deal with endian problem 
		 * @param input a byte buffer 
		 * @param numberOfBytes the size of the byte buffer 
		 */
		void Write( const char* input, const int numberOfBytes );
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const unsigned char input );
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const char input );
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const unsigned short input );
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const short input );
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const unsigned int input );
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const int input );
#if  defined ( __APPLE__ ) || defined ( __APPLE_CC__ ) || defined ( _WIN32 )
		// These are only provided for MS Windows and
		// Mac OSX (G4 processor) convenience and are
		// equivalent to the (int) versions.
		// The use of 'long' for any network data is
		// a fault since it will not be portable to 64-bit CPUs.
		void WriteCompressed( const unsigned long input );
		void WriteCompressed( const long input );
#endif
	
#ifdef HAS_INT64
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const uint64_t input );
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const int64_t input );
#endif
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const float input );
		/**
		* Write 3 floats, using 1 byte each, where those floats comprise a normalized vector
		* Lossy, but accurate to 1/256.
		* 
		* @param input The data.
		*/
		void WriteCompressed1( const float input1, const float input2, const float input3 );
		/**
		* Write 3 floats, using 2 bytes each, where those floats comprise a normalized vector
		* Lossy, but accurate to 1/65536.
		* @param input The data.
		*/
		void WriteCompressed2( const float input1, const float input2, const float input3 );
		/**
		 * Write the native types with simple compression.
		 * Best used with  negatives and positives close to 0
		 * @param input The data.
		 */
		void WriteCompressed( const double input );
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( bool &output );
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( unsigned char &output );
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( char &output );
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( unsigned short &output );
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( short &output );
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( unsigned int &output );
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( int &output );
#if  defined ( __APPLE__ ) || defined ( __APPLE_CC__ ) || defined ( _WIN32 )
		// These are only provided for MS Windows and
		// Mac OSX (G4 processor) convenience and are
		// equivalent to the (int) versions.
		// The use of 'long' for any network data is
		// a fault since it will not be portable to 64-bit CPUs.
		bool Read( unsigned long &output );
		bool Read( long &output );
#endif
		
#ifdef HAS_INT64
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( uint64_t &output );
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( int64_t &output );
#endif
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( float &output );
		/**
		 * Read the native types from the front of the buffer
		 * @param output The readed value. 
		 * @return true on success false otherwise. The result of a reading 
		 * can only be wrong in the case we reach the end of the BitStream 
		 * with some missing bits. 
		 */
		bool Read( double &output );
		/**
		 * Read an array or casted stream of byte. The array
		 * is raw data. There is no automatic conversion on
		 * big endian arch
		 * @param output The result byte array. It should be larger than @em numberOfBytes. 
		 * @param numberOfBytes The number of byte to read
		 * @return true on success false if there is some missing bytes. 
		 */
		bool Read( char* output, const int numberOfBytes );
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( unsigned char & output );
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( char &output );
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( unsigned short &output );
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( short &output );
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( unsigned int &output );
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( int &output );

#if  defined ( __APPLE__ ) || defined ( __APPLE_CC__ )|| defined ( _WIN32 )
		// These are only provided for MS Windows and
		// Mac OSX (G4 processor) convenience and are
		// equivalent to the (int) versions.
		// The use of 'long' for any network data is
		// a fault since it will not be portable to 64-bit CPUs.
		bool ReadCompressed( unsigned long &output );
		bool ReadCompressed( long &output );
#endif

#ifdef HAS_INT64
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( uint64_t &output );
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( int64_t &output );
#endif
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( float &output );
		/**
		* Write 3 floats, using 1 byte each, where those floats comprise a normalized vector
		* Lossy, but accurate to 1/256.
		* 
		* @param output1, output2, output3 The read value
		* @param renormalizeOutput - self explanitory.  If you don't do this the output probably will not be normalized.
		*/
		bool ReadCompressed1( float &output1, float &output2, float &output3, bool renormalizeOutput );
		/**
		* Write 3 floats, using 2 bytes each, where those floats comprise a normalized vector
		* Lossy, but accurate to 1/65536.
		* @param output1, output2, output3 The read value.
		* @param renormalizeOutput - self explanitory.  If you don't do this the output probably will not be normalized.
		*/
		bool ReadCompressed2( float &output1, float &output2, float &output3, bool renormalizeOutput );
		/**
		 * Read the types you wrote with WriteCompressed
		 * @param output The read value
		 * @return true on success, false on not enough data to read
		 */
		bool ReadCompressed( double &output );
		/**
		 * Sets the read pointer back to the beginning of your data.
		 */
		void ResetReadPointer( void );
		/**
		* Sets the write pointer back to the beginning of your data.
		*/
		void ResetWritePointer( void );
		/**
		 * This is good to call when you are done with the stream to make
		 * sure you didn't leave any data left over void
		 */
		void AssertStreamEmpty( void );
		/**
		 * print to the standard output the state of the stream bit by bit 
		 */
		void PrintBits( void ) const;
		
		/**
		 * Ignore data we don't intend to read
		 * @param numberOfBits The number of bits to ignore
		 */
		void IgnoreBits( const int numberOfBits );
		
		/**
		 * Ignore data we don't intend to read
		 * @param numberOfBytes The number of bytes to ignore
		 */
		void IgnoreBytes( const int numberOfBytes );
		
		/**
		 * Move the write pointer to a position on the array.  
		 * @param offset the offset from the start of the array. 
		 * @attention 
		 * Dangerous if you don't know what you are doing!
		 *
		 */
		void SetWriteOffset( const int offset );
		/**
		 * Returns the length in bits of the stream
		 */
		int GetNumberOfBitsUsed( void ) const;
		/**
		 * Returns the length in bytes of the stream
		 */
		int GetNumberOfBytesUsed( void ) const;
		/**
		 * Returns the number of bits into the stream that we have read
		 */
		int GetReadOffset( void ) const;
		/**
		 * Returns the number of bits left in the stream that haven't been read
		 */
		int GetNumberOfUnreadBits( void ) const;
		/**
		 * Makes a copy of the internal data for you Data will point to
		 * the stream. Returns the length in bits of the stream. Partial
		 * bytes are left aligned 
		 * @param _data the resulting byte copy of the internal state. 
		 */
		int CopyData( unsigned char** _data ) const;
		/**
		 * Set the stream to some initial data.  For internal use
		 * Partial bytes are left aligned
		 * @param input The data
		 * @param numberOfBits the number of bits set in the data buffer 
		 */
		void SetData( const unsigned char* input, const int numberOfBits );
		/**
		 * Exposes the internal data.
		 * Partial bytes are left aligned.
		 * @return A pointer to the internal state 
		 */
		unsigned char* GetData( void ) const;
		/**
		 * Write numberToWrite bits from the input source Right aligned
		 * data means in the case of a partial byte, the bits are aligned
		 * from the right (bit 0) rather than the left (as in the normal
		 * internal representation) You would set this to true when
		 * writing user data, and false when copying bitstream data, such
		 * as writing one bitstream to another
		 * @param input The data 
		 * @param numberOfBitsToWrite The number of bits to write 
		 * @param rightAlignedBits if true data will be right aligned 
		 */
		void WriteBits( const unsigned char* input,
			int numberOfBitsToWrite, const bool rightAlignedBits = true );
		/**
		 * Align the bitstream to the byte boundary and then write the
		 * specified number of bits.  This is faster than WriteBits but
		 * wastes the bits to do the alignment and requires you to call
		 * ReadAlignedBits at the corresponding read position.
		 * @param input The data
		 * @param numberOfBytesToWrite The size of data. 
		 */
		void WriteAlignedBytes( const unsigned char* input,
			const int numberOfBytesToWrite );
		/**
		 * Read bits, starting at the next aligned bits. Note that the
		 * modulus 8 starting offset of the sequence must be the same as
		 * was used with WriteBits. This will be a problem with packet
		 * coalescence unless you byte align the coalesced packets.
		 * @param output The byte array larger than @em numberOfBytesToRead
		 * @param numberOfBytesToRead The number of byte to read from the internal state 
		 * @return true if there is enough byte. 
		 */
		bool ReadAlignedBytes( unsigned char* output,
			const int numberOfBytesToRead );
		/**
		 * Align the next write and/or read to a byte boundary.  This can
		 * be used to 'waste' bits to byte align for efficiency reasons It
		 * can also be used to force coalesced bitstreams to start on byte
		 * boundaries so so WriteAlignedBits and ReadAlignedBits both
		 * calculate the same offset when aligning.
		 */
		void AlignWriteToByteBoundary( void );
		/**
		 * Align the next write and/or read to a byte boundary.  This can
		 * be used to 'waste' bits to byte align for efficiency reasons It
		 * can also be used to force coalesced bitstreams to start on byte
		 * boundaries so so WriteAlignedBits and ReadAlignedBits both
		 * calculate the same offset when aligning.
		 */
		void AlignReadToByteBoundary( void );
		
		/**
		 * Read numberOfBitsToRead bits to the output source
		 * alignBitsToRight should be set to true to convert internal
		 * bitstream data to userdata It should be false if you used
		 * WriteBits with rightAlignedBits false
		 * @param output The resulting bits array 
		 * @param numberOfBitsToRead The number of bits to read 
		 * @param alignsBitsToRight if true bits will be right aligned. 
		 * @return true if there is enough bits to read 
		 */
		bool ReadBits( unsigned char* output, int numberOfBitsToRead,
			const bool alignBitsToRight = true );
		
		/**
		 * --- Low level functions --- 
		 * These are for when you want to deal
		 * with bits and don't care about type checking 
		 * Write a 0  
		 */
		void Write0( void );
		/**
		 * --- Low level functions --- 
		 * These are for when you want to deal
		 * with bits and don't care about type checking 
		 * Write a 1 
		 */
		void Write1( void );
		/**
		 * --- Low level functions --- 
		 * These are for when you want to deal
		 * with bits and don't care about type checking 
		 * Reads 1 bit and returns true if that bit is 1 and false if it is 0
		 */
		bool ReadBit( void );
		/**
		 * If we used the constructor version with copy data off, this
		 * makes sure it is set to on and the data pointed to is copied.
		 */
		void AssertCopyData( void );
		/**
		 * Use this if you pass a pointer copy to the constructor
		 * (_copyData==false) and want to overallocate to prevent
		 * reallocation
		 */
		void SetNumberOfBitsAllocated( const unsigned int lengthInBits );
		
	private:
		/**
		 * Assume the input source points to a native type, compress and write it.
		 */
		void WriteCompressed( const unsigned char* input,
			const int size, const bool unsignedData );
		
		/**
		 * Assume the input source points to a compressed native type.
		 * Decompress and read it.
		 */
		bool ReadCompressed( unsigned char* output,
			const int size, const bool unsignedData );
		
		/**
		 * Reallocates (if necessary) in preparation of writing
		 * numberOfBitsToWrite 
		 */
		void AddBitsAndReallocate( const int numberOfBitsToWrite );
		
		/**
		 * Number of bits currently used 
		 */
		int numberOfBitsUsed;
		/**
		 * Number of bits currently allocated 
		 */
		int numberOfBitsAllocated;
		/**
		 * Current readOffset 
		 */
		int readOffset;
		/**
		 * array of byte storing the data.  Points to stackData or if is bigger than that then is allocated
		 */
		unsigned char *data;
		/**
		 * true if the internal buffer is copy of the data passed to the
		 * constructor
		 */
		bool copyData;

		unsigned char stackData[BITSTREAM_STACK_ALLOCATION_SIZE];
	};	
}

#endif
