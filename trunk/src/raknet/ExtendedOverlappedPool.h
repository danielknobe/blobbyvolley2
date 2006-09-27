/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file ExtendedOverlappedPool.h
 *  
 * This file is part of RakNet Copyright 2003  Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the shareware license found at http://www.rakkarsoft.com/shareWareLicense.html which you agreed to upon purchase of a "Shareware license"
 * "Commercial" Licensees with Rakkarsoft LLC are subject to the commercial license found at http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed to upon purchase of a "Commercial license"
 * All other users are subject to the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 *
 * Refer to the appropriate license agreement for distribution, modification, and warranty rights.
 */
#ifdef __USE_IO_COMPLETION_PORTS
#ifndef __EXTENDED_OVERLAPPED_POOL
#define __EXTENDED_OVERLAPPED_POOL
#include "SimpleMutex.h"
#include "ClientContextStruct.h"
#include "RakNetQueue.h"

/**
 * @internal 
 * @todo Document this class  
 */

class ExtendedOverlappedPool
{

public:
	ExtendedOverlappedPool();
	~ExtendedOverlappedPool();
	ExtendedOverlappedStruct* GetPointer( void );
	void ReleasePointer( ExtendedOverlappedStruct *p );
	static inline ExtendedOverlappedPool* Instance()
	{
		return & I;
	}
	
private:
	BasicDataStructures::Queue<ExtendedOverlappedStruct*> pool;
	SimpleMutex poolMutex;
	static ExtendedOverlappedPool I;
};

#endif
#endif

