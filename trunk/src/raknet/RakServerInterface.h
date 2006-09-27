/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief User visible interface of RakServer 
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

#ifndef __RAK_SERVER_INTERFACE_H
#define __RAK_SERVER_INTERFACE_H

#include "NetworkTypes.h"
#include "PacketPriority.h"
#include "RakPeerInterface.h"
#include "BitStream.h"
#include "RakNetStatistics.h" 
/**
 * @brief Visible Interface of RakServer
 * 
 * This class define the features available to the user when he want
 * to use server interface.
 */

class RakServerInterface
{

public:
	/**
	 * Destructor
	 */
	
	virtual ~RakServerInterface()	{}
	
	/**
	 * Call this to initiate the server with the number of players you want to be allowed connected at once
	 * @param AllowedPlayers Current maximum number of allowed players is 65535
	 * @param depreciated is for legacy purposes and is unused
	 * @param threadSleepTimer >=0 for how many ms to Sleep each internal update cycle (recommended 30 for low performance, 0 for regular)
	 * @param port is the port you want the server to read and write on
	 * Make sure this port is open for UDP
	 * @param forceHostAddress Can force RakNet to use a particular IP to host on.  Pass 0 to automatically pick an IP
	 * @return true on successful initiation, false otherwise
	 */
	virtual bool Start( unsigned short AllowedPlayers, unsigned int depreciated, int threadSleepTimer, unsigned short port, const char *forceHostAddress=0 ) = 0;
	
	/**
	 * Must be called while offline
	 * Secures connections though a combination of SHA1, AES128, SYN Cookies, and RSA to prevent
	 * connection spoofing, replay attacks, data eavesdropping, packet tampering, and MitM attacks.
	 * There is a significant amount of processing and a slight amount of bandwidth
	 * overhead for this feature.
	 *
	 * If you accept connections, you must call this or else secure connections will not be enabled
	 * for incoming connections. If the private keys are 0, then a new key will be generated when this function is called
	 *
	 * @param pubKeyE A pointer to the public keys from the RSACrypt class.  
	 * @param  pubKeyN A pointer to the public keys from the RSACrypt class. 
	 * @see the Encryption sample
	 * 
	 */
	virtual void InitializeSecurity( const char *pubKeyE, const char *pubKeyN ) = 0;
	
	/**
	 * Must be called while offline
	 * Disables all security.
	 */
	virtual void DisableSecurity( void ) = 0;

	/**
	 * Set the password clients have to use to connect to this server. The password persists between connections.
	 * Pass 0 for no password.
	 * You can call this anytime
	 * @param _password The password name. 
	 */
	virtual void SetPassword( const char *_password ) = 0;
	
	/**
	 * Returns true if a password was set, false otherwise
	 */
	virtual bool HasPassword( void ) = 0;
	
	/**
	 * Stops the server, stops synchronized data, and resets all internal data.  This will drop all players currently connected, however
	 * since the server is stopped packet reliability is not enforced so the Kick network message may not actually
	 * arrive. Those players will disconnect due to timeout. If you want to end the server more gracefully, you
	 * can manually Kick each player first. Does nothing if the server is not running to begin with
	 *
	 * @param blockDuration How long you should wait for all remaining packets to go out, per connected system
	 * If you set it to 0 then the disconnection notifications probably won't arrive
	 */
	virtual void Disconnect( unsigned int blockDuration ) = 0;
	
	/**
	 * This function only works while the server is active (Use the Start function).  Returns false on failure, true on success
	 * Send the data stream of length length to whichever playerId you specify.  Specify UNASSIGNED_PLAYER_ID for all players connected
	 * If you aren't sure what to specify for priority and reliability, use HIGH_PRIORITY, RELIABLE, 0 for ordering channel
	 * Set broadcast to true to broadcast to all connected clients EXCEPT the one specified in the playerId field.
	 * To broadcast to everyone specify UNASSIGNED_PLAYER_ID for the playerId field.
	 */
	virtual bool Send( const char *data, const long length, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast ) = 0;
	
