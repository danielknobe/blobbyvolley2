/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief DataBlockEncryptor Class Declaration 
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
#ifndef __DATA_BLOCK_ENCRYPTOR_H
#define __DATA_BLOCK_ENCRYPTOR_H

#include "rijndael.h"

/**
 * Encrypt and Decrypt block of data 
 * 
 */

class DataBlockEncryptor
{

public:
	/**
	 * Default Constructor 
	 */
	DataBlockEncryptor();
	/**
	 * Destructor
	 */
	~DataBlockEncryptor();
	/**
	 * Test if encryption/decryption key are set 
	 * @return true if SetKey has been called previously 
	 */
	bool IsKeySet( void ) const;
	
	/**
	 * Set the encryption key 
	 * @param key The new encryption key 
	 */
	void SetKey( const unsigned char key[ 16 ] );
	/**
	 * Unset the encryption key 
	 */
	void UnsetKey( void );
	
	/**
	 * Encrypt adds up to 15 bytes.  Output should be large enough to hold this.
	 * Output can be the same memory block as input
	 * @param input the input buffer to encrypt 
	 * @param inputLength the size of the @em input buffer 
	 * @param output the output buffer to store encrypted data 
	 * @param outputLength the size of the output buffer 
	 */
	void Encrypt( unsigned char *input, int inputLength, unsigned char *output, int *outputLength );
	
	/**
	 * Decrypt removes bytes, as few as 6.  Output should be large enough to hold this.
	 * Output can be the same memory block as input
	 * @param input the input buffer to decrypt 
	 * @param inputLength the size of the @em input buffer 
	 * @param output the output buffer to store decrypted data 
	 * @param outputLength the size of the @em output buffer 
	 * @return False on bad checksum or input, true on success
	 */
	bool Decrypt( unsigned char *input, int inputLength, unsigned char *output, int *outputLength );
	
protected:
	/**
	 * The encryption / decryption key 
	 */
	//AES128 secretKeyAES128; // TODO
	keyInstance keyEncrypt;
	keyInstance keyDecrypt;
	cipherInstance cipherInst;
	/**
	 * True if a key is set 
	 */
	bool keySet;
};

#endif
