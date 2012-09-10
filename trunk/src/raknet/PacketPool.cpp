/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief 
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

#include "PacketPool.h"
#include <cassert>

PacketPool::PacketPool()
{
#ifdef _DEBUG
	packetsReleased = 0;
#endif
}

PacketPool::~PacketPool()
{
#ifdef _DEBUG
	// If this assert hits then not all packets given through GetPointer have been returned to ReleasePointer.
	// Either
	// 1. You got a packet from Receive and didn't give it back to DeallocatePacket when you were done with it
	// 2. You didn't call Disconnect before shutdown, and the order of destructor calls happened to hit the PacketPool singleton before it hit the RakPeer class(es).
	assert( packetsReleased == 0 );
#endif

	ClearPool();
}

void PacketPool::ClearPool( void )
{
	Packet * p;
	poolMutex.Lock();

	while ( !pool.empty() )
	{
		p = pool.top();
		pool.pop();
		delete [] p->data;
		delete p;
	}

	poolMutex.Unlock();
}

Packet* PacketPool::GetPointer( void )
{
	Packet * p = 0;
	poolMutex.Lock();

#ifdef _DEBUG

	packetsReleased++;
#endif

	if ( !pool.empty() )
	{
		p = pool.top();
		pool.pop();
	}
	
	poolMutex.Unlock();

	if ( p )
		return p;

	p = new Packet;

	p->data = 0;

	return p;
}

void PacketPool::ReleasePointer( Packet *p )
{
	if ( p == 0 )
	{
		// Releasing a null pointer?
#ifdef _DEBUG
		assert( 0 );
#endif

		return ;
	}

	delete [] p->data;
	p->data = 0;

	poolMutex.Lock();
	pool.push( p );
#ifdef _DEBUG

//	assert( packetsReleased > 0 );
	packetsReleased--;
#endif

	poolMutex.Unlock();
}

