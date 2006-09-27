/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file HuffmanEncodingTreeFactory.h
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

#ifndef __HUFFMAN_ENCODING_TREE_FACTORY
#define __HUFFMAN_ENCODING_TREE_FACTORY

class HuffmanEncodingTree;

/**
 * This generates a special case of the huffman encoding tree with 8 bit keys
 */

class HuffmanEncodingTreeFactory
{

public:
	/**
	 * Default Constructor
	 */
	HuffmanEncodingTreeFactory();
	/**
	 * Reset the frequency table.  You don't need to call this unless you want to reuse the class for a new tree
	 */
	void Reset( void );
	/**
	 * Pass an array of bytes to this to add those elements to the frequency table
	 * @param array the data to insert into the frequency table 
	 * @param size the size of the data to insert 
	 */
	void AddToFrequencyTable( unsigned char *array, int size );
	/**
	 * Copies the frequency table to the array passed
	 * Retrieve the frequency table 
	 * @param _frequency The frequency table used currently
	 */
	void GetFrequencyTable( unsigned int _frequency[ 256 ] );
	/**
	 * Returns the frequency table as a pointer
	 * @return the address of the frenquency table 
	 */
	unsigned int * GetFrequencyTable( void );
	/**
	 * Generate a HuffmanEncodingTree.
	 * You can also use GetFrequencyTable and GenerateFromFrequencyTable in the tree itself
	 */
	HuffmanEncodingTree * GenerateTree( void );
	
private:
	/**
	 * 256 frequencies.  
	 */
	unsigned int frequency[ 256 ];
};

#endif
