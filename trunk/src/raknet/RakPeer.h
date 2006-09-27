/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file
* @brief A RakPeer is the lower level Communication End Point. 
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
#ifndef __RAK_PEER_H
#define __RAK_PEER_H

#include "ReliabilityLayer.h"
#include "RakPeerInterface.h"
#include "RPCNode.h"
#include "RSACrypt.h"
#include "BitStream.h"
#include "SingleProducerConsumer.h"
#include "RPCMap.h"
#include "PacketPool.h"

class HuffmanEncodingTree;
class MessageHandlerInterface;

#ifdef _WIN32
// unsigned __stdcall RecvFromNetworkLoop(LPVOID arguments);
void __stdcall ProcessPortUnreachable( unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer );
void __stdcall ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
unsigned __stdcall UpdateNetworkLoop( LPVOID arguments );
#else
// void*  RecvFromNetworkLoop( void*  arguments );
void ProcessPortUnreachable( unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer );
void ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
void* UpdateNetworkLoop( void* arguments );
#endif


/**
* @brief The lowest communication end point in RakNet.
*
* This class provide the lowest communication end point in RakNet.
* It is recommended that you use it if you are going to be at the
* same time client and server.
*/

class RakPeer : public RakPeerInterface
{

public:
	/**
	* Constructor
	*/
	RakPeer();
	/**
	* Destructor
	*/
	virtual ~RakPeer();

	/*
	* --------------------------------------------------------------------------------------------
	* Major Low Level Functions - Functions needed by most users
	* --------------------------------------------------------------------------------------------
	*/
	/**
	* Starts the network threads, opens the listen port
	* You must call this before calling SetMaximumIncomingConnections or Connect
	* Multiple calls while already active are ignored.  To call this function again with different settings, you must first call Disconnect()
	* To accept incoming connections, use SetMaximumIncomingConnections
	*
	* Parameters:
	* @param MaximumNumberOfPeers Required so the network can preallocate and for thread safety.
	* - A pure client would set this to 1.  A pure server would set it to the number of allowed clients.
	* - A hybrid would set it to the sum of both types of connections
	* @param localPort The port to listen for connections on.
	* @param _threadSleepTimer >=0 for how many ms to Sleep each internal update cycle (recommended 30 for low performance, 0 for regular)
	* @param forceHostAddress Can force RakNet to use a particular IP to host on.  Pass 0 to automatically pick an IP
	*
	* @return False on failure (can't create socket or thread), true on success.
	*/
	bool Initialize( unsigned short MaximumNumberOfPeers, unsigned short localPort, int _threadSleepTimer, const char *forceHostAddress=0 );
	/**
	* Must be called while offline
	* Secures connections though a combination of SHA1, AES128, SYN Cookies, and RSA to prevent
	* connection spoofing, replay attacks, data eavesdropping, packet tampering, and MitM attacks.
	* There is a significant amount of processing and a slight amount of bandwidth
	* overhead for this feature.
	*
	* If you accept connections, you must call this or else secure connections will not be enabled
	* for incoming connections.
	* If you are connecting to another system, you can call this with values for the
	* (e and p,q) public keys before connecting to prevent MitM
	*
	* @param pubKeyE A pointer to the public keys from the RSACrypt class.  
	* @param pubKeyN A pointer to the public keys from the RSACrypt class. 
	* @param privKeyP Public key generated from the RSACrypt class.  
	* @param privKeyQ Public key generated from the RSACrypt class.  
	* If the private keys are 0, then a new key will be generated when this function is called
	* 
	* @see the Encryption sample
	*/
	void InitializeSecurity(const char *pubKeyE, const char *pubKeyN, const char *privKeyP, const char *privKeyQ );
	/**
	* Disables all security.
	* @note Must be called while offline
	*
	*/
	virtual void DisableSecurity( void );
	/**
	* Sets how many incoming connections are allowed. If this is less than the number of players currently connected, no
	* more players will be allowed to connect.  If this is greater than the maximum number of peers allowed, it will be reduced
	* to the maximum number of peers allowed.  Defaults to 0.
	*
	* @param numberAllowed Maximum number of incoming connections allowed.
	*/
	void SetMaximumIncomingConnections( unsigned short numberAllowed );
	/**
	* Get the number of maximum incoming connection. 
	* @return the maximum number of incoming connections, which is always <= MaximumNumberOfPeers
	*/
	unsigned short GetMaximumIncomingConnections( void ) const;
	/**
	* Sets the password incoming connections must match in the call to Connect (defaults to none)
	* Pass 0 to passwordData to specify no password
	*
	* @param passwordData A data block that incoming connections must match.  This can be just a password, or can be a stream of data.
	* Specify 0 for no password data
	* @param passwordDataLength The length in bytes of passwordData
	*/
	void SetIncomingPassword( const char* passwordData, int passwordDataLength );
	/**
	* Get the password set by SetIncomingPassword in a BitStream
	* @return The password in a BitStream. 
	*/
	RakNet::BitStream *GetIncomingPassword( void );
	/**
	* Call this to connect to the specified host (ip or domain name) and server port.
	* Calling Connect and not calling SetMaximumIncomingConnections acts as a dedicated client.  Calling both acts as a true peer.
	* This is a non-blocking connection.  You know the connection is successful when IsConnected() returns true
	* or receive gets a packet with the type identifier ID_CONNECTION_ACCEPTED.  If the connection is not
	* successful, such as rejected connection or no response then neither of these things will happen.
	* Requires that you first call Initialize
	*
	* @param host Either a dotted IP address or a domain name
	* @param remotePort Which port to connect to on the remote machine.
	* @param passwordData A data block that must match the data block on the server.  This can be just a password, or can be a stream of data
	* @param passwordDataLength The length in bytes of passwordData
	*
	* @return True on successful initiation. False on incorrect parameters, internal error, or too many existing peers
	*/
	bool Connect( const char* host, unsigned short remotePort, char* passwordData, int passwordDataLength );
	/**
	* Stops the network threads and close all connections.  Multiple calls are ok.
	*
	*
	* @param blockDuration How long you should wait for all remaining packets to go out, per connected system
	* If you set it to 0 then the disconnection notification probably won't arrive
	*/
	virtual void Disconnect( unsigned int blockDuration );
	/**
	* Returns true if the network threads are running
	*/
	bool IsActive( void ) const;

