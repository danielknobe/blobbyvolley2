/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief CheckSum class declaration 
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
 */
#ifndef __CHECKSUM_H
#define __CHECKSUM_H

/**
 * This class provide checksuming service. 
 * 
 */

class CheckSum
{

public:
	/**
	 * Default constructor
	 */
	CheckSum()
	{
		clear();
	}
	
	/**
	 * Reset to an initial state. 
	 */
	void clear()
	{
		sum = 0;
		r = 55665;
		c1 = 52845;
		c2 = 22719;
	}
	
	/**
	 * add data to the checksum 
	 * @param w add a dword of data 
	 */
	
	void add ( unsigned int w )
	
	;
	
	/**
	 * add data to the checksum 
	 * @param w a word of data.
	 */
	void add ( unsigned short w )
	
	;
	/**
	 * add an array of byte to the checksum. 
	 * @param b a pointer to the buffer.
	 * @param length the size of the buffer. 
	 */
	void add ( unsigned char* b, unsigned int length )
	
	;
	/**
	 * Add one byte of data for checksuming 
	 * @param b a byte of data. 
	 */
	void add ( unsigned char b )
	
	;
	/**
	 * Get the checksum of the data. 
	 */
	unsigned int get ()
	{
		return sum;
	}
	
protected:
	unsigned short r;
	unsigned short c1;
	unsigned short c2;
	unsigned int sum;
};

#endif
