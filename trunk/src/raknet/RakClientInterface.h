/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief User view of a RakClient object. 
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

#ifndef __RAK_CLIENT_INTERFACE_H
#define __RAK_CLIENT_INTERFACE_H

#include "NetworkTypes.h"
#include "PacketPriority.h"
#include "RakPeerInterface.h"
#include "BitStream.h"
#include "RakNetStatistics.h" 
/**
* @brief Define user point of vue of a RakClient communication end point.
* 
* This class define the user view of a RakClient communication
* end-point. All accessible operation are available in this class.
* You should only deal with RakClient object throught instance of
* RakClientInterface.
*/

class RakClientInterface
{

public:
	/**
	* Destructor
	*/
	virtual ~RakClientInterface()
	{}

	/**
	* Call this to connect the client to the specified host (ip or domain name) and server port.
	* This is a non-blocking connection.  You know the connection is successful when IsConnected() returns true
	* or receive gets a packet with the type identifier ID_CONNECTION_REQUEST_ACCEPTED.
	* serverPort is which port to connect to on the remote machine. clientPort is the port you want the 
	* client to use. Both ports must be open for UDP
	*
	* @param host a hostname 
	* @param serverPort The port on which to contact @em host 
	* @param clientPort The port to use localy 
	* @param depreciated is legacy and unused
	* @param threadSleepTimer >=0 for how many ms to Sleep each internal update cycle 
	* (recommended 30 for low performance, 0 for regular)
	* @return true on successful initiation, false otherwise
	*/
	virtual bool Connect( const char* host, unsigned short serverPort, unsigned short clientPort, unsigned int depreciated, int threadSleepTimer ) = 0;
	/**
	* Stops the client, stops synchronized data, and resets all internal data. 
	* Does nothing if the client is not connected to begin with
	* blockDuration is how long you should wait for all remaining packets to go out
	* If you set it to 0 then the disconnection notification probably won't arrive
	* @param blockDuration The time to wait before truly close the communication and point
	*/
	virtual void Disconnect( unsigned int blockDuration ) = 0;
	/**
	* Can be called to use specific public RSA keys. (e and n)
	* In order to prevent altered keys.  Will return ID_RSA_PUBLIC_KEY_MISMATCH in a packet
	* If a key has been altered.
	*
	* @param privKeyP Private keys generated from the RSACrypt class. Can be 0
	* @param privKeyQ Private keys generated from the RSACrypt class. Can be 0 
	* @see   Encryption sample. 
	*/
	virtual void InitializeSecurity( const char *privKeyP, const char *privKeyQ ) = 0;
	/**
	* Set the password to use when connecting to a server.  The password persists between connections.
	* Pass 0 for no password.
	* @param _password The password to use to connect to a server 
	*/
	virtual void SetPassword( const char *_password ) = 0;
	/**
	* Returns true if a password was set, false otherwise
	* @return true if a password has previously been set using SetPassword
	*/
	virtual bool HasPassword( void ) const = 0;
	/**
	* This function only works while the client is connected (Use the
	* Connect function).  Returns false on failure, true on success
	* Sends the data stream of length length If you aren't sure what to
	* specify for priority and reliability, use HIGH_PRIORITY and
	* RELIABLE, 0 for ordering channel
	* @param data a byte buffer 
	* @param length the size of the byte buffer 
	* @param priority the priority of the message 
	* @param reliability the reliability policy required 
	* @param orderingChannel the channel to send the message to. 
	*/
	virtual bool Send( const char *data, const long length, PacketPriority priority, PacketReliability reliability, char orderingChannel ) = 0;
	/**
	* This function only works while the client is connected (Use the
	* Connect function).  Returns false on failure, true on success
	* Sends the BitStream If you aren't sure what to specify for
	* priority and reliability, use HIGH_PRIORITY and RELIABLE, 0 for
	* ordering channel
	* @param bitstream the data to send. 
	* @param priority the priority of the message 
	* @param reliability the reliability policy required 
	* @param orderingChannel the channel to send the message to. 
	*/
	virtual bool Send( RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel ) = 0;
	/**
	* Call this to get a packet from the incoming packet queue. Use
	* DeallocatePacket to deallocate the packet after you are done with
	* it.  Check the Packet struct at the top of
	* CoreNetworkStructures.h for the format of the struct Returns 0 if
	* no packets are waiting to be handled If the client is not active
	* this will also return 0, as all waiting packets are flushed when
	* the client is Disconnected This also updates all memory blocks
	* associated with synchronized memory
	* @return the last receive packet 
	*/
	virtual Packet* Receive( void ) = 0;
	/**
	* Call this to deallocate a packet returned by Receive when you are done handling it.
	* Free the memory associated to a packet. It is not the same as using delete operator because 
	* RakNet might decide not to delete right now the packet in order to use it later. 
	* @param packet the packet to deallocate. 
	*/
	virtual void DeallocatePacket( Packet *packet ) = 0;
	/**
	* Send a ping request to the server.Occasional pings are on by
	* default (see StartOccasionalPing and StopOccasionalPing) so
	* unless you turn them off it is not necessary to call this
	* function.  It is here for completeness if you want it Does
	* nothing if the client is not connected to begin with
	*/
	virtual void PingServer( void ) = 0;
	/**
	* Sends a ping request to a server we are not connected to.  This will also initialize the
	* networking system if it is not already initialized.  You can stop the networking system
	* by calling Disconnect()
	* The final ping time will be encoded in the following 4 bytes (2-5) as an unsigned int
	* You can specify if the server should only reply if it has an open connection or not
	* This must be true for LAN broadcast server discovery on "255.255.255.255"
	* or you will get replies from clients as well.  A broadcast will always have 0 for the ping time as only single
	* byte packets are allowed from unconnected systems.
	* @param host The host to contact 
	* @param ServerPort the port used by the server
	* @param clientPort the port used to receive the answer 
	* @param onlyReplyOnAcceptingConnections if true the server must be ready to accept incomming connection. 
	*/
	virtual void PingServer( const char* host, unsigned short serverPort, unsigned short clientPort, bool onlyReplyOnAcceptingConnections ) = 0;
	/**
	* Returns the average of all ping times read
	* @return the average ping value to the server 
	*/
	virtual int GetAveragePing( void ) = 0;
	/**
	* Returns the last ping time read for the specific player or -1 if none read yet
	* @return last ping value
	*/
	virtual int GetLastPing( void ) const = 0;
	/**
	* Returns the lowest ping time read or -1 if none read yet
	* @return lowest ping value 
	*/
	virtual int GetLowestPing( void ) const = 0;
	/**
	* Returns the last ping for the specified player. This information
	* is broadcast by the server automatically In order to save
	* bandwidth this information is updated only infrequently and only
	* for the first 32 players 
	* @param playerId The id of the player you want to have the ping (it might be your id) 
	* @return the last ping for this player 
	* @note You can read your own ping with
	* this method by passing your own playerId, however for more
	* up-to-date readings you should use one of the three functions
	* above
	*
	*/
	virtual int GetPlayerPing( PlayerID playerId ) = 0;
	/**
	* Ping the server every so often. This is on by default. In games
	* where you don't care about ping you can call StopOccasionalPing
	* to save the bandwidth This will work anytime
	*/
	virtual void StartOccasionalPing( void ) = 0;
	/**
	* Stop pinging the server every so often. The server is pinged by
	* default.  In games where you don't care about ping you can call
	* this to save the bandwidth This will work anytime
	*/
	virtual void StopOccasionalPing( void ) = 0;
	/**
	* Returns true if the client is connected to a responsive server
	* @return true if connected to a server 
	*/
	virtual bool IsConnected( void ) const = 0;
	/**
	* Returns a number automatically synchronized between the server
	* and client which randomly changes every 9 seconds. The time it
	* changes is accurate to within a few ms and is best used to seed
	* random number generators that you want to usually return the same
	* output on all systems.  Keep in mind this isn't perfectly
	* accurate as there is always a very small chance the numbers will
	* by out of synch during changes so you should confine its use to
	* visual effects or functionality that has a backup method to
	* maintain synchronization.  If you don't need this functionality
	* and want to save the bandwidth call StopSynchronizedRandomInteger
	* after starting the server
	* @return A random int common to all client and to the server. 
	*/
	virtual unsigned int GetSynchronizedRandomInteger( void ) const = 0;

