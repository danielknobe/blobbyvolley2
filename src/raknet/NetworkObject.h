/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief NetworkObject Class Declaration.  Derive from NetworkObject to generate IDs.  This class utilizes the Multiplayer and DistributedNetworkObject classes
 * NetworkObject is an add-on system that uses RakNet and interfaces with other add-on systems.
 * NetworkIDGenerator is a core system in RakNet and does not interface with any external systems
 * DEPRECIATED! Use NetworkIDGenerator instead!
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