	/**
	 * This function only works while the server is active (Use the Start function).  Returns false on failure, true on success
	 * Send the bitstream to whichever playerId you specify.
	 * You can set the first byte to a packet identifier, however you will need to have TYPE_CHECKING undefined or the internal type checking
	 * will add extra data and make this not work.  If you want TYPE_CHECKING on, you will need to use BitStream::WriteBits to avoid the type checking.
	 * This interface will probably change to fix this in future versions.
	 * If you aren't sure what to specify for priority and reliability, use HIGH_PRIORITY and RELIABLE, 0 for ordering channel
	 * Set broadcast to true to broadcast to all connected clients EXCEPT the one specified in the playerId field.
	 * To broadcast to everyone specify UNASSIGNED_PLAYER_ID for the playerId field.
	 */
	virtual bool Send( RakNet::BitStream *bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast ) = 0;
	
	/**
	 * Call this to get a packet from the incoming packet queue.  Use DeallocatePacket to deallocate the packet after you are done with it.
	 * Check the Packet struct at the top of CoreNetworkStructures.h for the format of the struct
	 * Returns 0 if no packets are waiting to be handled
	 * If the server is not active this will also return 0, as all waiting packets are flushed when the server is Disconnected
	 * This also updates all memory blocks associated with synchronized memory
	 */
	virtual Packet* Receive( void ) = 0;
	
	/**
	 * Kick out the specified player.
	 */
	virtual void Kick( PlayerID playerId ) = 0;
	
	/**
	 * Call this to deallocate a packet returned by Receive when you are done handling it.
	 */
	virtual void DeallocatePacket( Packet *packet ) = 0;
	
	/**
	 * Set how many players are allowed on the server. If more players are currently connected then are allowed then
	 * No more players will be allowed to join until the number of players is less than the number of allowed players
	 * The server must be active for this to have meaning
	 */
	virtual void SetAllowedPlayers( unsigned short AllowedPlayers ) = 0;
	
	/**
	 * Return how many players are allowed to connect. This value was set either from Start or from SetAllowedPlayers
	 * The server must be active for this to have meaning
	 */
	virtual unsigned short GetAllowedPlayers( void ) const = 0;
	
	/**
	 * Return how many players are currently connected to the server.
	 * The server must be active for this to have meaning
	 */
	virtual unsigned short GetConnectedPlayers( void ) = 0;

	/**
	 * Returns a static string pointer containing the IP of the specified connected player.
	 * Also returns the client port
	 * This changes between calls so be sure to copy what is returned if you need to retain it
	 * Useful for creating and maintaining ban lists
	 * The server must be active for this to have meaning
	 * If the specified id does not represent an active player the results are undefined (most likely returns 0)
	 */
	virtual void GetPlayerIPFromID( PlayerID playerId, char returnValue[ 22 ], unsigned short *port ) = 0;
	
	/**
	 * Send a ping request to the specified player
	 */
	virtual void PingPlayer( PlayerID playerId ) = 0;
	
	/**
	 * Returns the average of all ping times read for the specific player or -1 if none read yet
	 */
	virtual int GetAveragePing( PlayerID playerId ) = 0;
	
	/**
	 * Returns the last ping time read for the specific player or -1 if none read yet
	 */
	virtual int GetLastPing( PlayerID playerId ) = 0;
	
	/**
	 * Returns the lowest ping time read or -1 if none read yet
	 */
	virtual int GetLowestPing( PlayerID playerId ) = 0;
	
	/**
	 * Ping all players every so often.  This is on by default.  In games where you don't care about ping you can call
	 * StopOccasionalPing to save the bandwidth
	 * This will work anytime
	 */
	virtual void StartOccasionalPing( void ) = 0;
	
	/**
	 * Stop pinging players every so often.  Players are pinged by default.  In games where you don't care about ping
	 * you can call this to save the bandwidth
	 * This will work anytime
	 */
	virtual void StopOccasionalPing( void ) = 0;
	
