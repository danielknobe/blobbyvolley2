/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file HuffmanEncodingTreeFactory.cpp 
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

#include "HuffmanEncodingTreeFactory.h"
#include "HuffmanEncodingTreeNode.h"
#include "HuffmanEncodingTree.h"
#include <memory.h>

HuffmanEncodingTreeFactory::HuffmanEncodingTreeFactory()
{
	Reset();
}

// Reset the frequency table.  You don't need to call this unless you want to reuse the class for a new tree
void HuffmanEncodingTreeFactory::Reset( void )
{
	for ( int counter = 0; counter < 256; counter++ )
		frequency[ counter ] = 0;
}

// Pass an array of bytes to this to add those elements to the frequency table
void HuffmanEncodingTreeFactory::AddToFrequencyTable( unsigned char *array, int size )
{
	while ( size-- > 0 )
		frequency[ array[ size ] ] ++;
}

// Copies the frequency table to the array passed
void HuffmanEncodingTreeFactory::GetFrequencyTable( unsigned int _frequency[ 256 ] )
{
	memcpy( _frequency, frequency, sizeof( unsigned int ) * 256 );
}

unsigned int * HuffmanEncodingTreeFactory::GetFrequencyTable( void )
{
	return frequency;
}

// Generate a HuffmanEncodingTree.
// You can also use GetFrequencyTable and GenerateFromFrequencyTable in the tree itself
HuffmanEncodingTree * HuffmanEncodingTreeFactory::GenerateTree( void )
{
	HuffmanEncodingTree * huffmanEncodingTree = new HuffmanEncodingTree;
	huffmanEncodingTree->GenerateFromFrequencyTable( frequency );
	return huffmanEncodingTree;
}