	/**
	* Fills the array remoteSystems with the playerID of all the systems we are connected to
	*
	* @param[out] remoteSystems  An array of PlayerID structures to be filled with the PlayerIDs of the systems we are connected to
	* - pass 0 to remoteSystems to only get the number of systems we are connected to
	* @param numberOfSystems As input, the size of remoteSystems array.  As output, the number of elements put into the array 
	*/
	bool GetConnectionList( PlayerID *remoteSystems, unsigned short *numberOfSystems ) const;

	/**
	* Sends a block of data to the specified system that you are connected to.
	* This function only works while the client is connected (Use the Connect function).
	*
	* @param data The block of data to send
	* @param length The size in bytes of the data to send
	* @param priority What priority level to send on.
	* @param reliability How reliability to send this data
	* @param orderingChannel When using ordered or sequenced packets, what channel to order these on.
	* - Packets are only ordered relative to other packets on the same stream
	* @param playerId Who to send this packet to, or in the case of broadcasting who not to send it to.  Use UNASSIGNED_PLAYER_ID to specify none
	* @param broadcast True to send this packet to all connected systems. If true, then playerId specifies who not to send the packet to.
	* @return 
	* False if we are not connected to the specified recipient.  True otherwise
	*/
	bool Send( const char *data, const long length, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast );
	/**
	* Sends a block of data to the specified system that you are connected to.
	* This function only works while the client is connected (Use the Connect function).
	*
	* @param bitStream The bitstream to send
	* @param priority What priority level to send on.
	* @param reliability How reliability to send this data
	* @param orderingChannel When using ordered or sequenced packets, what channel to order these on.
	* - Packets are only ordered relative to other packets on the same stream
	* @param playerId Who to send this packet to, or in the case of broadcasting who not to send it to.  Use UNASSIGNED_PLAYER_ID to specify none
	* @param broadcast True to send this packet to all connected systems. If true, then playerId specifies who not to send the packet to.
	* @return 
	* False if we are not connected to the specified recipient.  True otherwise
	*/
	bool Send( RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast );

	/**
	* Gets a packet from the incoming packet queue. Use DeallocatePacket to deallocate the packet after you are done with it.
	* Check the Packet struct at the top of CoreNetworkStructures.h for the format of the struct
	*
	* @return
	* 0 if no packets are waiting to be handled, otherwise an allocated packet
	* If the client is not active this will also return 0, as all waiting packets are flushed when the client is Disconnected
	* This also updates all memory blocks associated with synchronized memory and distributed objects
	*/
	Packet* Receive( void );

	/**
	* Call this to deallocate a packet returned by Receive when you are done handling it.
	* @param packet A packet to free
	*/
	void DeallocatePacket( Packet *packet );

	/**
	* Return the total number of connections we are allowed
	*/
	unsigned short GetMaximumNumberOfPeers( void ) const;
	/*
	* --------------------------------------------------------------------------------------------
	* Remote Procedure Call Functions - Functions to initialize and perform RPC
	* --------------------------------------------------------------------------------------------
	*/
	/**
	* @ingroup RAKNET_RPC
	* Register a C or static member function as available for calling as a remote procedure call.
	* 
	* @param uniqueID: A null terminated non-case sensitive string of only letters to identify this procedure
	* @param functionPointer(...): The name of the function to be used as a function pointer
	* This can be called whether the client is active or not, and registered functions stay registered unless unregistered with
	* UnregisterAsRemoteProcedureCall
	* @note This is part of the Remote Procedure Call Subsystem 
	*
	*/
	void RegisterAsRemoteProcedureCall( char* uniqueID, void ( *functionPointer ) ( RPCParameters *rpcParms ) );

	/**
	* @ingroup RAKNET_RPC
	* Register a C++ member function as available for calling as a remote procedure call.
	* 
	* @param uniqueID: A null terminated string to identify this procedure.
	* Recommended you use the macro REGISTER_CLASS_MEMBER_RPC
	* @param functionPointer: The name of the function to be used as a function pointer
	* This can be called whether the client is active or not, and registered functions stay registered unless unregistered with
	* UnregisterAsRemoteProcedureCall
	* See the ObjectMemberRPC sample for notes on how to call this and setup the member function
	* @note This is part of the Remote Procedure Call Subsystem 
	*
	*/
	void RegisterClassMemberRPC( char* uniqueID, void *functionPointer );

