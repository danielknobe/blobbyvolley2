/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief A RakServer provide Server facilities
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

#ifndef __RAK_SERVER_H
#define __RAK_SERVER_H

#include "RakPeer.h"

/**
* @brief Server Facilities on top of RakPeer
*
* This class provide an increase level of services on top of
* RakPeer. The communication end point can now only accept connexion
* and exchange message with peer connected to him
*
*@see RakClient
*
*/

class RakServer : public RakPeer
{

public:
	/**
	* Constructor
	*/
	RakServer();

	/**
	* Destructor
	*/
	virtual ~RakServer();

	/**
	* Call this to initiate the server with the number of players you want to be allowed connected at once
	* @param AllowedPlayers Current maximum number of allowed players is 65535
	* @param threadSleepTimer >=0 for how many ms to Sleep each internal update cycle (recommended 30 for low performance, 0 for regular)
	* @param port is the port you want the server to read and write on
	* Make sure this port is open for UDP
	* @param forceHostAddress Can force RakNet to use a particular IP to host on. Pass 0 to automatically pick an IP
	* @return true on successful initiation, false otherwise
	*/
	bool Start( unsigned short AllowedPlayers, int threadSleepTimer, unsigned short port, const char *forceHostAddress=0 );
	/**
	* Stops the server, stops synchronized data, and resets all internal data.  This will drop all players currently connected, however
	* since the server is stopped packet reliability is not enforced so the Kick network message may not actually
	* arrive. Those players will disconnect due to timeout. If you want to end the server more gracefully, you
	* can manually Kick each player first. Does nothing if the server is not running to begin with
	*
	* @param blockDuration How long you should wait for all remaining packets to go out, per connected system
	* If you set it to 0 then the disconnection notifications probably won't arrive
	*/
	void Disconnect( unsigned int blockDuration );
	/**
	* This function only works while the server is active (Use the Start function).  Returns false on failure, true on success
	* Send the data stream of length length to whichever playerId you specify.  Specify UNASSIGNED_PLAYER_ID for all players connected
	* If you aren't sure what to specify for priority and reliability, use HIGH_PRIORITY, RELIABLE, 0 for ordering channel
	* Set broadcast to true to broadcast to all connected clients EXCEPT the one specified in the playerId field.
	* To broadcast to everyone specify UNASSIGNED_PLAYER_ID for the playerId field.
	*/
	bool Send( const char *data, const long length, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast );
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
	bool Send( const RakNet::BitStream *bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast );
	/**
	* Call this to get a packet from the incoming packet queue.  Use DeallocatePacket to deallocate the packet after you are done with it.
	* Check the Packet struct at the top of CoreNetworkStructures.h for the format of the struct
	* Returns 0 if no packets are waiting to be handled
	* If the server is not active this will also return 0, as all waiting packets are flushed when the server is Disconnected
	* This also updates all memory blocks associated with synchronized memory
	*/
	packet_ptr Receive( void );
	/**
	* Kick out the specified player.
	*/
	void Kick( PlayerID playerId );
	/**
	* Set how many players are allowed on the server. If more players are currently connected then are allowed then
	* No more players will be allowed to join until the number of players is less than the number of allowed players
	* The server must be active for this to have meaning
	*/
	void SetAllowedPlayers( unsigned short AllowedPlayers );
	/**
	* Return how many players are allowed to connect. This value was set either from Start or from SetAllowedPlayers
	* The server must be active for this to have meaning
	*/
	unsigned short GetAllowedPlayers( void ) const;
	/**
	* Return how many players are currently connected to the server.
	* The server must be active for this to have meaning
	*/
	unsigned short GetConnectedPlayers( void );
	/**
	* Returns a static string pointer containing the IP of the specified connected player.
	* Also returns the client port
	* This changes between calls so be sure to copy what is returned if you need to retain it
	* Useful for creating and maintaining ban lists
	* The server must be active for this to have meaning
	* If the specified id does not represent an active player the results are undefined (most likely returns 0)
	*/
	void GetPlayerIPFromID( PlayerID playerId, char returnValue[ 22 ], unsigned short *port );
	/**
	* Send a ping request to the specified player
	*/
	void PingPlayer( PlayerID playerId );
	/**
	* Returns the last ping time read for the specific player or -1 if none read yet
	*/
	int GetLastPing( PlayerID playerId );
	/**
	* Returns the lowest ping time read or -1 if none read yet
	*/
	int GetLowestPing( PlayerID playerId );
	/**
	* Returns true if the server is currently active
	*/
	bool IsActive( void ) const;

	/**
	* Internally store the IP address(es) for the server and return how many it has.
	* This can easily be more than one, for example a system on both a LAN and with a net connection.
	* The server does not have to be active for this to work
	*/
	unsigned int GetNumberOfAddresses( void );
	/**
	* Call this function where 0 <= index < x where x is the value returned by GetNumberOfAddresses
	* Returns a static string filled with the server IP of the specified index
	* Strings returned in no particular order.  You'll have to check every index see which string you want
	* Returns 0 on invalid input
	* The server does not have to be active for this to work
	*/
	const char* GetLocalIP( unsigned int index );
	/**
	* Put a packet back at the end of the receive queue in case you don't want to deal with it immediately
	*/
	void PushBackPacket( Packet *packet );
	/**
	* Given a playerID, returns an index from 0 to the maximum number of players allowed - 1.
	*/
	int GetIndexFromPlayerID( PlayerID playerId );
	/**
	* This function is only useful for looping through all players.
	* Index should range between 0 and the maximum number of players allowed - 1.
	*/
	PlayerID GetPlayerIDFromIndex( int index );

	/**
	* Returns true if that player ID is currently used
	*/
	bool IsActivePlayerID( PlayerID playerId );
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
	bool SetMTUSize( int size );
	/**
	* Returns the current MTU size
	*/
	int GetMTUSize( void ) const;

	/**
	* Sends a one byte message ID_ADVERTISE_SYSTEM to the remote unconnected system.
	* This will tell the remote system our external IP outside the LAN, and can be used for NAT punch through
	*
	* host: Either a dotted IP address or a domain name
	* remotePort: Which port to connect to on the remote machine.
	*/
	void AdvertiseSystem( char *host, unsigned short remotePort, const char *data, int dataLength );
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
	RakNetStatisticsStruct * const GetStatistics( PlayerID playerId );
};

#endif