	/**
	 * Returns true if the server is currently active
	 */
	virtual bool IsActive( void ) const = 0;
	
	/**
	 * Returns a number automatically synchronized between the server and client which randomly changes every
	 * 9 seconds. The time it changes is accurate to within a few ms and is best used to seed
	 * random number generators that you want to usually return the same output on all systems.  Keep in mind this
	 * isn't perfectly accurate as there is always a very small chance the numbers will by out of synch during
	 * changes so you should confine its use to visual effects or functionality that has a backup method to
	 * maintain synchronization.  If you don't need this functionality and want to save the bandwidth call
	 * StopSynchronizedRandomInteger after starting the server
	 */
	virtual unsigned int GetSynchronizedRandomInteger( void ) const = 0;
	
	/**
	 * Start or restart the synchronized random integer.  This is on by default.  Call StopSynchronizedRandomInteger
	 * to stop it keeping the number in synch
	 */
	virtual void StartSynchronizedRandomInteger( void ) = 0;
	
	/**
	 * Stop the synchronized random integer.  Call StartSynchronizedRandomInteger to start it again
	 */
	virtual void StopSynchronizedRandomInteger( void ) = 0;
	
	/**
	 * This is an optional function to generate the compression layer from the input frequency table.
	 * You should call this twice - once with inputLayer as true and once as false.
	 * The frequency table passed here with inputLayer=true should match the frequency table on the recipient with inputLayer=false.
	 * Likewise, the frequency table passed here with inputLayer=false should match the frequency table on the recipient with inputLayer=true
	 * Calling this function when there is an existing layer will overwrite the old layer
	 * You should only call this when disconnected
	 * @return false (failure) if connected.  Otherwise true (success)
	 */
	virtual bool GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer ) = 0;
	
	/**
	 * Delete the output or input layer as specified.  This is not necessary to call and is only valuable for freeing memory
	 * You should only call this when disconnected
	 * @return false (failure) if connected.  Otherwise true (success)
	 */
	virtual bool DeleteCompressionLayer( bool inputLayer ) = 0;
	
	/**
	 * @ingroup RAKNET_RPC
	 * Register a C or static member function as available for calling as a remote procedure call
	 * 
	 * @param uniqueID: A string of only letters to identify this procedure.  Recommended you use the macro CLASS_MEMBER_ID for class member functions
	 * @param functionPointer(...): The name of the function to be used as a function pointer
	 * This can be called whether the client is active or not, and registered functions stay registered unless unregistered with
	 * UnregisterAsRemoteProcedureCall
	 * Only call offline
	 * @note This is part of the Remote Procedure Call Subsystem 
	 *
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
	virtual void UnregisterAsRemoteProcedureCall( char* uniqueID ) = 0;
	
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
	 * @param broadcast True to send this packet to all connected systems.  If true, then playerId specifies who not to send the packet to.
	 * @param shiftTimestamp True to treat the first 4 bytes as a timestamp and make it system relative on arrival (Same as ID_TIMESTAMP for a packet enumeration type)
	 * @param objectID For static functions, pass UNASSIGNED_OBJECT_ID.  For member functions, you must derive from NetworkIDGenerator and pass the value returned by NetworkIDGenerator::GetNetworkID for that object.
	 * @return True on a successful packet send (this does not indicate the recipient performed the call), false on failure
	 * @note This is part of the Remote Procedure Call Subsystem 
	 */
	virtual bool RPC( char* uniqueID, const char *data, unsigned int bitLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool shiftTimestamp, ObjectID objectID ) = 0;
	
	/**
	 * @ingroup RAKNET_RPC 
	 * Calls a C function on the server that the server already registered using RegisterAsRemoteProcedureCall
	 * If you want that function to return data you should call RPC from that system in the same way
	 * Returns true on a successful packet send (this does not indicate the recipient performed the call), false on failure
	 *
	 * @param uniqueID A string of only letters to identify this procedure.  Recommended you use the macro CLASS_MEMBER_ID for class member functions.  Must match the parameter
	 * @param parameters The bitstream to send
	 * @param priority What priority level to send on.
	 * @param reliability How reliability to send this data
	 * @param orderingChannel When using ordered or sequenced packets, what channel to order these on.
	 * @param playerId Who to send this packet to, or in the case of broadcasting who not to send it to.  Use UNASSIGNED_PLAYER_ID to specify none
	 * @param broadcast True to send this packet to all connected systems.  If true, then playerId specifies who not to send the packet to.
	 * @param shiftTimestamp True to treat the first 4 bytes as a timestamp and make it system relative on arrival (Same as ID_TIMESTAMP for a packet enumeration type)
	 * @param objectID For static functions, pass UNASSIGNED_OBJECT_ID.  For member functions, you must derive from NetworkIDGenerator and pass the value returned by NetworkIDGenerator::GetNetworkID for that object.
	 * @return True on a successful packet send (this does not indicate the recipient performed the call), false on failure
	 * @note This is part of the Remote Procedure Call Subsystem 
	 */
	virtual bool RPC( char* uniqueID, RakNet::BitStream *parameters, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool shiftTimestamp, ObjectID objectID ) = 0;
	
	// OBSOLETE - DONE AUTOMATICALLY
	// Handles an RPC packet.  If you get a packet with the ID ID_RPC you should pass it to this function
	// This is already done in Multiplayer.cpp, so if you use the Multiplayer class it is handled for you.
	// Returns true on success, false on a bad packet or an unregistered function
	// virtual bool HandleRPCPacket(Packet* packet)=0;
	
	/**
	 * Enables or disables frequency table tracking.  This is required to get a frequency table, which is used to generate
	 * A new compression layer.
	 * You can call this at any time - however you SHOULD only call it when disconnected.  Otherwise you will only track
	 * part of the values sent over the network.
	 * This value persists between connect calls and defaults to false (no frequency tracking)
	 * @param b True to enable tracking 
	 */
	virtual void SetTrackFrequencyTable( bool b ) = 0;
	
	/**
	 * Returns the frequency of outgoing bytes into outputFrequencyTable
	 * The purpose is to save to file as either a master frequency table from a sample game session for passing to
	 * GenerateCompressionLayer.
	 * You should only call this when disconnected
	 * Requires that you first enable data frequency tracking by calling SetTrackFrequencyTable(true)
	 * @param[out] outputFrequencyTable The Frequency Table used in the compression layer 
	 * @return false (failure) if connected or if frequency table tracking is not enabled.  Otherwise true (success)
	 */
	virtual bool GetSendFrequencyTable( unsigned int outputFrequencyTable[ 256 ] ) = 0;
	
	/**
	 * Returns the compression ratio. A low compression ratio is good.  Compression is for outgoing data
	 */
	virtual float GetCompressionRatio( void ) const = 0;
	
	/**
	 * Returns the decompression ratio.  A high decompression ratio is good.  Decompression is for incoming data
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
	 * The server internally maintains a data struct that is automatically sent to clients when the connect.
	 * This is useful to contain data such as the server name or message of the day.  Access that struct with this
	 * function.
	 * @note 
	 * If you change any data in the struct remote clients won't reflect this change unless you manually update them
	 * Do so by calling SendStaticServerDataToClient(UNASSIGNED_PLAYER_ID) (broadcast to all)
	 * The data is entered as an array and stored and returned as a BitStream.
	 * To store a bitstream, use the GetData() and GetNumberOfBytesUsed() methods
	 * of the bitstream for the 2nd and 3rd parameters
	 */
	virtual RakNet::BitStream * GetStaticServerData( void ) = 0;
	
	/**
	 * The server internally maintains a data struct that is automatically sent to clients when the connect.
	 * This is useful to contain data such as the server name or message of the day.  Access that struct with this
	 * function.
	 * @note 
	 * If you change any data in the struct remote clients won't reflect this change unless you manually update them
	 * Do so by calling SendStaticServerDataToClient(UNASSIGNED_PLAYER_ID) (broadcast to all)
	 * The data is entered as an array and stored and returned as a BitStream.
	 * To store a bitstream, use the GetData() and GetNumberOfBytesUsed() methods
	 * of the bitstream for the 2nd and 3rd parameters
	 */
	virtual void SetStaticServerData( const char *data, const long length ) = 0;
	
	/**
	 * This sets to true or false whether we want to support relaying of static client data to other connected clients.
	 * If set to false it saves some bandwdith, however other clients won't know the static client data and attempting
	 * to read that data will return garbage.  Default is true.  This also only works for up to 32 players.  Games
	 * supporting more than 32 players will have this shut off automatically upon server start and must have it forced
	 * back on with this function if you do indeed want it
	 * This should be called after the server is started in case you want to override when it shuts off at 32 players
	 */
	virtual void SetRelayStaticClientData( bool b ) = 0;
	
	/**
	 * Send the static server data to the specified player.  Pass UNASSIGNED_PLAYER_ID to broadcast to all players
	 * The only time you need to call this function is to update clients that are already connected when you change the static
	 * server data by calling GetStaticServerData and directly modifying the object pointed to.  Obviously if the
	 * connected clients don't need to know the new data you don't need to update them, so it's up to you
	 * The server must be active for this to have meaning
	 */
	virtual void SendStaticServerDataToClient( PlayerID playerId ) = 0;
	
	/**
	 * Sets the data to send with an (LAN server discovery) /(offline ping) response
	 * Length should be under 400 bytes, as a security measure against flood attacks
	 * @see the Ping sample project for how this is used.
	 * @param data a block of data to store, or 0 for none
	 * @param length The length of data in bytes, or 0 for none
	 */
	virtual void SetOfflinePingResponse( const char *data, const unsigned int length ) = 0;
	
	/**
	 * Returns a pointer to an attached client's character name specified by the playerId
	 * Returns 0 if no such player is connected
	 * Note that you can modify the client data here.  Changes won't be reflected on clients unless you force them to
	 * update by calling ChangeStaticClientData
	 * The server must be active for this to have meaning
	 * The data is entered as an array and stored and returned as a BitStream. 
	 * Everytime you call GetStaticServerData it resets the read pointer to the start of the bitstream.  To do multiple reads without reseting the pointer
	 * Maintain a pointer copy to the bitstream as in
	 * RakNet::BitStream *copy = ...->GetStaticServerData(...);
	 * To store a bitstream, use the GetData() and GetNumberOfBytesUsed() methods
	 * of the bitstream for the 2nd and 3rd parameters
	 * Note that the client may change at any time the
	 * data contents and/or its length!
	 * @param playerId The ID of the client 
	 * @return Statistical information of this client 
	 */
	virtual RakNet::BitStream * GetStaticClientData( PlayerID playerId ) = 0;
	
	/**
	 * Returns a pointer to an attached client's character name specified by the playerId
	 * Returns 0 if no such player is connected
	 * Note that you can modify the client data here.  Changes won't be reflected on clients unless you force them to
	 * update by calling ChangeStaticClientData
	 * The server must be active for this to have meaning
	 * The data is entered as an array and stored and returned as a BitStream. 
	 * Everytime you call GetStaticServerData it resets the read pointer to the start of the bitstream.  To do multiple reads without reseting the pointer
	 * Maintain a pointer copy to the bitstream as in
	 * RakNet::BitStream *copy = ...->GetStaticServerData(...);
	 * To store a bitstream, use the GetData() and GetNumberOfBytesUsed() methods
	 * of the bitstream for the 2nd and 3rd parameters
	 * Note that the client may change at any time the
	 * data contents and/or its length!
	 * @param playerId The ID of the client 
	 * @param data A buffer containing statistics 
	 * @param length The size of the buffer 
	 */
	virtual void SetStaticClientData( PlayerID playerId, const char *data, const long length ) = 0;
	
	/**
	 * This function is used to update the information on connected clients when the server effects a change
	 * of static client data
	 * playerChangedId should be the playerId of the player whose data was changed.  This is the parameter passed to
	 * GetStaticClientData to get a pointer to that player's information
	 * Note that a client that gets this new information for himself will update the data for his playerID but not his local data (which is the user's copy)
	 * i.e. player 5 would have the data returned by GetStaticClientData(5) changed but his local information returned by
	 * GetStaticClientData(UNASSIGNED_PLAYER_ID) would remain the same as it was previously.
	 * playerToSendToId should be the player you want to update with the new information.  This will almost always
	 * be everybody, in which case you should pass UNASSIGNED_PLAYER_ID.
	 * The server must be active for this to have meaning
	 */
	virtual void ChangeStaticClientData( PlayerID playerChangedId, PlayerID playerToSendToId ) = 0;
	
	/**
	 * Internally store the IP address(es) for the server and return how many it has.
	 * This can easily be more than one, for example a system on both a LAN and with a net connection.
	 * The server does not have to be active for this to work
	 */
	virtual unsigned int GetNumberOfAddresses( void ) = 0;
	
	/**
	 * Call this function where 0 <= index < x where x is the value returned by GetNumberOfAddresses
	 * Returns a static string filled with the server IP of the specified index
	 * Strings returned in no particular order.  You'll have to check every index see which string you want
	 * Returns 0 on invalid input
	 * The server does not have to be active for this to work
	 */
	virtual const char* GetLocalIP( unsigned int index ) = 0;
	
	/**
	 * Put a packet back at the end of the receive queue in case you don't want to deal with it immediately
	 */
	virtual void PushBackPacket( Packet *packet ) = 0;
	
	/**
	 * Given a playerID, returns an index from 0 to the maximum number of players allowed - 1.
	 */
	virtual int GetIndexFromPlayerID( PlayerID playerId ) = 0;
	
	/**
	 * This function is only useful for looping through all players.
	 * Index should range between 0 and the maximum number of players allowed - 1.
	 */
	virtual PlayerID GetPlayerIDFromIndex( int index ) = 0;
	
	/**
	 * Bans an IP from connecting.  Banned IPs persist between connections.
	 *
	 * Parameters
	 * IP - Dotted IP address. Can use * as a wildcard, such as 128.0.0.* will ban
	 * All IP addresses starting with 128.0.0
	 */
	virtual void AddToBanList( const char *IP ) = 0;
	
	/**
	 * Allows a previously banned IP to connect.
	 *
	 * Parameters
	 * IP - Dotted IP address. Can use * as a wildcard, such as 128.0.0.* will ban
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
	 * Parameters
	 * IP - Complete dotted IP address
	 *
	 * Returns
	 * True if IP matches any IPs in the ban list, accounting for any wildcards.
	 * False otherwise.
	 */
	virtual bool IsBanned( const char *IP ) = 0;
	
	/**
	 * Returns true if that player ID is currently used
	 */
	virtual bool IsActivePlayerID( PlayerID playerId ) = 0;
	
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
	 * Sends a one byte message ID_ADVERTISE_SYSTEM to the remote unconnected system.
	 * This will tell the remote system our external IP outside the LAN, and can be used for NAT punch through
	 *
	 * host: Either a dotted IP address or a domain name
	 * remotePort: Which port to connect to on the remote machine.
	 */
	virtual void AdvertiseSystem( char *host, unsigned short remotePort, const char *data, int dataLength ) = 0;
	
	/**
	 * Returns a structure containing a large set of network statistics for the specified system
	 * You can map this data to a string using the C style StatisticsToString function
	 *
	 * Parameters
	 * playerId: Which connected system to get statistics for
	 *
	 * Returns:
	 * 0 on can't find the specified system.  A pointer to a set of data otherwise.
	 */
	virtual RakNetStatisticsStruct * const GetStatistics( PlayerID playerId ) = 0;
};

#endif