	/**
	* @ingroup RAKNET_RPC
	* Unregisters a C function as available for calling as a remote procedure call that was formerly registered
	* with RegisterAsRemoteProcedureCall
	* Only call offline
	*
	* @param uniqueID A string of only letters to identify this procedure.  Recommended you use the macro CLASS_MEMBER_ID for class member functions.  Must match the parameter
	* passed to RegisterAsRemoteProcedureCall
	*
	* @note This is part of the Remote Procedure Call Subsystem 
	*
	*/
	void UnregisterAsRemoteProcedureCall( char* uniqueID );

	/**
	* @ingroup RAKNET_RPC 
	* Calls a C function on the server that the server already registered using RegisterAsRemoteProcedureCall
	* If you want that function to return data you should call RPC from that system in the same way
	* Returns true on a successful packet send (this does not indicate the recipient performed the call), false on failure
	*
	* @param uniqueID A string of only letters to identify this procedure.  Recommended you use the macro CLASS_MEMBER_ID for class member functions.  Must match the parameter
	* @param data The block of data to send
	* @param bitLength The size in BITS of the data to send
	* @param priority What priority level to send on.
	* @param reliability How reliability to send this data
	* @param orderingChannel When using ordered or sequenced packets, what channel to order these on.
	* @param playerId Who to send this packet to, or in the case of broadcasting who not to send it to.  Use UNASSIGNED_PLAYER_ID to specify none
	* @param broadcast True to send this packet to all connected systems. If true, then playerId specifies who not to send the packet to.
	* @param shiftTimestamp True to treat the first 4 bytes as a timestamp and make it system relative on arrival (Same as ID_TIMESTAMP for a packet enumeration type)
	* @param objectID For static functions, pass UNASSIGNED_OBJECT_ID.  For member functions, you must derive from NetworkIDGenerator and pass the value returned by NetworkIDGenerator::GetNetworkID for that object.
	* @return True on a successful packet send (this does not indicate the recipient performed the call), false on failure
	* @note This is part of the Remote Procedure Call Subsystem 
	*/
	bool RPC( char* uniqueID, const char *data, unsigned int bitLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool shiftTimestamp, ObjectID objectID );
	/**
	* @ingroup RAKNET_RPC 
	* Calls a C function on the server that the server already registered using RegisterAsRemoteProcedureCall
	* If you want that function to return data you should call RPC from that system in the same way
	* Returns true on a successful packet send (this does not indicate the recipient performed the call), false on failure
	*
	* @param uniqueID A string of only letters to identify this procedure.  Recommended you use the macro CLASS_MEMBER_ID for class member functions.  Must match the parameter
	* @param bitStream The bitstream to send

	* @param priority What priority level to send on.
	* @param reliability How reliability to send this data
	* @param orderingChannel When using ordered or sequenced packets, what channel to order these on.
	* @param playerId Who to send this packet to, or in the case of broadcasting who not to send it to.  Use UNASSIGNED_PLAYER_ID to specify none
	* @param broadcast True to send this packet to all connected systems. If true, then playerId specifies who not to send the packet to.
	* @param shiftTimestamp True to treat the first 4 bytes as a timestamp and make it system relative on arrival (Same as ID_TIMESTAMP for a packet enumeration type)
	* @param objectID For static functions, pass UNASSIGNED_OBJECT_ID.  For member functions, you must derive from NetworkIDGenerator and pass the value returned by NetworkIDGenerator::GetNetworkID for that object.
	* @return True on a successful packet send (this does not indicate the recipient performed the call), false on failure
	* @note This is part of the Remote Procedure Call Subsystem 
	*/
	bool RPC( char* uniqueID, RakNet::BitStream *bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool shiftTimestamp, ObjectID objectID );
	/*
	* --------------------------------------------------------------------------------------------
	* Player Management Functions
	* --------------------------------------------------------------------------------------------
	*/

	/**
	* Close the connection to another host (if we initiated the connection it will disconnect, if they did it will kick them out).
	*
	* @param target Which connection to close
	* @param sendDisconnectionNotification True to send ID_DISCONNECTION_NOTIFICATION to the recipient.  False to close it silently.
	*/
	void CloseConnection( PlayerID target, bool sendDisconnectionNotification, int depreciated=0 );

	/**
	* Given a playerID, returns an index from 0 to the maximum number of players allowed - 1.
	*
	* @param playerId The playerID to search for
	*
	* @return An integer from 0 to the maximum number of peers -1, or -1 if that player is not found
	*/
	int GetIndexFromPlayerID( PlayerID playerId );

	/**
	* This function is only useful for looping through all players.
	*
	* @param index  an integer between 0 and the maximum number of players allowed - 1.
	*
	* @return A valid playerID or UNASSIGNED_PLAYER_ID if no such player at that index
	*/
	PlayerID GetPlayerIDFromIndex( int index );

	/**
	* Bans an IP from connecting. Banned IPs persist between connections.
	*
	* @param IP Dotted IP address.  Can use * as a wildcard, such as 128.0.0.* will ban
	* All IP addresses starting with 128.0.0
	* @param milliseconds - how many ms for a temporary ban.  Use 0 for a permanent ban
	*/
	void AddToBanList( const char *IP, unsigned int milliseconds=0 );

	/**
	* Allows a previously banned IP to connect.
	*
	* @param IP Dotted IP address.  Can use * as a wildcard, such as 128.0.0.* will unban
	* All IP addresses starting with 128.0.0 
	*/
	void RemoveFromBanList( const char *IP );

