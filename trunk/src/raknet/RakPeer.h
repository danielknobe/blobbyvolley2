/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief A RakPeer is the lower level Communication End Point.
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
#ifndef __RAK_PEER_H
#define __RAK_PEER_H

#include "ReliabilityLayer.h"
#include "BitStream.h"
#include "SingleProducerConsumer.h"
#include "PacketPool.h"
#include "RakNetPacket.h"


#ifdef _WIN32
void __stdcall ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
unsigned __stdcall UpdateNetworkLoop( LPVOID arguments );
#else
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

class RakPeer
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
	* Call this to connect to the specified host (ip or domain name) and server port.
	* Calling Connect and not calling SetMaximumIncomingConnections acts as a dedicated client.  Calling both acts as a true peer.
	* This is a non-blocking connection.  You know the connection is successful when IsConnected() returns true
	* or receive gets a packet with the type identifier ID_CONNECTION_ACCEPTED.  If the connection is not
	* successful, such as rejected connection or no response then neither of these things will happen.
	* Requires that you first call Initialize
	*
	* @param host Either a dotted IP address or a domain name
	* @param remotePort Which port to connect to on the remote machine.
	*
	* @return True on successful initiation. False on incorrect parameters, internal error, or too many existing peers
	*/
	bool Connect( const char* host, unsigned short remotePort );
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
	bool Send( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast );

	/**
	* Gets a packet from the incoming packet queue. Use DeallocatePacket to deallocate the packet after you are done with it.
	* Check the Packet struct at the top of CoreNetworkStructures.h for the format of the struct
	*
	* @return
	* 0 if no packets are waiting to be handled, otherwise an allocated packet
	* If the client is not active this will also return 0, as all waiting packets are flushed when the client is Disconnected
	* This also updates all memory blocks associated with synchronized memory and distributed objects
	*/
	packet_ptr Receive( void );

	/**
	* Return the total number of connections we are allowed
	*/
	unsigned short GetMaximumNumberOfPeers( void ) const;

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
	void CloseConnection( PlayerID target, bool sendDisconnectionNotification );

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
	* Micellaneous Functions
	* --------------------------------------------------------------------------------------------
	*/

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
		short pingTime;  /**< last ping time */
		int lowestPing; /**<The lowest ping value encounter */
		unsigned int nextPingTime;  /**< When to next ping this player */
		unsigned int lastReliableSend; /**< When did the last reliable send occur.  Reliable sends must occur at least once every TIMEOUT_TIME/2 units to notice disconnects */
		unsigned int connectionTime; /**< connection time, if active. */
		enum ConnectMode {NO_ACTION, DISCONNECT_ASAP, REQUESTED_CONNECTION, HANDLING_CONNECTION_REQUEST, UNVERIFIED_SENDER, CONNECTED} connectMode;
	};

protected:

#ifdef _WIN32
	friend void __stdcall ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
	friend unsigned __stdcall UpdateNetworkLoop( LPVOID arguments );
#else
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
	void OnConnectionRequest( RakPeer::RemoteSystemStruct *remoteSystem );
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
	* @todo Document this
	*
	*/
	void PushPortRefused( PlayerID target );

	/**
	* Set this to true to terminate the Peer thread execution
	*/
	bool endThreads;
	/**
	* true if the peer thread is active.
	*/
	bool isMainLoopThreadActive;
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

	/**
	* RunUpdateCycle is not thread safe but we don't need to mutex calls. Just skip calls if it is running already
	*/
	bool updateCycleIsRunning;
	/**
	* Data that both the client and the server needs
	*/
	unsigned int bytesSentPerSecond, bytesReceivedPerSecond;
	unsigned int validationInteger;
#ifdef _WIN32

	HANDLE
#else
	pthread_t
#endif
		processPacketsThreadHandle, recvfromThreadHandle;
	SimpleMutex incomingQueueMutex;
	BlobNet::ADT::Queue<Packet *> incomingPacketQueue;
	// BitStream enumerationData;

	struct RequestedConnectionStruct
	{
		PlayerID playerId;
		unsigned int nextRequestTime;
		unsigned char requestsMade;
		char *data;
		unsigned short dataLength;
		enum {CONNECT=1, PING=2, PING_OPEN_CONNECTIONS=4, ADVERTISE_SYSTEM=8} actionToTake;
	};

	BasicDataStructures::SingleProducerConsumer<RequestedConnectionStruct> requestedConnectionList;

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
		enum {BCS_SEND, BCS_CLOSE_CONNECTION, BCS_DO_NOTHING} command;
	};

	// Single producer single consumer queue using a linked list
	BasicDataStructures::SingleProducerConsumer<BufferedCommandStruct> bufferedCommands;

	bool AllowIncomingConnections(void) const;

	void PingInternal( PlayerID target, bool performImmediate );
	bool ValidSendTarget(PlayerID playerId, bool broadcast);
	// This stores the user send calls to be handled by the update thread.  This way we don't have thread contention over playerIDs
	void CloseConnectionInternalBuffered( PlayerID target, bool sendDisconnectionNotification );
	void CloseConnectionInternalImmediate( PlayerID target );
	void SendBuffered( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, RemoteSystemStruct::ConnectMode connectionMode );
	bool SendImmediate( char *data, int numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool useCallerDataAllocation, unsigned int currentTime );

	void ClearBufferedCommands(void);
	void ClearRequestedConnectionList(void);

	int MTUSize;
	int threadSleepTimer;

	SOCKET connectionSocket;

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
