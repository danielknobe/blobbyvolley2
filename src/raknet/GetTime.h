/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file GetTime.h
 * This file is part of RakNet Copyright 2003  Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the
 * shareware license found at
 * http://www.rakkarsoft.com/shareWareLicense.html which you agreed to
 * upon purchase of a "Shareware license" "Commercial" Licensees with
 * Rakkarsoft LLC are subject to the commercial license found at
 * http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed
 * to upon purchase of a "Commercial license" All other users are
 * subject to the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Refer to the appropriate license agreement for distribution, modification, and warranty rights.
 */
#ifndef __GET_TIME_H
#define __GET_TIME_H

#ifndef RAKNET_DEPRECATED 
/**
 * Retrieve the current time in seconds. 
 * 
 * @deprecated You should better use RakNet::GetTime(). 
 * It require the definition of the RAKNET_COMPATIBILITY 
 * preprocessor symbol during the compilation of RakNet 
 * to be available. 
 *
 * @see RakNet::GetTime 
 */
#define RakNetGetTime(void) RakNet::GetTime(void)
#endif

namespace RakNet
{
	/**
	 * This function return the current time 
	 * @return The number of seconds from 1970 january the 1st.  
	 */
	unsigned int GetTime( void );
}

#endif

