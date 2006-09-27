/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_DNO 
 * @file EncodeClassName.h 
 * @brief Class Name encoding for DNO subsystem. 
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
#ifndef __ENCODE_CLASS_NAME
#define __ENCODE_CLASS_NAME 1
#define MAXIMUM_CLASS_IDENTIFIER_LENGTH 20

/**
 * @ingroup RAKNET_DNO 
 * Build the encoded class name.  
 * @param name the name 
 * @param identifier 
 */
void EncodeClassName( char *name, char *identifier );
#endif
