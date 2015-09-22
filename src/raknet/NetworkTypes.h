/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file NetworkTypes.h
 * @brief Define Network Common Class and Types.
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

#ifndef __NETWORK_TYPES_H
#define __NETWORK_TYPES_H

#include <string>
#include <boost/shared_ptr.hpp>

namespace RakNet
{
	class BitStream;
}

/**
* Typename for player index
*/
typedef unsigned short PlayerIndex;
/**
* @brief Player Identifier
*
* This define a Player Unique Identifier.
* In fact, it corresponds to the peer address.
*/
struct PlayerID
{
	/**
	* The peer address from inet_addr.
	*/
	unsigned int binaryAddress;
	/**
	* The port number associated to the connexion.
	*/
	unsigned short port;
	/**
	* Copy operator
	* @param input a player ID
	* @return a reference to the current object
	*/
	PlayerID& operator = ( const PlayerID& input )
	{
		binaryAddress = input.binaryAddress;
		port = input.port;
		return *this;
	}

	/**
	* Test if two Player Unique Identifier are the same
	* @param left a Player Unique Identifier
	* @param right a Player Unique Identifier
	* @return 1 if left and right corresponds to the same player 0 otherwise
	*/
	friend int operator==( const PlayerID& left, const PlayerID& right );
	/**
	* Test if two Player Unique Identifier differs
	* @param left a Player Unique Identifier
	* @param right a Player Unique Identifier
	* @return 0 if left and right corresponds to the same player 1 otherwise
	*/
	friend int operator!=( const PlayerID& left, const PlayerID& right );
	/**
	* Test if left is greater than right
	* @param left a Player Unique Identifier
	* @param right a Player Unique Identifier
	* @return 1 if left is greater than right, 0 otherwise
	*/
	friend int operator > ( const PlayerID& left, const PlayerID& right );
	/**
	* Test if left is lesser than right
	* @param left a Player Unique Identifier
	* @param right a Player Unique Identifier
	* @return 1 if left is lesser than right, 0 otherwise
	*/
	friend int operator < ( const PlayerID& left, const PlayerID& right );

	/**
	* Converts the playerID into a readable string representation
	*/
	std::string toString() const;
};

/// default implementation of printing a player ID to an ostream.
std::ostream& operator<<(std::ostream& stream, const PlayerID& p);

/// Size of PlayerID data
#define PlayerID_Size 6

/**
* @brief Network Packet
*
* This structure store information concerning
* a packet going throught the network
*/

struct Packet
{
	/**
	* Server only - this is the index into the player array that this playerId maps to
	*/
	PlayerIndex playerIndex;
	/**
	* The Player Unique Identifier that send this packet.
	*/
	PlayerID playerId;
	/**
	* The length of the data.
	* @deprecated You should use bitSize inplace.
	*
	*/
	unsigned int length;
	/**
	* The number of bits in used.
	* Same as length but represents bits. Length is obsolete and retained for backwards compatibility.
	*/
	unsigned int bitSize;
	/**
	* The byte array.
	* The standard behaviour in RakNet define the first byte of the data array as the packet class.
	* @see PacketEnumerations.h
	*/
	unsigned char* data;


	/**
	* Converts the contents of this packet into a bitstream. Does not perform a deep copy of the packet data!
	* Use stream only as long as packet exists.
	*/
	RakNet::BitStream getStream() const;
};

typedef boost::shared_ptr<Packet> packet_ptr;

/**
*  Index of an unassigned player
*/
const PlayerIndex UNASSIGNED_PLAYER_INDEX = 65535;

/**
* Index of an invalid Player Unique Id
*/
const PlayerID UNASSIGNED_PLAYER_ID =
{
	0xFFFFFFFF, 0xFFFF
};

/**
* Sizeof the Ping Array
*/
const int PING_TIMES_ARRAY_SIZE = 5;

#endif

