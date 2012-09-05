/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief User interface of RakPeer.
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

#ifndef __RAK_PEER_INTERFACE_H
#define __RAK_PEER_INTERFACE_H

#include "PacketPriority.h"
#include "NetworkTypes.h"
#include "BitStream.h"
#include "RakNetStatistics.h"

/**
 * @brief Define the user view of a RakPeer instance.
 *
 * This class define the user point of view of a RakPeer instance. 
 *
 */

class MessageHandlerInterface;

class RakPeerInterface
{

public:
	/**
	 * Destructor
	 */
	virtual ~RakPeerInterface()	{}
	
	// --------------------------------------------------------------------------------------------
	// Major Low Level Functions - Functions needed by most users
	// --------------------------------------------------------------------------------------------
	
	/**
	 * Starts the network threads, opens the listen port
	 * You must call this before calling SetMaximumIncomingConnections or Connect
	 * Multiple calls while already active are ignored.  To call this function again with different settings, you must first call Disconnect()
	 * To accept incoming connections, use SetMaximumIncomingConnections
	 *
	 * Parameters:
	 * @param MaximumNumberOfPeers  Required so RakNet can preallocate and for thread safety.
	 * - A pure client would set this to 1.  A pure server would set it to the number of allowed clients.
	 * - A hybrid would set it to the sum of both types of connections
	 * @param localPort The port to listen for connections on.
	 * @param _threadSleepTimer >=0 for how many ms to Sleep each internal update cycle (recommended 30 for low performance, 0 for regular)
	 * @param forceHostAddress Can force RakNet to use a particular IP to host on.  Pass 0 to automatically pick an IP
	 *
	 * @return False on failure (can't create socket or thread), true on success.
	 */
	virtual bool Initialize( unsigned short MaximumNumberOfPeers, unsigned short localPort, int _threadSleepTimer, const char *forceHostAddress=0 ) = 0;
	
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
	virtual void InitializeSecurity(const char *pubKeyE, const char *pubKeyN, const char *privKeyP, const char *privKeyQ ) = 0;
	
	/**
	 * Disables all security.
	 * @note Must be called while offline
	 *
	 */
	virtual void DisableSecurity( void ) = 0;
	
	/**
	 * Sets how many incoming connections are allowed. If this is less than the number of players currently connected, no
	 * more players will be allowed to connect.  If this is greater than the maximum number of peers allowed, it will be reduced
	 * to the maximum number of peers allowed.
	 *
	 * @param numberAllowed Maximum number of incoming connections allowed.
	 */
	virtual void SetMaximumIncomingConnections( unsigned short numberAllowed ) = 0;

	/**
	 * Get the number of maximum incomming connection. 
	 * @return the maximum number of incoming connections, which is always <= MaximumNumberOfPeers
	 */
	
	virtual unsigned short GetMaximumIncomingConnections( void ) const = 0;
	
	/**
	 * Sets the password incoming connections must match in the call to Connect (defaults to none)
	 * Pass 0 to passwordData to specify no password
	 *
	 * @param passwordData A data block that incoming connections must match.  This can be just a password, or can be a stream of data.
	 * Specify 0 for no password data
	 * @param passwordDataLength The length in bytes of passwordData
	 */
	virtual void SetIncomingPassword( const char* passwordData, int passwordDataLength ) = 0;
	
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
	virtual bool Connect( const char* host, unsigned short remotePort, char* passwordData, int passwordDataLength ) = 0;

	/**
	 * Stops the network threads and close all connections.  Multiple calls are ok.
	 *
	 *
	 * @param blockDuration How long you should wait for all remaining packets to go out, per connected system
	 * If you set it to 0 then the disconnection notification probably won't arrive
	 */
	virtual void Disconnect( unsigned int blockDuration ) = 0;

	/**
	 * Returns true if the network threads are running
	 */
	virtual bool IsActive( void ) const = 0;

	/**
	 * Fills the array remoteSystems with the playerID of all the systems we are connected to
	 *
	 * @param[out] remoteSystems  An array of PlayerID structures to be filled with the PlayerIDs of the systems we are connected to
	 * - pass 0 to remoteSystems to only get the number of systems we are connected to
	 * @param numberOfSystems As input, the size of remoteSystems array.  As output, the number of elements put into the array 
	 */
	virtual bool GetConnectionList( PlayerID *remoteSystems, unsigned short *numberOfSystems ) const = 0;

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
	 * @param broadcast True to send this packet to all connected systems.  If true, then playerId specifies who not to send the packet to.
	 * @return 
	 * False if we are not connected to the specified recipient.  True otherwise
	 */
	virtual bool Send( const char *data, const long length, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast ) = 0;

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
	 * @param broadcast True to send this packet to all connected systems.  If true, then playerId specifies who not to send the packet to.
	 * @return 
	 * False if we are not connected to the specified recipient.  True otherwise
	 */
	virtual bool Send( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast ) = 0;

