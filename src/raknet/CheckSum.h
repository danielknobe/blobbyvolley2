/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief CheckSum class declaration 
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