	/**
	* This is an optional function to generate the compression layer
	* from the input frequency table. You should call this twice -
	* once with inputLayer as true and once as false. The frequency
	* table passed here with inputLayer=true should match the frequency
	* table on the recipient with inputLayer=false.  Likewise, the
	* frequency table passed here with inputLayer=false should match
	* the frequency table on the recipient with inputLayer=true Calling
	* this function when there is an existing layer will overwrite the
	* old layer You should only call this when disconnected 
	* 
	* @param inputFrenquencyTable the table to used for compression 
	* @param inputLayer says if the @em inputFrequencyTable should be used for
	* sending or receiveing.
	* @return false (failure) if connected.  Otherwise true (success)
	* 
	* @note The server Sends should share the same inputFrequencyTable
	* as the client for receiving. It's also true for the client sending and the server receiving. 
	*/
	virtual bool GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer ) = 0;
	/**
	* Delete the output or input layer as specified.  This is not necessary to call and is only valuable for freeing memory
	* You should only call this when disconnected
	* @param inputLayer Delete the compression layer for sending or for receiving ? 
	* @return false (failure) if connected.  Otherwise true (success)
	*/
	virtual bool DeleteCompressionLayer( bool inputLayer ) = 0;
	/**
	* Register a C function as available for calling as a remote
	* procedure call uniqueID should be a null terminated non-case
	* senstive string of only letters to identify this procedure
	* Parameter 2 should be the name of the C function or C++ singleton
	* to be used as a function pointer This can be called whether the
	* client is active or not, and registered functions stay registered
	* unless unregistered with UnregisterAsRemoteProcedureCall
	* Only call offline
	* @param uniqueID the id of the RPC 
	* @param functionPointer a pointer to the C function. 
	*/
	virtual void RegisterAsRemoteProcedureCall( char* uniqueID, void ( *functionPointer ) ( RPCParameters *rpcParms ) ) = 0;

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
	virtual void RegisterClassMemberRPC( char* uniqueID, void *functionPointer ) = 0;

	/**
	* Unregisters a C function as available for calling as a remote procedure call that was formerly registered
	* with RegisterAsRemoteProcedureCall
	* Only call offline
	* @param uniqueID the id of the RPC. 
	*/
	virtual void UnregisterAsRemoteProcedureCall( char* uniqueID ) = 0;
	/**
	* Calls a C function on the server that the server already
	* registered using RegisterAsRemoteProcedureCall Pass the data you
	* want to pass to that function in parameters, or 0 for no data to
	* pass You can also pass a regular data stream which will be
	* converted to a bitstream internally by passing data and bit
	* length If you want that function to return data you should call
	* RPC from that system in the same way Returns true on a successful
	* packet send (this does not indicate the recipient performed the
	* call), false on failure The uniqueID must be composed of a string
	* with only characters from a-z and is not case sensitive
	* @param objectID For static functions, pass UNASSIGNED_OBJECT_ID.  For member functions, you must derive from NetworkIDGenerator and pass the value returned by NetworkIDGenerator::GetNetworkID for that object.
	*/
	virtual bool RPC( char* uniqueID, const char *data, unsigned int bitLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool shiftTimestamp, ObjectID objectID ) = 0;
	/**
	* Calls a C function on the server that the server already
	* registered using RegisterAsRemoteProcedureCall Pass the data you
	* want to pass to that function in parameters, or 0 for no data to
	* pass You can also pass a regular data stream which will be
	* converted to a bitstream internally by passing data and bit
	* length If you want that function to return data you should call
	* RPC from that system in the same way Returns true on a successful
	* packet send (this does not indicate the recipient performed the
	* call), false on failure The uniqueID must be composed of a string
	* with only characters from a-z and is not case sensitive
	* @param objectID For static functions, pass UNASSIGNED_OBJECT_ID.  For member functions, you must derive from NetworkIDGenerator and pass the value returned by NetworkIDGenerator::GetNetworkID for that object.
	*/
	virtual bool RPC( char* uniqueID, RakNet::BitStream *parameters, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool shiftTimestamp, ObjectID objectID ) = 0;

	// OBSOLETE - DONE AUTOMATICALLY
	// Handles an RPC packet.  If you get a packet with the ID ID_RPC you should pass it to this function
	// This is already done in Multiplayer.cpp, so if you use the Multiplayer class it is handled for you.
	// Returns true on success, false on a bad packet or an unregistered function
	// virtual bool HandleRPCPacket(Packet* packet)=0;

	/**
	* Enables or disables frequency table tracking.  This is required
	* to get a frequency table, which is used to generate A new
	* compression layer.  You can call this at any time - however you
	* SHOULD only call it when disconnected.  Otherwise you will only
	* track part of the values sent over the network. This value
	* persists between connect calls and defaults to false (no
	* frequency tracking)
	* @param b true to unable tracking. 
	*/
	virtual void SetTrackFrequencyTable( bool b ) = 0;

	/**
	* Returns the frequency of outgoing bytes into outputFrequencyTable
	* The purpose is to save to file as either a master frequency table
	* from a sample game session for passing to
	* GenerateCompressionLayer.  You should only call this when
	* disconnected.  Requires that you first enable data frequency
	* tracking by calling SetTrackFrequencyTable(true) 
	* @param outputFrequencyTable The frequency table produce during the tracking time. 
	* @return false (failure) if connected or if frequency table tracking is
	* not enabled.  Otherwise true (success)
	*/
	virtual bool GetSendFrequencyTable( unsigned int outputFrequencyTable[ 256 ] ) = 0;
	/**
	* Returns the compression ratio.  A low compression ratio is good.  Compression is for outgoing data
	* @return the current compression ratio 
	*/
	virtual float GetCompressionRatio( void ) const = 0;
	/**
	* Returns the decompression ratio.  A high decompression ratio is good.  Decompression is for incoming data
	* @return the current decompression ratio 
	*/
	virtual float GetDecompressionRatio( void ) const = 0;
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

	/**
	* The server internally maintains a data struct that is
	* automatically sent to clients when the connect. This is useful
	* to contain data such as the server name or message of the day.
	* Access that struct with this function.  The data is entered as an
	* array and stored and returned as a BitStream.  Everytime you call
	* GetStaticServerData it resets the read pointer to the start of
	* the bitstream.  To do multiple reads without reseting the pointer
	* Maintain a pointer copy to the bitstream as in RakNet::BitStream *copy = ...->GetStaticServerData(...);
	* To store a bitstream, use the GetData() and GetNumberOfBytesUsed() methods
	* of the bitstream for the 2nd and 3rd parameters
	* Note that the server may change at any time the
	* data contents and/or its length!
	* @return a bitstream containing statistics. 
	*/
	virtual RakNet::BitStream * GetStaticServerData( void ) = 0;
	/**
	* The server internally maintains a data struct that is
	* automatically sent to clients when the connect. This is useful
	* to contain data such as the server name or message of the day.
	* Access that struct with this function.  The data is entered as an
	* array and stored and returned as a BitStream.  Everytime you call
	* GetStaticServerData it resets the read pointer to the start of
	* the bitstream.  To do multiple reads without reseting the pointer
	* Maintain a pointer copy to the bitstream as in RakNet::BitStream *copy = ...->GetStaticServerData(...);
	* To store a bitstream, use the GetData() and GetNumberOfBytesUsed() methods
	* of the bitstream for the 2nd and 3rd parameters
	* Note that the server may change at any time the
	* data contents and/or its length!
	* @param data a byte buffer containing statistical information. 
	* @param length the size of @em data 
	*/
	virtual void SetStaticServerData( const char *data, const long length ) = 0;
	/**
	* The client internally maintains a data struct that is automatically sent to the server on connection
	* This is useful to contain data such as the player name. Access that struct with this
	* function. Pass UNASSIGNED_PLAYER_ID for playerId to reference your internal data.  A playerId value to access the data of another player.
	* *** NOTE ***
	* If you change any data in the struct the server won't reflect this change unless you manually update it
	* Do so by calling SendStaticClientDataToServer
	* The data is entered as an array and stored and returned as a BitStream.
	* Everytime you call GetStaticServerData it resets the read pointer to the start of the bitstream.  To do multiple reads without reseting the pointer
	* Maintain a pointer copy to the bitstream as in
	* RakNet::BitStream *copy = ...->GetStaticServerData(...);
	* To store a bitstream, use the GetData() and GetNumberOfBytesUsed() methods
	* of the bitstream for the 2nd and 3rd parameters
	*/
	virtual RakNet::BitStream * GetStaticClientData( PlayerID playerId ) = 0;
	/**
	* Set Local statistical information for playId. Call this
	* function when you receive statistical information from a
	* client.
	*
	* @param playerId the player ID 
	* @param data the packet data 
	* @param length the size of the data 
	*/
	virtual void SetStaticClientData( PlayerID playerId, const char *data, const long length ) = 0;
	/**
	* Send the static server data to the server The only time you need
	* to call this function is to update clients that are already
	* connected when you change the static server data by calling
	* GetStaticServerData and directly modifying the object pointed to.
	* Obviously if the connected clients don't need to know the new
	* data you don't need to update them, so it's up to you The server
	* must be active for this to have meaning
	*/
	virtual void SendStaticClientDataToServer( void ) = 0;
	/**
	* Return the player number of the server.
	* @return the server playerID 
	*/
	virtual PlayerID GetServerID( void ) const = 0;
	/**
	* Return the player number the server has assigned to you.
	* 
	* @return our player ID 
	* @note that unlike in previous versions, this is a struct and is not sequential
	* 
	*/
	virtual PlayerID GetPlayerID( void ) const = 0;
	/**
	* Returns the dotted IP address for the specified playerId
	*
	* @param playerId Any player ID other than UNASSIGNED_PLAYER_ID,
	* even if that player is not currently connected
	* @return a dotted notation string representation of the address of playerId. 
	*/
	virtual const char* PlayerIDToDottedIP( PlayerID playerId ) const = 0;
	/**
	* Put a packet back at the end of the receive queue in case you don't want to deal with it immediately
	* @param packet the packet to delayed 
	*/
	virtual void PushBackPacket( Packet *packet ) = 0;
	/**
	* Change the MTU size in order to improve performance when sending large packets
	* This can only be called when not connected.
	* Returns false on failure (we are connected).  True on success.  Maximum allowed size is MAXIMUM_MTU_SIZE
	* A too high of value will cause packets not to arrive at worst and be fragmented at best.
	* A too low of value will split packets unnecessarily.
	* Set according to the following table:
	* 1500. The largest Ethernet packet size; it is also the default value.
	* This is the typical setting for non-PPPoE, non-VPN connections. The default value for NETGEAR routers, adapters and switches. 
	* 1492. The size PPPoE prefers. 
	* 1472. Maximum size to use for pinging. (Bigger packets are fragmented.) 
	* 1468. The size DHCP prefers. 
	* 1460. Usable by AOL if you don't have large email attachments, etc. 
	* 1430. The size VPN and PPTP prefer. 
	* 1400. Maximum size for AOL DSL. 
	* 576. Typical value to connect to dial-up ISPs. (Default)
	*/
	virtual bool SetMTUSize( int size ) = 0;
	/**
	* Returns the current MTU size
	*/
	virtual int GetMTUSize( void ) const = 0;
	/**
	* Allow or disallow connection responses from any IP.  Normally this should be false, but may be necessary
	* when connection to servers with multiple IP addresses.
	*
	*
	* @param allow True to allow this behavior, false to not allow.
	* Defaults to false.  Value persists between connections
	*/
	virtual void AllowConnectionResponseIPMigration( bool allow ) = 0;
	/**
	* Sends a one byte message ID_ADVERTISE_SYSTEM to the remote unconnected system.
	* This will tell the remote system our external IP outside the LAN, and can be used for NAT punch through
	*
	* @param host Either a dotted IP address or a domain name
	* @param remotePort Which port to connect to on the remote machine.
	* @param data Optional data to append to the packet.
	* @param dataLength length of data in bytes.  Use 0 if no data.
	*/
	virtual void AdvertiseSystem( char *host, unsigned short remotePort, const char *data, int dataLength ) = 0;
	/**
	* Returns a structure containing a large set of network statistics for the server/client connection
	* You can map this data to a string using the C style StatisticsToString function
	*
	* @return 0 on can't find the specified system.  A pointer to a set of data otherwise.
	*/
	virtual RakNetStatisticsStruct * const GetStatistics( void ) = 0;
	/**
	* @internal 
	* Retrieve the player index corresponding to this client. 
	*/
	virtual PlayerIndex GetPlayerIndex( void ) = 0;
};

#endif
