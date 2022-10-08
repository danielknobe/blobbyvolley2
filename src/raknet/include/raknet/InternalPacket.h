/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Define the Structure of an Internal Packet 
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
#ifndef __INTERNAL_PACKET_H
#define __INTERNAL_PACKET_H

#include "PacketPriority.h"
#ifdef _DEBUG
#include "NetworkTypes.h"
#endif
/**
* This must be able to hold the highest value of RECEIVED_PACKET_LOG_LENGTH.
*/
typedef unsigned short PacketNumberType;

/**
* Use unsigned char for most apps.  Unsigned short if you send many packets at the same time on the same ordering stream
* This has to be the same value on all systems
*/
// TODO - Change to unsigned short?
typedef unsigned char OrderingIndexType;

/**
* @brief Structure of an Internal Packet
* 
* Internal packets are used with the RakNet Network Library for internal 
* management only.
*/

struct InternalPacket
{
	/**
	* True if this is an acknowledgment packet
	*/
	bool isAcknowledgement;
	/**
	* The number of this packet, used as an identifier
	*/
	PacketNumberType packetNumber;
	/**
	* The priority level of this packet
	*/
	PacketPriority priority;
	/**
	* What type of reliability algorithm to use with this packet
	*/
	PacketReliability reliability;
	/**
	* What ordering channel this packet is on, if the reliability type uses ordering channels
	*/
	unsigned char orderingChannel;
	/**
	* The ID used as identification for ordering channels
	*/
	OrderingIndexType orderingIndex;
	/**
	* The ID of the split packet, if we have split packets
	*/
	unsigned int splitPacketId;
	/**
	* If this is a split packet, the index into the array of subsplit packets
	*/
	unsigned int splitPacketIndex;
	/**
	* The size of the array of subsplit packets
	*/
	unsigned int splitPacketCount;
	/**
	* When this packet was created
	*/
	unsigned int creationTime;
	/**
	* The next time to take action on this packet
	*/
	unsigned int nextActionTime;
	/**
	* How many bits the data is
	*/
	unsigned int dataBitLength;
	/**
	* Buffer is a pointer to the actual data, assuming this packet has data at all
	*/
	char *data;
};

#endif

