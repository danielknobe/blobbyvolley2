/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Manage memory for packet. 
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

#ifndef __PACKET_POOL
#define __PACKET_POOL
#include "SimpleMutex.h"
#include "NetworkTypes.h"

#include "../blobnet/adt/Queue.hpp"
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
	BlobNet::ADT::Queue<Packet*> pool;
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

