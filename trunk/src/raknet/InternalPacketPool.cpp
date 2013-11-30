/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Internal Packet Pool Implementation
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
#include "InternalPacketPool.h"
#include <assert.h>

InternalPacketPool::InternalPacketPool()
{
#ifdef _DEBUG
	packetsReleased = 0;
#endif
	
	unsigned i;
	for (i=0; i < 64; i++)
		pool.push(new InternalPacket);
}

InternalPacketPool::~InternalPacketPool()
{
#ifdef _DEBUG
	// If this assert hits then not all packets given through GetPointer have been returned to ReleasePointer
	assert( packetsReleased == 0 );
#endif
	
	ClearPool();
}

void InternalPacketPool::ClearPool( void )
{
	while ( !pool.empty() )
	{
		InternalPacket* p = pool.top();
		pool.pop();
		delete p;
	}
}

InternalPacket* InternalPacketPool::GetPointer( void )
{
#ifdef _DEBUG
	packetsReleased++;
#endif

	InternalPacket *p = 0;

	if ( !pool.empty() )
	{
		p = pool.top();
		pool.pop();
	}

	if ( p )
		return p;

	p = new InternalPacket;
#ifdef _DEBUG
	p->data=0;
#endif
	return p;
}

void InternalPacketPool::ReleasePointer( InternalPacket *p )
{
	if ( p == 0 )
	{
		// Releasing a null pointer?
#ifdef _DEBUG
		assert( 0 );
#endif
		return ;
	}
	
#ifdef _DEBUG
	packetsReleased--;
#endif
#ifdef _DEBUG
	p->data=0;
#endif
	pool.push( p );
}

