/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Client Context Internal Structure 
 * @note Windows Port Only 
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
#ifndef __CLIENT_CONTEXT_STRUCT_H
#define __CLIENT_CONTEXT_STRUCT_H

#ifdef _WIN32
#include <windows.h>
#endif
#include "NetworkTypes.h"
#include "MTUSize.h"

class RakPeer;

#ifdef __USE_IO_COMPLETION_PORTS

struct ClientContextStruct
{
	HANDLE handle; // The socket, also used as a file handle
};

struct ExtendedOverlappedStruct
{
	OVERLAPPED overlapped;
	char data[ MAXIMUM_MTU_SIZE ]; // Used to hold data to send
	int length; // Length of the actual data to send, always under MAXIMUM_MTU_SIZE
	unsigned int binaryAddress;
	unsigned short port;
	RakPeer *rakPeer;
	bool read; // Set to true for reads, false for writes
};

#endif

#endif
