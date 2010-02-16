	/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief RakNetworkFactory class is a factory for communication End Point.
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

#ifndef __RAK_NETWORK_FACTORY_H
#define __RAK_NETWORK_FACTORY_H

class RakClientInterface;

class RakServerInterface;

class RakPeerInterface;

#ifdef _WIN32
#define RAK_DLL_EXPORT __declspec(dllexport)
#else 
// Unix needs no export, but for name mangling, keep the function name
// clean. If you omit the 'extern "C"', the .so names will be
// compiler dependent.
#define RAK_DLL_EXPORT extern "C"
#endif

#if defined(DLL_EXPORTS) || defined(_USRDLL)

class RAK_DLL_EXPORT RakNetworkFactory
#else 
//class __declspec( dllimport ) RakNetworkFactor
/**
* @brief Communication End Point Provider
*
* This class is in charge of creating and managing Peers. You should always
* pass throught this class to get a communication End Point.
*/

class RakNetworkFactory
#endif
{

public:
	/**
	 * Returns a new instance of the network client.
	 */
	static RakClientInterface* GetRakClientInterface( void );
	/**
	 * Returns a new instance of the network server.
	 */
	static RakServerInterface* GetRakServerInterface( void );
	/**
	 * Returns a new instance of the network server.
	 */
	static RakPeerInterface* GetRakPeerInterface( void );
	/**
	 * Destroys an instance of the network client.
	 */
	static void DestroyRakClientInterface( RakClientInterface* i );
	/**
	 * Destroys an instance of the network server.
	 */
	static void DestroyRakServerInterface( RakServerInterface* i );
	/**
	 * Destroys an instance of the network server.
	 */
	static void DestroyRakPeerInterface( RakPeerInterface* i );
};

#endif
