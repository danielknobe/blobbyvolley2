/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 *  
 * @brief This file is used on win32 ports to benefits the use of IO Completion ports. 
 * 
 * @bug Currently IO Completion ports does not work correctly.
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

#ifndef __ASYNCHRONOUS_FILE_IO_H
#define __ASYNCHRONOUS_FILE_IO_H

#ifdef _WIN32
#ifdef __USE_IO_COMPLETION_PORTS
#include <WinSock2.h>
#else
#include <WinSock.h>
#endif
#include <windows.h>
#endif
#include "SimpleMutex.h"

struct ExtendedOverlappedStruct;

/**
 * This class provide Asynchronous IO mecanism
 */

class AsynchronousFileIO
{

public:
	/**
	 * Default Constructor 
	 */
	AsynchronousFileIO();
	/**
	 * Destructor 
	 */
	~AsynchronousFileIO();
	
	/**
	 * Associate a socket to a port 
	 * @param socket the socket used for communication 
	 * @param dwCompletionKey the completion port key 
	 */
	bool AssociateSocketWithCompletionPort( SOCKET socket, DWORD dwCompletionKey );
#endif
	/**
	 * Singleton pattern. 
	 * Retrieve the unique instance 
	 */
	static inline AsynchronousFileIO* Instance()
	{
		return & I;
	}
	
	/**
	 * Increase the number of user of the instance 
	 */
	void IncreaseUserCount( void );
	/**
	 * Decrease the number of user of the instance 
	 */
	void DecreaseUserCount( void );
	/**
	 * Stop using asynchronous IO 
	 */
	void Shutdown( void );
	/**
	 * Get the number of user of the instance 
	 */
	int GetUserCount( void );
	
	/**
	 * @todo Document this member. 
	 *
	 *
	 */
	unsigned threadCount;
	/**
	 * @todo Document this member. 
	 *
	 *
	 */
	bool killThreads;
	
private:
	HANDLE completionPort;
	SimpleMutex userCountMutex;
	SYSTEM_INFO systemInfo;
	int userCount;
	
	static AsynchronousFileIO I;
};

unsigned __stdcall ThreadPoolFunc( LPVOID arguments );

void WriteAsynch( HANDLE handle, ExtendedOverlappedStruct *extended );

BOOL ReadAsynch( HANDLE handle, ExtendedOverlappedStruct *extended );

#endif
