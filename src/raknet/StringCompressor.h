/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Provide String Encoding Class 
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
#ifndef __STRING_COMPRESSOR_H
#define __STRING_COMPRESSOR_H

#include "BitStream.h"

class HuffmanEncodingTree;

/**
 * This class provide compression for text in natural language. It is
 * based on frequencies table and an Huffman encoding algorithm. This
 * class follow the Singleton pattern. The default frequency table is
 * for english language. You should generate your first frequencies
 * table for the language in use in your application. 
 */
class StringCompressor
{

public:
	/**
	 * Destructor
	 */
	~StringCompressor();
	/**
	 * static function because only static functions can access static members
	 * Singleton pattern 
	 * @return the unique instance of the StringCompressor 
	 * 
	 * @todo Ensure there is no way to create a StringCompressor object 
	 * 
	 */
	static inline StringCompressor* Instance()
	{
		return & instance;
	}
	
	/**
	 * Given an array of strings, such as a chat log, generate the optimal encoding tree for it.
	 * This function is optional and if it is not called a default tree will be used instead.
	 * @param input An array of byte 
	 * @param inputLength The number of byte in the buffer 
	 */
	void GenerateTreeFromStrings( unsigned char *input, unsigned inputLength );
	/**
	 * Writes input to output, compressed.  Takes care of the null terminator for you
	 * @param input a byte buffer 
	 * @param maxCharsToWrite The size of @em input 
	 * @param output The bitstream that will contain the data of the compressed string 
	 */
	void EncodeString( char *input, int maxCharsToWrite, RakNet::BitStream *output );
	/**
	 * Writes input to output, uncompressed.  Takes care of the null terminator for you.
	 * maxCharsToWrite should be the allocated size of output
	 * @param[out] output a byte buffer previously allocated.
	 * @param maxCharsToWrite The amount of byte available to store the result. There should be 
	 * enought space in @em output to decode all the data 
	 * @param input The bitstream containing the data to process. 
	 */
	bool DecodeString( char *output, int maxCharsToWrite, RakNet::BitStream *input );
	
private:
	/**
	 * Create Huffman frequencies Tree used for both encoding and decoding 
	 */
	void GenerateHuffmanEncodingTree( void );
	/**
	 * Default Constructor 
	 */
	StringCompressor();
	/**
	 * Singleton instance 
	 */
	static StringCompressor instance;
	/**
	 * Huffman frequencies Tree
	 */
	HuffmanEncodingTree *huffmanEncodingTree;
};

/**
 * Provide a shorcut to access to the unique instance of the 
 * string compressor singleton. 
 */
#define stringCompressor StringCompressor::Instance()

#endif
