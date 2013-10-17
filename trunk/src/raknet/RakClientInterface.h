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
	virtual bool Send( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel ) = 0;
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
	* Returns true if the client is connected to a responsive server
	* @return true if connected to a server
	*/
	virtual bool IsConnected( void ) const = 0;

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
