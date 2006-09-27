/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Internal Packet Pool Implementation  
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
#include "InternalPacketPool.h"
#include <assert.h>

InternalPacketPool::InternalPacketPool()
{
#ifdef _DEBUG
	packetsReleased = 0;
#endif
	
	// Speed things up by not reallocating at runtime when a mutex is locked.
	pool.clearAndForceAllocation( 64 );
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
	InternalPacket * p;
	//poolMutex.Lock();
	
	while ( pool.size() )
	{
		p = pool.pop();
		delete p;
	}
	
	//poolMutex.Unlock();
}

InternalPacket* InternalPacketPool::GetPointer( void )
{
#ifdef _DEBUG
	packetsReleased++;
#endif
	
	InternalPacket *p = 0;
	//poolMutex.Lock();
	if ( pool.size() )
		p = pool.pop();
	//poolMutex.Unlock();
	
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
	//poolMutex.Lock();
	pool.push( p );
	//poolMutex.Unlock();
}

