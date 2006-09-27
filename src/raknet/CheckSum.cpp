/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file 
* @brief Provide implementation of the CheckSum class 
* 
* From http://www.flounder.com/checksum.htm
* 
*/
#include "CheckSum.h"

/****************************************************************************
*        CheckSum::add
* Inputs:
*   unsigned int d: word to add
* Result: void
* 
* Effect: 
*   Adds the bytes of the unsigned int to the CheckSum
****************************************************************************/

void CheckSum::add ( unsigned int value )
{
	union
	{
		unsigned int value;
		unsigned char bytes[ 4 ];
	}

	data;
	data.value = value;

	for ( unsigned int i = 0; i < sizeof( data.bytes ); i++ )
		add ( data.bytes[ i ] )

		;
} // CheckSum::add(unsigned int)

/****************************************************************************
*       CheckSum::add
* Inputs:
*   unsigned short value:
* Result: void
* 
* Effect: 
*   Adds the bytes of the unsigned short value to the CheckSum
****************************************************************************/

void CheckSum::add ( unsigned short value )
{
	union
	{
		unsigned short value;
		unsigned char bytes[ 2 ];
	}

	data;
	data.value = value;

	for ( unsigned int i = 0; i < sizeof( data.bytes ); i++ )
		add ( data.bytes[ i ] )

		;
} // CheckSum::add(unsigned short)

/****************************************************************************
*       CheckSum::add
* Inputs:
*   unsigned char value:
* Result: void
* 
* Effect: 
*   Adds the byte to the CheckSum
****************************************************************************/

void CheckSum::add ( unsigned char value )
{
	unsigned char cipher = (unsigned char)( value ^ ( r >> 8 ) );
	r = ( cipher + r ) * c1 + c2;
	sum += cipher;
} // CheckSum::add(unsigned char)


/****************************************************************************
*       CheckSum::add
* Inputs:
*   LPunsigned char b: pointer to byte array
*   unsigned int length: count
* Result: void
* 
* Effect: 
*   Adds the bytes to the CheckSum
****************************************************************************/

void CheckSum::add ( unsigned char *b, unsigned int length )
{
	for ( unsigned int i = 0; i < length; i++ )
		add ( b[ i ] )

		;
} // CheckSum::add(LPunsigned char, unsigned int)