	/**
	* Allows all previously banned IPs to connect.
	*/
	void ClearBanList( void );

	/**
	* Determines if a particular IP is banned.
	*
	* @param IP Complete dotted IP address
	*
	* @return
	* - True if IP matches any IPs in the ban list, accounting for any wildcards.
	* - False otherwise.
	*/
	bool IsBanned( const char *IP );
	/*
	* --------------------------------------------------------------------------------------------
	* Pinging Functions - Functions dealing with the automatic ping mechanism
	* --------------------------------------------------------------------------------------------
	*/

	/**
	* Send a ping to the specified connected system.
	*
	* Requires:
	* The sender and recipient must already be started via a successful call to Initialize
	*
	* @param target  who to ping
	*/
	void Ping( PlayerID target );

	/**
	* Send a ping to the specified unconnected system.
	* The remote system, if it is Initialized, will respond with ID_PONG.
	* The final ping time will be encoded in the following 4 bytes (2-5) as an unsigned int
	*
	* Requires:
	* The sender and recipient must already be started via a successful call to Initialize
	*
	* 
	* @param host Either a dotted IP address or a domain name.  Can be 255.255.255.255 for LAN broadcast.
	* @param remotePort Which port to connect to on the remote machine.
	* @param onlyReplyOnAcceptingConnections Only request a reply if the remote system is accepting connections
	*/
	void Ping( const char* host, unsigned short remotePort, bool onlyReplyOnAcceptingConnections );

	/**
	* Gets the average of all ping times read for a specified target
	*
	* @param target whose time to read
	* @return The average of all ping times read for a specified target. 
	*/
	int GetAveragePing( PlayerID playerId );

	/**
	* Gets the last ping time read for the specific player or -1 if none read yet
	*
	* @param target whose time to read
	* @return Just the last ping 
	*/
	int GetLastPing( PlayerID playerId ) const;

	/**
	* Gets the lowest ping time read or -1 if none read yet
	*
	* @param target  whose time to read
	* @return the lowest ping time 
	*/
	int GetLowestPing( PlayerID playerId ) const;

	/**
	* Ping the remote systems every so often. This is off by default
	* This will work anytime
	*
	* @param doPing True to start occasional pings.  False to stop them.
	*/
	void SetOccasionalPing( bool doPing );
	
	/*
	* --------------------------------------------------------------------------------------------
	* Static Data Functions - Functions dealing with API defined synchronized memory
	* --------------------------------------------------------------------------------------------
	*/
	/**
	* All systems have a block of data associated with them, for user use.  This block of data can be used to easily
	* specify typical system data that you want to know on connection, such as the player's name.
	*
	* @param playerId Which system you are referring to.  Pass the value returned by GetInternalID to refer to yourself
	*
	* @return The data passed to SetRemoteStaticData stored as a bitstream
	*/
	RakNet::BitStream * GetRemoteStaticData( PlayerID playerId );

	/**
	* All systems have a block of data associated with them, for user use.  This block of data can be used to easily
	* specify typical system data that you want to know on connection, such as the player's name.
	*
	* @param playerId Whose static data to change.  Use your own playerId to change your own static data
	* @param data a block of data to store
	* @param length The length of data in bytes 
	*/
	void SetRemoteStaticData( PlayerID playerId, const char *data, const long length );

	/**
	* Sends your static data to the specified system. This is automatically done on connection.
	* You should call this when you change your static data.
	* To send the static data of another system (such as relaying their data) you should do this normally with Send
	*
	* @param target Who to send your static data to.  Specify UNASSIGNED_PLAYER_ID to broadcast to all
	*/
	void SendStaticData( PlayerID target );

	/**
	* Sets the data to send with an  (LAN server discovery) /(offline ping) response
	* Length should be under 400 bytes, as a security measure against flood attacks
	* See the Ping sample project for how this is used.
	* @param data a block of data to store, or 0 for none
	* @param length The length of data in bytes, or 0 for none
	*/
	void SetOfflinePingResponse( const char *data, const unsigned int length );
	/*
	* --------------------------------------------------------------------------------------------
	* Network Functions - Functions dealing with the network in general
	* --------------------------------------------------------------------------------------------
	*/

	/**
	* Return the unique address identifier that represents you on the the network and is based on your local IP / port
	* Note that unlike in previous versions, this is a struct and is not sequential
	*/
	PlayerID GetInternalID( void ) const;

	/**
	* Return the unique address identifier that represents you on the the network and is based on your external
	* IP / port (the IP / port the specified player uses to communicate with you)
	* @note that unlike in previous versions, this is a struct and is not sequential
	*
	* @param target Which remote system you are referring to for your external ID
	*/
	PlayerID GetExternalID( PlayerID target ) const;

	/**
	* Change the MTU size in order to improve performance when sending large packets
	* This can only be called when not connected.
	* A too high of value will cause packets not to arrive at worst and be fragmented at best.
	* A too low of value will split packets unnecessarily.
	*
	* Parameters:
	* @param size: Set according to the following table:
	* - 1500. The largest Ethernet packet size; it is also the default value.
	* This is the typical setting for non-PPPoE, non-VPN connections. The default value for NETGEAR routers, adapters and switches. 
	* - 1492. The size PPPoE prefers. 
	* - 1472. Maximum size to use for pinging. (Bigger packets are fragmented.) 
	* - 1468. The size DHCP prefers. 
	* - 1460. Usable by AOL if you don't have large email attachments, etc. 
	* - 1430. The size VPN and PPTP prefer. 
	* - 1400. Maximum size for AOL DSL. 
	* - 576. Typical value to connect to dial-up ISPs. (Default)
	*
	* @return False on failure (we are connected).  True on success.  Maximum allowed size is MAXIMUM_MTU_SIZE
	*/
	bool SetMTUSize( int size );

