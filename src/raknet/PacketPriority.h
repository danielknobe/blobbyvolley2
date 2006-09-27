/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Defines Priority and Reliability Constants 
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

#ifndef __PACKET_PRIORITY_H
#define __PACKET_PRIORITY_H 
/**
 * This enum contains all level of priority that can be applyed to a packet.  
 */
enum PacketPriority
{
	SYSTEM_PRIORITY,   //!< System priority is for system related messaging.  Don't use it.
	HIGH_PRIORITY,   //!< Those message are handle first
	MEDIUM_PRIORITY,   //!< Message relativly important
	LOW_PRIORITY,   //!< Not critical information
	NUMBER_OF_PRIORITIES
};
/**
 * This define the reliability behaviour to apply to a packet
 * 
 * @note  Note to self: I write this with 3 bits in the stream!
 *
 */

enum PacketReliability
{
	UNRELIABLE,   //!< Send packet not reliable and not sequenced
	UNRELIABLE_SEQUENCED,  //!< Send packet not reliable but sequenced
	RELIABLE,   //!< Send packet reliable
	RELIABLE_ORDERED,   //!< Send packet reliable respecting ordering
	RELIABLE_SEQUENCED //!< Send packet reliable respecting sequenced
};

#endif
