/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Internal Packet Pool Class Declaration. 
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
#ifndef __INTERNAL_PACKET_POOL
#define __INTERNAL_PACKET_POOL
//#include "SimpleMutex.h"
#include "RakNetQueue.h"
#include "InternalPacket.h"

/**
 * @brief Manage Internal Packet using pools. 
 * 
 * This class provide memory management for packets used internally in RakNet. 
 * @see PacketPool 
 * 
 * @note Implement Singleton Pattern 
 * 
 */
class InternalPacketPool
{
public:
	/**
	 * Constructor
	 */
	InternalPacketPool();
	/**
	 * Destructor
	 */
	~InternalPacketPool();
	/**
	 * Retrieve a new InternalPacket instance. 
	 * @return a pointer to an InternalPacket structure. 
	 */
	InternalPacket* GetPointer( void );
	/**
	 * Free am InternalPacket instance
	 * @param p a pointer to the InternalPacket instance. 
	 */
	void ReleasePointer( InternalPacket *p );
	/**
	 * Clear the pool 
	 */
	void ClearPool( void );

private:
	/**
	 * InternalPacket pool 
	 */
	BasicDataStructures::Queue<InternalPacket*> pool;
	/**
	 * Multithread access management 
	 */
	// 10/17/05 - Don't need this now that all pool interactions are in the same thread
	//SimpleMutex poolMutex;
#ifdef _DEBUG
	/**
	 * Used in debugging stage to monitor the number of internal packet released. 
	 */
	int packetsReleased;
#endif
};

#endif

