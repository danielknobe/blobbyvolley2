/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Simple Mutex
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

#include "SimpleMutex.h"
#include <assert.h>
////#include "MemoryManager.h"

SimpleMutex::SimpleMutex()
{
#ifdef _WIN32
	//	hMutex = CreateMutex(NULL, FALSE, 0);
	//	assert(hMutex);
	InitializeCriticalSection(&criticalSection);
#else
	int error = pthread_mutex_init(&hMutex, 0);
	assert(error==0);
#endif
}

SimpleMutex::~SimpleMutex()
{
#ifdef _WIN32
	//	CloseHandle(hMutex);
	DeleteCriticalSection(&criticalSection);
#else
	pthread_mutex_destroy(&hMutex);
#endif
}

#ifdef _WIN32
#ifdef _DEBUG
#include <stdio.h>
#endif
#endif

void SimpleMutex::Lock(void)
{
#ifdef _WIN32
	/*
	DWORD d = WaitForSingleObject(hMutex, INFINITE);
	#ifdef _DEBUG
	if (d==WAIT_FAILED)
	{
	LPVOID messageBuffer;
	FormatMessage( 
	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	FORMAT_MESSAGE_FROM_SYSTEM | 
	FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
	GetLastError(),
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	(LPTSTR) &messageBuffer,
	0,
	NULL 
	);
	// Process any inserts in messageBuffer.
	// ...
	// Display the string.
	//MessageBox( NULL, (LPCTSTR)messageBuffer, "Error", MB_OK | MB_ICONINFORMATION );
	printf("SimpleMutex error: %s", messageBuffer);
	// Free the buffer.
	LocalFree( messageBuffer );

	}

	assert(d==WAIT_OBJECT_0);
	*/
	EnterCriticalSection(&criticalSection);

#else
	int error = pthread_mutex_lock(&hMutex);
	assert(error==0);
#endif
}

void SimpleMutex::Unlock(void)
{
#ifdef _WIN32
	//	ReleaseMutex(hMutex);
	LeaveCriticalSection(&criticalSection);
#else
	int error = pthread_mutex_unlock(&hMutex);
	assert(error==0);
#endif
}