	/**
	 * Gets a packet from the incoming packet queue. Use DeallocatePacket to deallocate the packet after you are done with it.
	 * Check the Packet struct at the top of CoreNetworkStructures.h for the format of the struct
	 *
	 * @return
	 * 0 if no packets are waiting to be handled, otherwise an allocated packet
	 * If the client is not active this will also return 0, as all waiting packets are flushed when the client is Disconnected
	 * This also updates all memory blocks associated with synchronized memory and distributed objects
	 */
	virtual Packet* Receive( void ) = 0;

	/**
	 * Call this to deallocate a packet returned by Receive when you are done handling it.
	 * @param packet A packet to free
	 */
	
	virtual void DeallocatePacket( Packet *packet ) = 0;

	/**
	 * Return the total number of connections we are allowed
	 */
	virtual unsigned short GetMaximumNumberOfPeers( void ) const = 0;
	
	// --------------------------------------------------------------------------------------------
	// Player Management Functions
	// --------------------------------------------------------------------------------------------

	/**
	 * Close the connection to another host (if we initiated the connection it will disconnect, if they did it will kick them out).
	 *
	 * @param target Which connection to immediately close
	 * @param sendDisconnectionNotification True to send ID_DISCONNECTION_NOTIFICATION to the recipient.  False to close it silently.
	*/
	virtual void CloseConnection( PlayerID target, bool sendDisconnectionNotification, int depreciated=0 ) = 0;

	/**
	 * Given a playerID, returns an index from 0 to the maximum number of players allowed - 1.
	 *
	 * @param playerId The playerID to search for
	 *
	 * @return An integer from 0 to the maximum number of peers -1, or -1 if that player is not found
	 */
	virtual int GetIndexFromPlayerID( PlayerID playerId ) = 0;
	
	/**
	 * This function is only useful for looping through all players.
	 *
	 * @param index  an integer between 0 and the maximum number of players allowed - 1.
	 *
	 * @return A valid playerID or UNASSIGNED_PLAYER_ID if no such player at that index
	 */
	virtual PlayerID GetPlayerIDFromIndex( int index ) = 0;
	
	/**
	 * Bans an IP from connecting.  Banned IPs persist between connections.
	 *
	 * @param IP Dotted IP address.  Can use * as a wildcard, such as 128.0.0.* will ban
	 * All IP addresses starting with 128.0.0
	 * @param milliseconds - how many ms for a temporary ban.  Use 0 for a permanent ban
	 */
	virtual void AddToBanList( const char *IP, unsigned int milliseconds=0 ) = 0;

	/**
	 * Bans an IP from connecting.  Banned IPs persist between connections.
	 *
	 * @param IP Dotted IP address.  Can use * as a wildcard, such as 128.0.0.* will ban
	 * All IP addresses starting with 128.0.0
	 */
	virtual void RemoveFromBanList( const char *IP ) = 0;

	/**
	 * Allows all previously banned IPs to connect.
	 */
	virtual void ClearBanList( void ) = 0;

	/**
	 * Determines if a particular IP is banned.
	 *
	 * @param IP Complete dotted IP address
	 *
	 * @return
	 * - True if IP matches any IPs in the ban list, accounting for any wildcards.
	 * - False otherwise.
	 */
	virtual bool IsBanned( const char *IP ) = 0;
	
	// --------------------------------------------------------------------------------------------
	// Pinging Functions - Functions dealing with the automatic ping mechanism
	// --------------------------------------------------------------------------------------------
	
	/**
	 * Send a ping to the specified connected system.
	 *
	 * @param target  who to ping
	 */
	virtual void Ping( PlayerID target ) = 0;

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
	virtual void Ping( const char* host, unsigned short remotePort, bool onlyReplyOnAcceptingConnections ) = 0;

	/**
	 * Gets the average of all ping times read for a specified target
	 *
	 * @param target whose time to read
	 * @return The average of all ping times read for a specified target. 
	 */
	virtual int GetAveragePing( PlayerID playerId ) = 0;

