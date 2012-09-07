/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Provide String Encoding Class 
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
	void EncodeString( const char *input, int maxCharsToWrite, RakNet::BitStream *output );
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
