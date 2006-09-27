/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file HuffmanEncodingTree.h
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
