/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Internal Packet Pool Class Declaration. 
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