	/**
	* Returns the current MTU size
	*
	* @return The MTU sized specified in SetMTUSize
	*/
	int GetMTUSize( void ) const;

	/**
	* Returns the number of IP addresses we have
	*/
	unsigned GetNumberOfAddresses( void );

	/**
	* Returns the dotted IP address for the specified playerId
	*
	* @param playerId Any player ID other than UNASSIGNED_PLAYER_ID, even if that player is not currently connected
	*/
	const char* PlayerIDToDottedIP( PlayerID playerId ) const;

	/**
	* Converts a dotted IP to a playerId
	*
	* @param[in] host Either a dotted IP address or a domain name
	* @param[in] remotePort Which port to connect to on the remote machine.
	* @param[out] playerId  The result of this operation
	*/
	void IPToPlayerID( const char* host, unsigned short remotePort, PlayerID *playerId );

	/**
	* Returns an IP address at index 0 to GetNumberOfAddresses-1
	*/
	const char* GetLocalIP( unsigned int index );

	/**
	* Allow or disallow connection responses from any IP. Normally this should be false, but may be necessary
	* when connection to servers with multiple IP addresses.
	*
	* @param allow - True to allow this behavior, false to not allow. Defaults to false. Value persists between connections
	*/
	void AllowConnectionResponseIPMigration( bool allow );

	/**
	* Sends a one byte message ID_ADVERTISE_SYSTEM to the remote unconnected system.
	* This will tell the remote system our external IP outside the LAN, and can be used for NAT punch through
	*
	* Requires:
	* The sender and recipient must already be started via a successful call to Initialize
	*
	* @param host Either a dotted IP address or a domain name
	* @param remotePort Which port to connect to on the remote machine.
	* @param data Optional data to append to the packet.
	* @param dataLength length of data in bytes.  Use 0 if no data.
	*/
	void AdvertiseSystem( char *host, unsigned short remotePort, const char *data, int dataLength );
	/*
	* --------------------------------------------------------------------------------------------
	* Compression Functions - Functions related to the compression layer
	* --------------------------------------------------------------------------------------------
	*/

	/**
	* Enables or disables our tracking of bytes input to and output from the network.
	* This is required to get a frequency table, which is used to generate a new compression layer.
	* You can call this at any time - however you SHOULD only call it when disconnected.  Otherwise you will only track
	* part of the values sent over the network.
	* This value persists between connect calls and defaults to false (no frequency tracking)
	* 
	* @param doCompile - true to track bytes. Defaults to false
	*/
	void SetCompileFrequencyTable( bool doCompile );

	/**
	* Returns the frequency of outgoing bytes into outputFrequencyTable
	* The purpose is to save to file as either a master frequency table from a sample game session for passing to
	* GenerateCompressionLayer(false);
	* You should only call this when disconnected.
	* Requires that you first enable data frequency tracking by calling SetCompileFrequencyTable(true)
	*
	* @param[out] outputFrequencyTable  The frequency of each corresponding byte
	* 
	* @return False (failure) if connected or if frequency table tracking is not enabled. Otherwise true (success)
	*/
	bool GetOutgoingFrequencyTable( unsigned int outputFrequencyTable[ 256 ] );