	/**
	 * Gets the last ping time read for the specific player or -1 if none read yet
	 *
	 * @param target whose time to read
	 * @return Just the last ping 
	 */
	virtual int GetLastPing( PlayerID playerId ) const = 0;

	/**
	 * Gets the lowest ping time read or -1 if none read yet
	 *
	 * @param target  whose time to read
	 * @return the lowest ping time 
	 */
	virtual int GetLowestPing( PlayerID playerId ) const = 0;

	/**
	 * Ping the remote systems every so often. This is off by default
	 * This will work anytime
	 *
	 * @param doPing True to start occasional pings.  False to stop them.
	 */
	virtual void SetOccasionalPing( bool doPing ) = 0;
	
	// --------------------------------------------------------------------------------------------
	// Static Data Functions - Functions dealing with API defined synchronized memory
	// --------------------------------------------------------------------------------------------
	/**
	 * All systems have a block of data associated with them, for user use.  This block of data can be used to easily
	 * specify typical system data that you want to know on connection, such as the player's name.
	 *
	 * @param playerId Which system you are referring to.  Pass the value returned by GetInternalID to refer to yourself
	 *
	 * @return The data passed to SetRemoteStaticData stored as a bitstream
	 */
	virtual RakNet::BitStream * GetRemoteStaticData( PlayerID playerId ) = 0;

	/**
	 * All systems have a block of data associated with them, for user use.  This block of data can be used to easily
	 * specify typical system data that you want to know on connection, such as the player's name.
	 *
	 * @param playerId Whose static data to change.  Use your own playerId to change your own static data
	 * @param data a block of data to store
	 * @param length The length of data in bytes 
	 */
	virtual void SetRemoteStaticData( PlayerID playerId, const char *data, const long length ) = 0;

	/**
	 * Sends your static data to the specified system. This is automatically done on connection.
	 * You should call this when you change your static data.
	 * To send the static data of another system (such as relaying their data) you should do this normally with Send
	 *
	 * @param target Who to send your static data to.  Specify UNASSIGNED_PLAYER_ID to broadcast to all
	 */
	virtual void SendStaticData( PlayerID target ) = 0;

	/**
	 * Sets the data to send with an  (LAN server discovery) /(offline ping) response
	 * Length should be under 400 bytes, as a security measure against flood attacks
	 * See the Ping sample project for how this is used.
	 * @param data a block of data to store, or 0 for none
	 * @param length The length of data in bytes, or 0 for none
	 */
	virtual void SetOfflinePingResponse( const char *data, const unsigned int length ) = 0;
	
	// --------------------------------------------------------------------------------------------
	// Network Functions - Functions dealing with the network in general
	// --------------------------------------------------------------------------------------------
	/**
	 * Return the unique address identifier that represents you on the the network and is based on your local IP / port
	 * Note that unlike in previous versions, this is a struct and is not sequential
	 */
	virtual PlayerID GetInternalID( void ) const = 0;

	/**
	 * Return the unique address identifier that represents you on the the network and is based on your external
	 * IP / port (the IP / port the specified player uses to communicate with you)
	 * @note that unlike in previous versions, this is a struct and is not sequential
	 *
	 * @param target Which remote system you are referring to for your external ID
	 */
	virtual PlayerID GetExternalID( PlayerID target ) const = 0;
	
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
	virtual bool SetMTUSize( int size ) = 0;
	
	/**
	 * Returns the current MTU size
	 *
	 * @return The MTU sized specified in SetMTUSize
	 */
	virtual int GetMTUSize( void ) const = 0;

	/**
	 * Returns the number of IP addresses we have
	 */
	virtual unsigned GetNumberOfAddresses( void ) = 0;

	/**
	 * Returns the dotted IP address for the specified playerId
	 *
	 * @param playerId Any player ID other than UNASSIGNED_PLAYER_ID, even if that player is not currently connected
	 */
	virtual const char* PlayerIDToDottedIP( PlayerID playerId ) const = 0;

	/**
	 * Converts a dotted IP to a playerId
	 *
	 * @param[in] host Either a dotted IP address or a domain name
	 * @param[in] remotePort Which port to connect to on the remote machine.
	 * @param[out] playerId  The result of this operation
	 */
	virtual void IPToPlayerID( const char* host, unsigned short remotePort, PlayerID *playerId ) = 0;

