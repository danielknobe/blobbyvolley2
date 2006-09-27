/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file NetworkTypes.h
* @brief Define Network Common Class and Types. 
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
* Typename for encoding RPC indices
*/
typedef unsigned char RPCIndex;
/**
* Maximum allowed registered RPC functions.
*/
const int MAX_RPC_MAP_SIZE=((RPCIndex)-1)-1;
/**
* Indicates an undefined index for an RPC,
*/
const int UNDEFINED_RPC_INDEX=((RPCIndex)-1);
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

// Sorry for changing the RPC parameters to a struct but this way I can change it in the future without making people change their code anymore.
struct RPCParameters
{
	char *input;
	unsigned int numberOfBitsOfData;
	PlayerID sender;
	RakPeerInterface *recipient;
};

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

/**
* @defgroup RAKNET_RPC Remote Procedure Call Subsystem. 
* @brief RPC Function Implementation 
* 
* The Remote Procedure Call Subsystem provide the RPC paradigm to
* RakNet user. It consists in providing remote function call over the
* network.  A call to a remote function require you to prepare the
* data for each parameter (using BitStream) for example. 
* 
*
* Use the following C function prototype for your callbacks
* @code
* void functionName(RPCParameters *rpcParms);
* @encode 
* If you pass input data, you can parse the input data in two ways.
*
* 1.
* Cast input to a struct (such as if you sent a struct)
* i.e. MyStruct *s = (MyStruct*) input;
* Make sure that the sizeof(MyStruct) is equal to the number of bytes passed!
*
* 2.
* Create a BitStream instance with input as data and the number of bytes
* i.e. BitStream myBitStream(input, (numberOfBitsOfData-1)/8+1)
*
* (numberOfBitsOfData-1)/8+1 is how convert from bits to bytes
*
* Full example:
* @code
* void MyFunc(RPCParameters *rpcParms) {}
* RakClient *rakClient;
* REGISTER_AS_REMOTE_PROCEDURE_CALL(rakClient, MyFunc);
* This would allow MyFunc to be called from the server using  (for example)
* rakServer->RPC("MyFunc", 0, clientID, false);
* @endocde
*/

/**
* @def REGISTER_STATIC_RPC
* @ingroup RAKNET_RPC
* Register a C function as a Remote procedure. 
* @param networkObject The object that will handle the remote procedure call 
* @param functionName The name of the function 
*/
// 12/01/05 REGISTER_AS_REMOTE_PROCEDURE_CALL renamed to REGISTER_STATIC_RPC.  Delete the old name sometime in the future
#pragma deprecated(REGISTER_AS_REMOTE_PROCEDURE_CALL)
#define REGISTER_AS_REMOTE_PROCEDURE_CALL(networkObject, functionName) REGISTER_STATIC_RPC(networkObject, functionName)
#define REGISTER_STATIC_RPC(networkObject, functionName) (networkObject)->RegisterAsRemoteProcedureCall((#functionName),(functionName))
/**
* @def REGISTER_CLASS_INST_RPC
* @ingroup RAKNET_RPC
* Register a member function of an instantiated object as a Remote procedure call.
* RPC member Functions MUST be marked __cdecl!  See the ObjectMemberRPC example.
* CLASS_MEMBER_ID is a utility macro to generate a unique signature for a class and function pair and can be used for the Raknet functions RegisterClassMemberRPC(...) and RPC(...)
* REGISTER_CLASS_MEMBER_RPC is a utility macro to more easily call RegisterClassMemberRPC
* @param networkObject The object that will handle the remote procedure call 
* @param className The class containing the function
* @param functionName The name of the function 
*/
#define CLASS_MEMBER_ID(className, functionName) #className "_" #functionName
#define REGISTER_CLASS_MEMBER_RPC(networkObject, className, functionName) {union {void (__cdecl className::*cFunc)( RPCParameters *rpcParms ); void* voidFunc;}; cFunc=&className::functionName; networkObject->RegisterClassMemberRPC(CLASS_MEMBER_ID(className, functionName),voidFunc);}

/**
* @def UNREGISTER_STATIC_RPC 
* @ingroup RAKNET_RPC
* Unregisters a remote procedure call
* RPC member Functions MUST be marked __cdecl!  See the ObjectMemberRPC example.
* @param networkObject The object that manages the function 
* @param functionName The function name 
*/
// 12/01/05 UNREGISTER_AS_REMOTE_PROCEDURE_CALL Renamed to UNREGISTER_STATIC_RPC.  Delete the old name sometime in the future
#pragma deprecated(UNREGISTER_AS_REMOTE_PROCEDURE_CALL)
#define UNREGISTER_AS_REMOTE_PROCEDURE_CALL(networkObject,functionName) UNREGISTER_STATIC_RPC(networkObject,functionName)
#define UNREGISTER_STATIC_RPC(networkObject,functionName) (networkObject)->UnregisterAsRemoteProcedureCall((#functionName))
/**
* @def UNREGISTER_CLASS_INST_RPC 
* @ingroup RAKNET_RPC
* Unregisters a member function of an instantiated object as a Remote procedure call.
* @param networkObject The object that manages the function 
* @param className The className that was originally passed to REGISTER_AS_REMOTE_PROCEDURE_CALL
* @param functionName The function name 
*/
#define UNREGISTER_CLASS_MEMBER_RPC(networkObject, className, functionName) (networkObject)->UnregisterAsRemoteProcedureCall((#className "_" #functionName))

#endif

