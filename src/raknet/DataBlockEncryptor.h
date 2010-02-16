/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief DataBlockEncryptor Class Declaration 
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