	/**
	* Generates the compression layer from the input frequency table.
	* You should call this twice - once with inputLayer as true and once as false.
	* The frequency table passed here with inputLayer=true should match the frequency table on the recipient with inputLayer=false.
	* Likewise, the frequency table passed here with inputLayer=false should match the frequency table on the recipient with inputLayer=true
	* Calling this function when there is an existing layer will overwrite the old layer
	* You should only call this when disconnected
	*
	* @param inputFrequencyTable The frequency table returned from GetSendFrequencyTable(...);
	* @param inputLayer Whether inputFrequencyTable represents incoming data from other systems (true) or outgoing data from this system (false)
	*
	* @return False on failure (we are connected).  True otherwise
	*/
	bool GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer );

	/**
	* Deletes the output or input layer as specified. This is not necessary to call and is only valuable for freeing memory
	* You should only call this when disconnected
	*
	* @param inputLayer Specifies the corresponding compression layer generated by GenerateCompressionLayer.
	*
	* @return False on failure (we are connected).  True otherwise
	*/
	bool DeleteCompressionLayer( bool inputLayer );

	/**
	* Get the compression ratio. A low compression ratio is good. Compression is for outgoing data 
	* @return The compression ratio. 
	*/
	float GetCompressionRatio( void ) const;

	/**
	* Get the decompression ratio. A high decompression ratio is good.  Decompression is for incoming data
	* @return The decompression ratio. 
	*/
	float GetDecompressionRatio( void ) const;

	/*
	* --------------------------------------------------------------------------------------------
	* Message Handler Functions
	* --------------------------------------------------------------------------------------------
	*/
	/**
	* Attatches a message handler interface to run code automatically on message receipt in the Receive call
	*
	* @param messageHandler Pointer to a message handler to attach
	*/
	void AttachMessageHandler( MessageHandlerInterface *messageHandler );

	/**
	* Detatches a message handler interface to run code automatically on message receipt
	*
	* @param messageHandler Pointer to a message handler to detatch
	*/
	void DetachMessageHandler( MessageHandlerInterface *messageHandler );

	/*
	* --------------------------------------------------------------------------------------------
	* Micellaneous Functions
	* --------------------------------------------------------------------------------------------
	*/
	/**
	* Retrieves the data you passed to the passwordData parameter in Connect
	*
	* @param[out] passwordData  Should point to a block large enough to hold the password data you passed to Connect
	* @param passwordDataLength Maximum size of the array passwordData.  Modified to hold the number of bytes actually written
	*/
	void GetPasswordData( char *passwordData, int *passwordDataLength );

	/**
	* Put a packet back at the end of the receive queue in case you don't want to deal with it immediately
	*
	* @param packet The packet you want to push back.
	*/
	void PushBackPacket( Packet *packet );

	/*
	* --------------------------------------------------------------------------------------------
	* Statistical Functions - Functions dealing with API performance
	* --------------------------------------------------------------------------------------------
	*/

	/**
	* Returns a structure containing a large set of network statistics for the specified system
	* You can map this data to a string using the C style StatisticsToString function
	*
	* @param playerId Which connected system to get statistics for
	* 
	* @return 0 on can't find the specified system.  A pointer to a set of data otherwise.
	*/
	RakNetStatisticsStruct * const GetStatistics( PlayerID playerId );


	/**
	* @brief Used to unify time
	*
	*
	* This structure agregate the ping time and the clock differential. 
	* both are used to synchronized time between peers 
	*/
	struct PingAndClockDifferential
	{
		/**
		* ping time 
		*/
		short pingTime;
		/**
		* clock differential 
		*/
		unsigned int clockDifferential;
	};

	/**
	* @brief Store Remote System Description.
	*
	* RakPeer need to maintain a set of information concerning all remote peer 
	* This is the goal of this structure. 
	*/
	struct RemoteSystemStruct
	{
		PlayerID playerId;  /**< The remote system associated with this reliability layer*/
		PlayerID myExternalPlayerId;  /**< Your own IP, as reported by the remote system*/
		ReliabilityLayer reliabilityLayer;  /**< The reliability layer associated with this player*/
		bool weInitiatedTheConnection; /**< True if we started this connection via Connect.  False if someone else connected to us.*/
		PingAndClockDifferential pingAndClockDifferential[ PING_TIMES_ARRAY_SIZE ];  /**< last x ping times and calculated clock differentials with it*/
		int pingAndClockDifferentialWriteIndex;  /**< The index we are writing into the pingAndClockDifferential circular buffer*/
		int lowestPing; /**<The lowest ping value encounter */
		unsigned int nextPingTime;  /**< When to next ping this player */
		unsigned int lastReliableSend; /**< When did the last reliable send occur.  Reliable sends must occur at least once every TIMEOUT_TIME/2 units to notice disconnects */
		RakNet::BitStream staticData; /**< static data */
		unsigned int connectionTime; /**< connection time, if active. */
		unsigned char AESKey[ 16 ]; /**< Security key. */
		bool setAESKey; /**< true if security is enabled. */
		RPCMap rpcMap; /**< Mapping of RPC calls to single byte integers to save transmission bandwidth. */
		enum ConnectMode {NO_ACTION, DISCONNECT_ASAP, REQUESTED_CONNECTION, HANDLING_CONNECTION_REQUEST, UNVERIFIED_SENDER, SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET, CONNECTED} connectMode;
	};

protected:

#ifdef _WIN32
	// friend unsigned __stdcall RecvFromNetworkLoop(LPVOID arguments);
	friend void __stdcall ProcessPortUnreachable( unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer );
	friend void __stdcall ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
	friend unsigned __stdcall UpdateNetworkLoop( LPVOID arguments );
#else
	// friend void*  RecvFromNetworkLoop( void*  arguments );
	friend void ProcessPortUnreachable( unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer );
	friend void ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
	friend void* UpdateNetworkLoop( void* arguments );
#endif

	//void RemoveFromRequestedConnectionsList( PlayerID playerId );
	bool SendConnectionRequest( const char* host, unsigned short remotePort );
	/**
	* Get the reliability layer associated with a playerID.  
	* @param playerID The player identifier 
	* @return 0 if none
	*/
	RemoteSystemStruct *GetRemoteSystemFromPlayerID( PlayerID playerID ) const;
	/**
	* Parse out a connection request packet
	*/
	void ParseConnectionRequestPacket( RakPeer::RemoteSystemStruct *remoteSystem, PlayerID playerId, const char *data, int byteSize);
	/**
	* When we get a connection request from an ip / port, accept it unless full
	*/
	void OnConnectionRequest( RakPeer::RemoteSystemStruct *remoteSystem, unsigned char *AESKey, bool setAESKey );
	/**
	* Send a reliable disconnect packet to this player and disconnect them when it is delivered
	*/
	void NotifyAndFlagForDisconnect( PlayerID playerId, bool performImmediate );
	/**
	* Returns how many remote systems initiated a connection to us
	*/
	unsigned short GetNumberOfRemoteInitiatedConnections( void ) const;
	/**
	* Get a free remote system from the list and assign our playerID to it.  Should only be called from the update thread - not the user thread
	*/
	RemoteSystemStruct * AssignPlayerIDToRemoteSystemList( PlayerID playerId, RemoteSystemStruct::ConnectMode connectionMode );
	/**
	* An incoming packet has a timestamp, so adjust it to be relative to this system
	*/
	void ShiftIncomingTimestamp( char *data, PlayerID playerId ) const;
	/**
	* Get the most probably accurate clock differential for a certain player
	*/
	unsigned int GetBestClockDifferential( PlayerID playerId ) const;
	/**
	* @todo Document this 
	* 
	*/
	void PushPortRefused( PlayerID target );
	/**
	* Handles an RPC packet.  If you get a packet with the ID ID_RPC you should pass it to this function
	* This is already done in Multiplayer.cpp, so if you use the Multiplayer class it is handled for you.
	*
	* @param data A packet returned from Receive with the ID ID_RPC
	* @param length The size of the packet data 
	* @param playerId The sender of the packet 
	* 
	* @return true on success, false on a bad packet or an unregistered function
	*/
	bool HandleRPCPacket( const char *data, int length, PlayerID playerId );
