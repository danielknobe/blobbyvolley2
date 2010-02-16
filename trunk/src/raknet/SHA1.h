 /* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
 /**
 * @file
 * @brief SHA-1 computation class 
 *
 * 100% free public domain implementation of the SHA-1
 * algorithm by Dominik Reichl <Dominik.Reichl@tiscali.de>
 *
 *
 * === Test Vectors (from FIPS PUB 180-1) ===
 *
 * "abc"
 *  A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
 *
 * "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
 *  84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
 *
 * A million repetitions of "a"
 *  34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
 */

#ifndef ___SHA1_H___
#define ___SHA1_H___

#include <stdio.h> // Needed for file access
#include <memory.h> // Needed for memset and memcpy
#include <string.h> // Needed for strcat and strcpy
#include "Types.h" 
/**
* Size of the read buffer when computing SHA-1 sum on a file 
*/
#define MAX_FILE_READ_BUFFER 8000 
/**
* Size of an SHA-1 key in byte 
*/
#define SHA1_LENGTH 20

/**
* Computes SHA-1 hashing class 
*/

class CSHA1
{

public:
	// Rotate x bits to the left
	// #define ROL32(value, bits) (((value)<<(bits))|((value)>>(32-(bits))))

#ifdef LITTLE_ENDIAN
#define SHABLK0(i) (block->l[i] = (ROL32(block->l[i],24) & 0xFF00FF00) \
	| (ROL32(block->l[i],8) & 0x00FF00FF))
#else
#define SHABLK0(i) (block->l[i])
#endif

#define SHABLK(i) (block->l[i&15] = ROL32(block->l[(i+13)&15] ^ block->l[(i+8)&15] \
	^ block->l[(i+2)&15] ^ block->l[i&15],1))

	// SHA-1 rounds
#define R0(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK0(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define R1(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define R2(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0x6ED9EBA1+ROL32(v,5); w=ROL32(w,30); }
#define R3(v,w,x,y,z,i) { z+=(((w|x)&y)|(w&x))+SHABLK(i)+0x8F1BBCDC+ROL32(v,5); w=ROL32(w,30); }
#define R4(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0xCA62C1D6+ROL32(v,5); w=ROL32(w,30); }

	typedef union {
		unsigned char c[ 64 ];
		unsigned int l[ 16 ];
	} SHA1_WORKSPACE_BLOCK;
	/*
	* Two different formats for ReportHash(...) 
	*/
	enum { REPORT_HEX = 0,   /**< Report in hexa */
		REPORT_DIGIT = 1 /**< Report in digit */};

	/**
	* Default Constructor 
	*/
	CSHA1();
	/**
	* Destructor
	*/
	virtual ~CSHA1();

	unsigned int m_state[ 5 ];
	unsigned int m_count[ 2 ];
	unsigned char m_buffer[ 64 ];
	unsigned char m_digest[ 20 ];
	/**
	* Reset the internal state of the SHA-1 computation 
	* to compute a new SHA-1 hash key 
	*/
	void Reset();

	/**
	* Update the hash value from a byte buffer 
	* @param data the byte buffer 
	* @param len the number of byte available in the byte buffer  
	*/
	void Update( unsigned char* data, unsigned int len );
	/**
	* Compute the SHA-1 hash key of a file 
	* @param szFileName The filename 
	* @return true on success false otherwise 
	*/
	bool HashFile( char *szFileName );

	/**
	* Finalize hash and report
	*/
	void Final();
	/**
	* Retrieve the hash key 
	* @param szReport an array of byte containing the hash key 
	* @param uReportType the format of the report might be REPORT_HEX or REPORT_DIGIT 
	*/
	void ReportHash( char *szReport, unsigned char uReportType = REPORT_HEX );
	/**
	* Retrieve the Hash in a previously allocated array  
	* @param uDest the destination array 
	*/
	void GetHash( unsigned char *uDest );
	/**
	* Get a pointer to the hash key data. 
	*/
	unsigned char * GetHash( void ) const;

private:
	/**
	* Private SHA-1 transformation
	*/
	void Transform( unsigned int state[ 5 ], unsigned char buffer[ 64 ] );
	/**
	* Moved here for thread and multiple instance safety
	*/
	unsigned char workspace[ 64 ];
};

#endif // ___SHA1_H___

