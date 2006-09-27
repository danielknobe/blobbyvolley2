/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file ExtendedOverlappedPool.cpp 
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
#ifdef __USE_IO_COMPLETION_PORTS
#include "ExtendedOverlappedPool.h"

ExtendedOverlappedPool ExtendedOverlappedPool::I;

ExtendedOverlappedPool::ExtendedOverlappedPool()
{}

ExtendedOverlappedPool::~ExtendedOverlappedPool()
{
	// The caller better have returned all the packets!
	ExtendedOverlappedStruct * p;
	poolMutex.Lock();
	
	while ( pool.size() )
	{
		p = pool.pop();
		delete p;
	}
	
	poolMutex.Unlock();
}

ExtendedOverlappedStruct* ExtendedOverlappedPool::GetPointer( void )
{
	ExtendedOverlappedStruct * p = 0;
	poolMutex.Lock();
	
	if ( pool.size() )
		p = pool.pop();
		
	poolMutex.Unlock();
	
	if ( p )
		return p;
		
	return new ExtendedOverlappedStruct;
}

void ExtendedOverlappedPool::ReleasePointer( ExtendedOverlappedStruct *p )
{
	poolMutex.Lock();
	pool.push( p );
	poolMutex.Unlock();
}

#endif