#ifdef __USE_IO_COMPLETION_PORTS

	bool SetupIOCompletionPortSocket( int index );
#endif
	/**
	* Set this to true to terminate the Peer thread execution 
	*/
	bool endThreads;
	/**
	* true if the peer thread is active. 
	*/
	bool isMainLoopThreadActive;
	// bool isRecvfromThreadActive;  /**< Tracks thread states */
	bool occasionalPing;  /**< Do we occasionally ping the other systems?*/
	/**
	* Store the maximum number of peers allowed to connect
	*/
	unsigned short maximumNumberOfPeers;
	/**
	* Store the maximum number of peers able to connect, including reserved connection slots for pings, etc.
	*/
	unsigned short remoteSystemListSize;
	/**
	* Store the maximum incoming connection allowed 
	*/
	unsigned short maximumIncomingConnections;
	/**
	* localStaticData necessary because we may not have a RemoteSystemStruct representing ourselves in the list
	*/
	RakNet::BitStream incomingPasswordBitStream, outgoingPasswordBitStream, localStaticData, offlinePingResponse;
	/**
	* Local Player ID 
	*/
	PlayerID myPlayerId;
	/**
	* This is an array of pointers to RemoteSystemStruct
	* This allows us to preallocate the list when starting, so we don't have to allocate or delete at runtime.
	* Another benefit is that is lets us add and remove active players simply by setting playerId
	* and moving elements in the list by copying pointers variables without affecting running threads, even if they are in the
	* reliability layer
	*/
	RemoteSystemStruct* remoteSystemList;

	enum
	{
//		requestedConnections_MUTEX,
		incomingPasswordBitStream_Mutex,
		outgoingPasswordBitStream_Mutex,
	//	remoteSystemList_Mutex,    /**< This mutex is to lock remoteSystemList::PlayerID and only used for critical cases */
		//  updateCycleIsRunning_Mutex,
		offlinePingResponse_Mutex,
		//bufferedCommandQueue_Mutex, /**< This mutex is to buffer all send calls to run from the update thread.  This is to get around the problem of the update thread changing playerIDs in the remoteSystemList while in the send call and thus having the sends go to the wrong player */
		//bufferedCommandPool_Mutex, /**< This mutex is to buffer all send calls to run from the update thread.  This is to get around the problem of the update thread changing playerIDs in the remoteSystemList while in the send call and thus having the sends go to the wrong player */
		NUMBER_OF_RAKPEER_MUTEXES
	};
	SimpleMutex rakPeerMutexes[ NUMBER_OF_RAKPEER_MUTEXES ];
	/**
	* RunUpdateCycle is not thread safe but we don't need to mutex calls. Just skip calls if it is running already
	*/
	bool updateCycleIsRunning;
	/**
	* The list of people we have tried to connect to recently
	*/
	//BasicDataStructures::Queue<RequestedConnectionStruct*> requestedConnectionsList;
	/**
	* Data that both the client and the server needs
	*/
	unsigned int bytesSentPerSecond, bytesReceivedPerSecond;
	// bool isSocketLayerBlocking;
	// bool continualPing,isRecvfromThreadActive,isMainLoopThreadActive, endThreads, isSocketLayerBlocking;
	unsigned int validationInteger;
#ifdef _WIN32

	HANDLE
#else
	pthread_t
