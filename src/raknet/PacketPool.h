/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file 
* @brief Manage memory for packet. 
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
#ifndef __PACKET_POOL
#define __PACKET_POOL
#include "SimpleMutex.h"
#include "RakNetQueue.h"
#include "NetworkTypes.h"

/**
* @brief Manage memory for packet. 
*
*
* The PacketPool class as multiple roles : 
*  - Managing memory associated to packets 
*  - Reuse memory of old packet to increase performances. 
* @note it implements Singleton Pattern 
* 
*/

class PacketPool
{

public:
	/**
	* Constructor
	*/
	PacketPool();
	/**
	* Destructor
	*/
	~PacketPool();
	/**
	* Get Memory for a packet
	* @return a Packet object 
	*/
	Packet* GetPointer( void );
	/**
	* Free Memory for a packet
	* @param p The packet to free 
	*/
	void ReleasePointer( Packet *p );
	/**
	* Clear the Packet Pool 
	*/
	void ClearPool( void );

	/**
	* Retrieve the unique  instance of a PacketPool. 
	* @return A pointer to the pool.  
	*/
	static inline PacketPool* Instance()
	{
		return & I;
	}

private:
	/**
	* Store packets 
	*/
	BasicDataStructures::Queue<Packet*> pool;
	/**
	* Exclusive access to the pool
	*/
	SimpleMutex poolMutex;
	/**
	* Singleton Pattern unique instance 
	*/
	static PacketPool I;
#ifdef _DEBUG
	/**
	* In debugging stage, stores the number of packet released
	*/
	int packetsReleased;
#endif
};

#endif

