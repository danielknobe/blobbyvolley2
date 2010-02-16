/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 *  
 * @brief This file is used on win32 ports to benefits the use of IO Completion ports. 
 * 
 * @bug Currently IO Completion ports does not work correctly.
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