#endif
		processPacketsThreadHandle, recvfromThreadHandle;
	SimpleMutex incomingQueueMutex, banListMutex; //,synchronizedMemoryQueueMutex, automaticVariableSynchronizationMutex;
	BasicDataStructures::Queue<Packet *> incomingPacketQueue; //, synchronizedMemoryPacketQueue;
	// BitStream enumerationData;
	/**
	* @brief Automatic Variable Synchronization Mechanism
	* 
	* automatic variable synchronization takes a primary and secondary identifier
	* The unique primary identifier is the index into the automaticVariableSynchronizationList
	* The unique secondary identifier (UNASSIGNED_OBJECT_ID for none) is in an unsorted list of memory blocks
	*/
	struct MemoryBlock
	{
		char *original, *copy;
		unsigned short size;
		ObjectID secondaryID;
		bool isAuthority;
		bool ( *synchronizationRules ) ( char*, char* );
	};

	struct BanStruct
	{
		char *IP;
		unsigned int timeout; // 0 for none
	};

	struct RequestedConnectionStruct
	{
		PlayerID playerId;
		unsigned int nextRequestTime;
		unsigned char requestsMade;
		char *data;
		unsigned short dataLength;
		enum {CONNECT=1, PING=2, PING_OPEN_CONNECTIONS=4, ADVERTISE_SYSTEM=8} actionToTake;
	};

	//BasicDataStructures::List<BasicDataStructures::List<MemoryBlock>* > automaticVariableSynchronizationList;
	BasicDataStructures::List<BanStruct*> banList;
	BasicDataStructures::List<MessageHandlerInterface*> messageHandlerList;
	BasicDataStructures::SingleProducerConsumer<RequestedConnectionStruct> requestedConnectionList;

	/*
	* Compression stuff
	*/
	unsigned int frequencyTable[ 256 ];
	HuffmanEncodingTree *inputTree, *outputTree;
	unsigned int rawBytesSent, rawBytesReceived, compressedBytesSent, compressedBytesReceived;
	// void DecompressInput(RakNet::BitStream *bitStream);
	// void UpdateOutgoingFrequencyTable(RakNet::BitStream * bitStream);
	void GenerateSYNCookieRandomNumber( void );
	void SecuredConnectionResponse( PlayerID playerId );
	void SecuredConnectionConfirmation( RakPeer::RemoteSystemStruct * remoteSystem, char* data );
	bool RunUpdateCycle( void );
	// void RunMutexedUpdateCycle(void);

	struct BufferedCommandStruct
	{
		char *data;
		int numberOfBitsToSend;
		PacketPriority priority;
		PacketReliability reliability;
		char orderingChannel;
		PlayerID playerId;
		bool broadcast;
		RemoteSystemStruct::ConnectMode connectionMode;
		ObjectID objectID;
		enum {BCS_SEND, BCS_CLOSE_CONNECTION, BCS_RPC, BCS_RPC_SHIFT, BCS_DO_NOTHING} command;
	};

	// Single producer single consumer queue using a linked list
	//BufferedCommandStruct* bufferedCommandReadIndex, bufferedCommandWriteIndex;
	BasicDataStructures::SingleProducerConsumer<BufferedCommandStruct> bufferedCommands;

	//BasicDataStructures::Queue<BufferedCommandStruct *> bufferedCommandQueue;
	//BasicDataStructures::Queue<BufferedCommandStruct *> bufferedCommandPool;
	//RakPeer::BufferedCommandStruct *LockWriteBufferedCommandStruct(bool *outParam_wasInserted); // Get the next BCS to write
	//void UnlockWriteBufferedCommandStruct(RakPeer::BufferedCommandStruct *bcs, bool *outParam_wasInserted); // Done with the next BCS to write
	//RakPeer::BufferedCommandStruct *LockReadBufferedCommandStruct(void); // Get the next BCS to read
	//RakPeer::BufferedCommandStruct *UnlockReadBufferedCommandStruct(void); // Done with the next BCS to read
	
	//void FreeBufferedCommandStruct(RakPeer::BufferedCommandStruct *bcs);

	bool AllowIncomingConnections(void) const;

	// Sends static data using immediate send mode or not (if called from user thread, put false for performImmediate.  If called from update thread, put true).
	// This is done for efficiency, so we don't buffer send calls that are from the network thread anyway
	void SendStaticDataInternal( PlayerID target, bool performImmediate );
	void PingInternal( PlayerID target, bool performImmediate );
	bool ValidSendTarget(PlayerID playerId, bool broadcast);
	// This stores the user send calls to be handled by the update thread.  This way we don't have thread contention over playerIDs
	void CloseConnectionInternal( PlayerID target, bool sendDisconnectionNotification, bool performImmediate );
	void SendBuffered( RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, RemoteSystemStruct::ConnectMode connectionMode );
	bool SendImmediate( char *data, int numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool useCallerDataAllocation, unsigned int currentTime );
	bool HandleBufferedRPC(BufferedCommandStruct *bcs, unsigned time);
	void ClearBufferedCommands(void);
	void ClearRequestedConnectionList(void);

	//BasicDataStructures::AVLBalancedBinarySearchTree<RPCNode> rpcTree;
	RPCMap rpcMap; // Can't use StrPtrHash because runtime insertions will screw up the indices
	int MTUSize;
	bool trackFrequencyTable;
	int threadSleepTimer;

	SOCKET connectionSocket;

	// Histogram statistics
	//unsigned int nextReadBytesTime;
	//int lastSentBytes,lastReceivedBytes;
	/*
	* Encryption and security
	*/
	big::RSACrypt<RSA_BIT_SIZE> rsacrypt;
	big::u32 publicKeyE;
	RSA_BIT_SIZE publicKeyN;
	bool keysLocallyGenerated, usingSecurity;
	unsigned int randomNumberExpirationTime;
	unsigned char newRandomNumber[ 20 ], oldRandomNumber[ 20 ];
	/**
	* How long it has been since things were updated by a call to receive
	* Update thread uses this to determine how long to sleep for
	*/
	unsigned int lastUserUpdateCycle;
	/*
	* True to allow connection accepted packets from anyone.  False to only allow these packets from servers
	* we requested a connection to.
	*/
	bool allowConnectionResponseIPMigration;

	PacketPool packetPool;
};

#endif
