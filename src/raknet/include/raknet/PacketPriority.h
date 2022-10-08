/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Defines Priority and Reliability Constants 
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
