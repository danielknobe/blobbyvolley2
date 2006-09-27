/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Provide some consts concerning the size of network packet. 
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
#ifndef DEFAULT_MTU_SIZE

/**
 * 1500. The largest Ethernet packet size; it is also the default
 * value. This is the typical setting for non-PPPoE, non-VPN
 * connections. The default value for NETGEAR routers, adapters and
 * switches.  1492. The size PPPoE prefers.  1472. Maximum size to use
 * for pinging. (Bigger packets are fragmented.)  1468. The size DHCP
 * prefers.  1460. Usable by AOL if you don't have large email
 * attachments, etc.  1430. The size VPN and PPTP prefer.  1400. Maximum
 * size for AOL DSL.  576. Typical value to connect to dial-up ISPs. 
 */
#define DEFAULT_MTU_SIZE 576 
/**
 * This is the largest value for an UDP packet Fixe DEFAULT_MTU_SIZE
 * to a lower value.
 */
#define MAXIMUM_MTU_SIZE 8000
#endif

