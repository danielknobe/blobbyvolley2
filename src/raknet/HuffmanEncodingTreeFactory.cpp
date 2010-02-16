/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file HuffmanEncodingTreeFactory.cpp 
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