	/**
	 * Returns an IP address at index 0 to GetNumberOfAddresses-1
	 */
	virtual const char* GetLocalIP( unsigned int index ) = 0;

	/**
	 * Allow or disallow connection responses from any IP.  Normally this should be false, but may be necessary
	 * when connection to servers with multiple IP addresses.
	 *
	 * @param allow - True to allow this behavior, false to not allow. Defaults to false.  Value persists between connections
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
	virtual void AdvertiseSystem( char *host, unsigned short remotePort, const char *data, int dataLength ) = 0;
	
	// --------------------------------------------------------------------------------------------
	// Compression Functions - Functions related to the compression layer
	// --------------------------------------------------------------------------------------------
	
	/**
	 * Enables or disables our tracking of bytes input to and output from the network.
	 * This is required to get a frequency table, which is used to generate a new compression layer.
	 * You can call this at any time - however you SHOULD only call it when disconnected.  Otherwise you will only track
	 * part of the values sent over the network.
	 * This value persists between connect calls and defaults to false (no frequency tracking)
	 * 
	 * @param doCompile - true to track bytes. Defaults to false
	 */
	virtual void SetCompileFrequencyTable( bool doCompile ) = 0;
	
	/**
	 * Returns the frequency of outgoing bytes into outputFrequencyTable
	 * The purpose is to save to file as either a master frequency table from a sample game session for passing to
	 * GenerateCompressionLayer(false);
	 * You should only call this when disconnected.
	 * Requires that you first enable data frequency tracking by calling SetCompileFrequencyTable(true)
	 *
	 * @param[out] outputFrequencyTable  The frequency of each corresponding byte
	 * 
	 * @return False (failure) if connected or if frequency table tracking is not enabled.  Otherwise true (success)
	 */
	virtual bool GetOutgoingFrequencyTable( unsigned int outputFrequencyTable[ 256 ] ) = 0;
	
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
	virtual bool GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer ) = 0;
	
	/**
	 * Deletes the output or input layer as specified. This is not necessary to call and is only valuable for freeing memory
	 * You should only call this when disconnected
	 *
	 * @param inputLayer Specifies the corresponding compression layer generated by GenerateCompressionLayer.
	 *
	 * @return False on failure (we are connected).  True otherwise
	 */
	virtual bool DeleteCompressionLayer( bool inputLayer ) = 0;
	
	/**
	 * Get the compression ratio. A low compression ratio is good.  Compression is for outgoing data 
	 * @return The compression ratio. 
	 */
	virtual float GetCompressionRatio( void ) const = 0;
	
	/**
	 * Get the decompression ratio. A high decompression ratio is good.  Decompression is for incoming data
	 * @return The decompression ratio.  
	 */
	virtual float GetDecompressionRatio( void ) const = 0;
	
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
	virtual void AttachMessageHandler( MessageHandlerInterface *messageHandler )=0;

	/**
	* Detatches a message handler interface to run code automatically on message receipt
	*
	* @param messageHandler Pointer to a message handler to detatch
	*/
	virtual void DetachMessageHandler( MessageHandlerInterface *messageHandler )=0;

	// --------------------------------------------------------------------------------------------
	// Micellaneous Functions
	// --------------------------------------------------------------------------------------------
	
	/**
	 * Retrieves the data you passed to the passwordData parameter in Connect
	 *
	 * @param[out] passwordData  Should point to a block large enough to hold the password data you passed to Connect
	 * @param passwordDataLength Maximum size of the array passwordData.  Modified to hold the number of bytes actually written
	 */
	virtual void GetPasswordData( char *passwordData, int *passwordDataLength ) = 0;

	/**
	 * Put a packet back at the end of the receive queue in case you don't want to deal with it immediately
	 *
	 * @param packet The packet you want to push back.
	 */
	virtual void PushBackPacket( Packet *packet ) = 0;
	
	
	// --------------------------------------------------------------------------------------------
	// Statistical Functions - Functions dealing with API performance
	// --------------------------------------------------------------------------------------------

	/**
	 * Returns a structure containing a large set of network statistics for the specified system
	 * You can map this data to a string using the C style StatisticsToString function
	 *
	 * @param playerId Which connected system to get statistics for
	 * 
	 * @return 0 on can't find the specified system.  A pointer to a set of data otherwise.
	 */
	virtual RakNetStatisticsStruct * const GetStatistics( PlayerID playerId ) = 0;
};

#endif
