/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file 
* @brief 
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
#include "PacketPool.h"
#include <assert.h>

PacketPool PacketPool::I;

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
//	assert( packetsReleased == 0 );
#endif

	ClearPool();
}

void PacketPool::ClearPool( void )
{
	Packet * p;
	poolMutex.Lock();

	while ( pool.size() )
	{
		p = pool.pop();
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

	if ( pool.size() )
		p = pool.pop();

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

