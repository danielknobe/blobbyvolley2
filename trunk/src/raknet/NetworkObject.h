/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief NetworkObject Class Declaration.  Derive from NetworkObject to generate IDs.  This class utilizes the Multiplayer and DistributedNetworkObject classes
 * NetworkObject is an add-on system that uses RakNet and interfaces with other add-on systems.
 * NetworkIDGenerator is a core system in RakNet and does not interface with any external systems
 * DEPRECIATED! Use NetworkIDGenerator instead!
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

#if !defined(AFX_NETWORKOBJECT_H__29266376_3F9E_42CD_B208_B58957E1935B__INCLUDED_)
#define      AFX_NETWORKOBJECT_H__29266376_3F9E_42CD_B208_B58957E1935B__INCLUDED_

#include "NetworkIDGenerator.h"

/**
 * A NetworkObject is used to identify uniquely an object on the
 * network You can easly create objects with network unique id by
 * subclassing this class.
 */

//#pragma deprecated(NetworkObject) // Use NetworkIDGenerator directly from now on
class NetworkObject : public NetworkIDGenerator
{

public:
	/**
	* Get the id of the object 
	* @return the id of the current object 
	*/
	virtual unsigned short GetID( void );
	/**
	 * Associate an id to an object 
	 * @param id the new id of the network object.
	 * @note Only the server code should
	 * call this.
	 *
	 */
	virtual void SetID( unsigned short id );

	/**
	* These function is only meant to be used when saving games as you
	* should save the HIGHEST value staticItemID has achieved upon save
	* and reload it upon load.  Save AFTER you've created all the items
	* derived from this class you are going to create.  
	* @return the HIGHEST Object Id currently used 
	*/
	static unsigned short GetStaticItemID( void );
	/**
	* These function is only meant to be used when loading games. Load
	* BEFORE you create any new objects that are not SetIDed based on
	* the save data. 
	* @param i the highest number of NetworkIDGenerator reached. 
	*/
	static void SetStaticItemID( unsigned short i );

	// Overloaded from NetworkIDGenerator
	virtual bool IsObjectIDAuthority(void) const;
	virtual bool IsObjectIDAuthorityActive(void) const;
	virtual bool IsObjectIDRecipient(void) const;
	virtual bool IsObjectIDRecipientActive(void) const;
	
protected:
};

#endif
