/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file GetTime.cpp 
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
#include "GetTime.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

unsigned int RakNet::GetTime( void )
{
#ifdef _WIN32
	static LARGE_INTEGER yo;
	static LONGLONG counts;
#else
	
	static timeval tp, initialTime;
#endif
	
	static bool initialized = false;
	
	if ( initialized == false )
	{
#ifdef _WIN32
		QueryPerformanceFrequency( &yo );
		// The original code shifted right 10 bits
		//counts = yo.QuadPart >> 10;
		// It gives the wrong value since 2^10 is not 1000
		counts = yo.QuadPart / 1000;
#else
		
		gettimeofday( &initialTime, 0 );
#endif
		
		initialized = true;
	}
	
#ifdef _WIN32
	LARGE_INTEGER PerfVal;
	
	QueryPerformanceCounter( &PerfVal );
	
	return ( unsigned int ) ( PerfVal.QuadPart / counts );
	
#else
	
	gettimeofday( &tp, 0 );
	
	// Seconds to ms and microseconds to ms
	return ( tp.tv_sec - initialTime.tv_sec ) * 1000 + ( tp.tv_usec - initialTime.tv_usec ) / 1000;
	
#endif
}
