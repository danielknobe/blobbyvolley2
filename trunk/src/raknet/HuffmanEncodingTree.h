/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file HuffmanEncodingTree.h
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
#ifndef __HUFFMAN_ENCODING_TREE
#define __HUFFMAN_ENCODING_TREE

#include "HuffmanEncodingTreeNode.h"
#include "BitStream.h"

#include "LinkedList.h" 
/**
 * This generates special cases of the huffman encoding tree using 8 bit keys with the additional condition that unused combinations of 8 bits are treated as a frequency of 1
 */

class HuffmanEncodingTree
{

public:
	HuffmanEncodingTree();
	~HuffmanEncodingTree();
	/**
	 * Pass an array of bytes to array and a preallocated BitStream to receive the output
	 */
	void EncodeArray( unsigned char *input, int sizeInBytes, RakNet::BitStream * output );
	/**
	 * Two versions that perform the same operation.
	 * The second version also the number of bytes in the stream, which may be greater than the max chars to write
	 */
	int DecodeArray( RakNet::BitStream * input, int sizeInBits, int maxCharsToWrite, unsigned char *output );
	void DecodeArray( unsigned char *input, int sizeInBits, RakNet::BitStream * output );
	/**
	 * Given a frequency table of 256 elements, all with a frequency of 1 or more, generate the tree
	 */
	void GenerateFromFrequencyTable( unsigned int frequencyTable[ 256 ] );
	/**
	 * Free the memory used by the tree
	 */
	void FreeMemory( void );
	
private:
	/**
	 * The root node of the tree 
	 */
	HuffmanEncodingTreeNode *root;
	/**
	 * Used to hold bit encoding for one character
	 */
	
	struct CharacterEncoding
	{
		unsigned char* encoding;
		unsigned short bitLength;
	};
	
	CharacterEncoding encodingTable[ 256 ];
	
	void InsertNodeIntoSortedList( HuffmanEncodingTreeNode * node, BasicDataStructures::LinkedList<HuffmanEncodingTreeNode *> *huffmanEncodingTreeNodeList ) const;
};

#endif
