/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file 
* @brief SimpleMutex class implementation 
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

#ifndef __SIMPLE_MUTEX_H
#define __SIMPLE_MUTEX_H

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sys/types.h>
#endif

class SimpleMutex
{
public:
	SimpleMutex();
	~SimpleMutex();
	void Lock(void);
	void Unlock(void);
private:
	#ifdef _WIN32
	//HANDLE hMutex;
	CRITICAL_SECTION criticalSection; // Docs say this is faster than a mutex for single process access
	#else
	pthread_mutex_t hMutex;
	#endif
};

#endif

