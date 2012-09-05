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

// Needed for Serialize/Deserialize functions
#include "BitStream.h"

/**
* Typename for Network Object Identifier 
*/
typedef unsigned short ObjectID;
/**
* Typename for Unique Id 
*/
typedef unsigned char UniqueIDType;
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
};
/// Size of PlayerID data
#define PlayerID_Size 6


/**
* @brief Connection request handling 
* 
* This structure is used internally to store the connection request 
* @internal
*/

//struct RequestedConnectionStruct
//{
	/**
	* Who we wanted to connect to.
	*/
//	PlayerID playerId;
	/**
	* When will we requested this connection. 
	*/
//	unsigned int time;
	/**
	* Security key 
	*/
//	unsigned char AESKey[ 16 ];
	/**
	* true if security policy are enabled 
	*/
//	bool setAESKey;
	/**
	* Next time we will try to connect 
	*/
//	unsigned int nextRequestTime;
	/**
	* How important this request is.  This is used two systems try to connect to each other at the same time - so only one request goes through
	*/
//	unsigned int priority;
//};

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
};

class RakPeerInterface;

//#pragma pack(push,1)

//#pragma pack(1) 
/**
* @brief Store Accepted Connection 
*  
* Handle active connection. 
* @internal 
*/

/*
struct ConnectionAcceptStruct
{
	unsigned char typeId;
	unsigned short remotePort;
	PlayerID externalID;
	PlayerIndex playerIndex;
	void Serialize(RakNet::BitStream &bstream)
	{
		bstream.Write(typeId);
		bstream.Write(remotePort);
		bstream.Write(externalID.binaryAddress);
		bstream.Write(externalID.port);
		bstream.Write(playerIndex);
	}
	void Deserialize(RakNet::BitStream &bstream)
	{
		bstream.Read(typeId);
		bstream.Read(remotePort);
		bstream.Read(externalID.binaryAddress);
		bstream.Read(externalID.port);
		bstream.Read(playerIndex);
	}
};
*/
/** Size of ConnectionAcceptStruct data.
1:  unsigned char typeId;
2:  unsigned short remotePort;
PlayerID externalID
4:  unsigned int binaryAddress
2:  unsigned short port
2:  PlayerIndex playerIndex
Total: 11
*/
//#define ConnectionAcceptStruct_Size 11

/*
#pragma pack(1)

struct PingStruct
{
	unsigned char typeId;
	unsigned int sendPingTime;
	unsigned int sendPongTime;
	void Serialize(RakNet::BitStream &bstream)
	{
		bstream.Write(typeId);
		bstream.Write(sendPingTime);
		bstream.Write(sendPongTime);
	}
	void Deserialize(RakNet::BitStream &bstream)
	{
		bstream.Read(typeId);
		bstream.Read(sendPingTime);
		bstream.Read(sendPongTime);
	}
};
*/
/**
Size of PingStruct data
1:  unsigned char typeId;
4:  unsigned int sendPingTime;
4:  unsigned int sendPongTime;
Total: 9
*/
//#define PingStruct_Size 9

//#pragma pack(1) 
/**
* @brief Store Unconnected ping informations 
* 
* @internal 
*/
/*
struct UnconnectedPingStruct
{
	unsigned char typeId;
	unsigned int sendPingTime;
	void Serialize(RakNet::BitStream &bstream)
	{
		bstream.Write(typeId);
		bstream.Write(sendPingTime);
	}
	void Deserialize(RakNet::BitStream &bstream)
	{
		bstream.Read(typeId);
		bstream.Read(sendPingTime);
	}
};
*/
/// Size of UnconnectedPingStruct data
//#define UnconnectedPingStruct_Size 5

//#pragma pack(1) 
/*
struct SetRandomNumberSeedStruct
{
	unsigned char ts;
	unsigned int timeStamp;
	unsigned char typeId;
	unsigned int seed;
	unsigned int nextSeed;
	void Serialize(RakNet::BitStream &bstream)
	{
		bstream.Write(ts);
		bstream.Write(timeStamp);
		bstream.Write(typeId);
		bstream.Write(seed);
		bstream.Write(nextSeed);
	}
	void Deserialize(RakNet::BitStream &bstream)
	{
		bstream.Write(ts);
		bstream.Write(timeStamp);
		bstream.Write(typeId);
		bstream.Write(seed);
		bstream.Write(nextSeed);
	}
};
*/
//#define SetRandomNumberSeedStruct_Size 14

//#pragma pack(1) 
/*
struct NewIncomingConnectionStruct
{
	unsigned char typeId;
	PlayerID externalID;
	void Serialize(RakNet::BitStream &bstream)
	{
		bstream.Write(typeId);
		bstream.Write(externalID.binaryAddress);
		bstream.Write(externalID.port);
	}
	void Deserialize(RakNet::BitStream &bstream)
	{
		bstream.Read(typeId);
		bstream.Read(externalID.binaryAddress);
		bstream.Read(externalID.port);
	}
};

*/
/*
#define NewIncomingConnectionStruct_Size 7

#pragma pack(pop)
*/

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
* Invalid Object Unique Id 
*/
const ObjectID UNASSIGNED_OBJECT_ID = 65535;
/**
* Sizeof the Ping Array 
*/
const int PING_TIMES_ARRAY_SIZE = 5;

#endif

