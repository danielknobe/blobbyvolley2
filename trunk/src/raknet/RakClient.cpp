/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Client communication End Point Implementation
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

#include "RakClient.h"
#include "PacketEnumerations.h"
#include "GetTime.h"

// Constructor
RakClient::RakClient()
{

	for (unsigned i = 0; i < 32; i++ )
		otherClients[ i ].isActive = false;
}

// Destructor
RakClient::~RakClient()
{}

#pragma warning( disable : 4100 ) // warning C4100: 'depreciated' : unreferenced formal parameter
bool RakClient::Connect( const char* host, unsigned short serverPort, unsigned short clientPort, unsigned int depreciated, int threadSleepTimer )
{
	RakPeer::Disconnect( 100 );

	RakPeer::Initialize( 1, clientPort, threadSleepTimer );

	if ( host[ 0 ] < '0' || host[ 0 ] > '2' )
	{
		host = ( char* ) SocketLayer::Instance()->DomainNameToIP( host );
	}

	unsigned i;

	for ( i = 0; i < 32; i++ )
	{
		otherClients[ i ].isActive = false;
		otherClients[ i ].playerId = UNASSIGNED_PLAYER_ID;
	}

	// ignore depreciated. A pointless variable
	return RakPeer::Connect( host, serverPort );
}

void RakClient::Disconnect( unsigned int blockDuration )
{
	RakPeer::Disconnect( blockDuration );
}

bool RakClient::Send( const char *data, const long length, PacketPriority priority, PacketReliability reliability, char orderingChannel )
{
	if ( remoteSystemList == 0 )
		return false;

	return RakPeer::Send( data, length, priority, reliability, orderingChannel, remoteSystemList[ 0 ].playerId, false );
}

bool RakClient::Send( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel )
{
	if ( remoteSystemList == 0 )
		return false;

	return RakPeer::Send( bitStream, priority, reliability, orderingChannel, remoteSystemList[ 0 ].playerId, false );
}

packet_ptr RakClient::Receive( void )
{
	packet_ptr packet = RakPeer::Receive();

	// Intercept specific client / server feature packets

	if ( packet )
	{
		RakNet::BitStream bitStream( ( char* ) packet->data, packet->length, false );
		int i;

		if ( packet->data[ 0 ] == ID_CONNECTION_REQUEST_ACCEPTED )
		{
			PlayerIndex playerIndex;

			RakNet::BitStream inBitStream((char*)packet->data, packet->length, false);
			inBitStream.IgnoreBits(8); // ID_CONNECTION_REQUEST_ACCEPTED
			inBitStream.IgnoreBits(8 * sizeof(unsigned short)); //inBitStream.Read(remotePort);
			inBitStream.IgnoreBits(8 * sizeof(unsigned int)); //inBitStream.Read(externalID.binaryAddress);
			inBitStream.IgnoreBits(8 * sizeof(unsigned short)); //inBitStream.Read(externalID.port);
			inBitStream.Read(playerIndex);

			localPlayerIndex = playerIndex;
			packet->playerIndex = playerIndex;
		}
		else if (
				packet->data[ 0 ] == ID_REMOTE_NEW_INCOMING_CONNECTION ||
				packet->data[ 0 ] == ID_REMOTE_EXISTING_CONNECTION ||
				packet->data[ 0 ] == ID_REMOTE_DISCONNECTION_NOTIFICATION ||
				packet->data[ 0 ] == ID_REMOTE_CONNECTION_LOST )
		{
			bitStream.IgnoreBits( 8 ); // Ignore identifier
			bitStream.Read( packet->playerId.binaryAddress );
			bitStream.Read( packet->playerId.port );

			if ( bitStream.Read( ( unsigned short& ) packet->playerIndex ) == false )
			{
				// no need to deallocate, shared_ptr will do that
				return packet_ptr();
			}


			if ( packet->data[ 0 ] == ID_REMOTE_DISCONNECTION_NOTIFICATION ||
				packet->data[ 0 ] == ID_REMOTE_CONNECTION_LOST )
			{
				i = GetOtherClientIndexByPlayerID( packet->playerId );

				if ( i >= 0 )
					otherClients[ i ].isActive = false;
			}
		}
		else if ( packet->data[ 0 ] == ID_BROADCAST_PINGS )
		{
			PlayerID playerId;
			int index;

			bitStream.IgnoreBits( 8 ); // Ignore identifier

			for ( i = 0; i < 32; i++ )
			{
				if ( bitStream.Read( playerId.binaryAddress ) == false )
					break; // No remaining data!

				bitStream.Read( playerId.port );

				index = GetOtherClientIndexByPlayerID( playerId );

				if ( index >= 0 )
					bitStream.Read( otherClients[ index ].ping );
				else
				{
					index = GetFreeOtherClientIndex();

					if ( index >= 0 )
					{
						otherClients[ index ].isActive = true;
						bitStream.Read( otherClients[ index ].ping );
						otherClients[ index ].playerId = playerId;
					}

					else
						bitStream.IgnoreBits( sizeof( short ) * 8 );
				}
			}

			// no need to deallocate, shared_ptr will do that
			return packet_ptr();
		}
	}

	return packet;
}

void RakClient::PingServer( void )
{
	if ( remoteSystemList == 0 )
		return ;

	RakPeer::Ping( remoteSystemList[ 0 ].playerId );
}

void RakClient::PingServer( const char* host, unsigned short serverPort, unsigned short clientPort, bool onlyReplyOnAcceptingConnections )
{
	// Must be 2 for linux systems
	RakPeer::Initialize( 2, clientPort, 10 );
	RakPeer::Ping( host, serverPort, onlyReplyOnAcceptingConnections );
}

int RakClient::GetLastPing( void ) const
{
	if ( remoteSystemList == 0 )
		return -1;

	return RakPeer::GetLastPing( remoteSystemList[ 0 ].playerId );
}

int RakClient::GetLowestPing( void ) const
{
	if ( remoteSystemList == 0 )
		return -1;

	return RakPeer::GetLowestPing( remoteSystemList[ 0 ].playerId );
}

int RakClient::GetPlayerPing( PlayerID playerId )
{
	int i;

	for ( i = 0; i < 32; i++ )
		if ( otherClients[ i ].playerId == playerId )
			return otherClients[ i ].ping;

	return -1;
}

bool RakClient::IsConnected( void ) const
{
	unsigned short numberOfSystems;

	RakPeer::GetConnectionList( 0, &numberOfSystems );
	return numberOfSystems == 1;
}

PlayerID RakClient::GetServerID( void ) const
{
	if ( remoteSystemList == 0 )
		return UNASSIGNED_PLAYER_ID;

	return remoteSystemList[ 0 ].playerId;
}

PlayerID RakClient::GetPlayerID( void ) const
{
	if ( remoteSystemList == 0 )
		return UNASSIGNED_PLAYER_ID;

	// GetExternalID is more accurate because it reflects our external IP and port to the server.
	// GetInternalID only matches the parameters we passed
	PlayerID myID = RakPeer::GetExternalID( remoteSystemList[ 0 ].playerId );

	if ( myID == UNASSIGNED_PLAYER_ID )
		return RakPeer::GetInternalID();
	else
		return myID;
}

const char* RakClient::PlayerIDToDottedIP( PlayerID playerId ) const
{
	return RakPeer::PlayerIDToDottedIP( playerId );
}

void RakClient::PushBackPacket( Packet *packet )
{
	RakPeer::PushBackPacket( packet );
}

bool RakClient::SetMTUSize( int size )
{
	return RakPeer::SetMTUSize( size );
}

int RakClient::GetMTUSize( void ) const
{
	return RakPeer::GetMTUSize();
}

void RakClient::AllowConnectionResponseIPMigration( bool allow )
{
	RakPeer::AllowConnectionResponseIPMigration( allow );
}

void RakClient::AdvertiseSystem( char *host, unsigned short remotePort, const char *data, int dataLength )
{
	RakPeer::AdvertiseSystem( host, remotePort, data, dataLength );
}

RakNetStatisticsStruct* const RakClient::GetStatistics( void )
{
	return RakPeer::GetStatistics( remoteSystemList[ 0 ].playerId );
}

int RakClient::GetOtherClientIndexByPlayerID( PlayerID playerId )
{
	unsigned i;

	for ( i = 0; i < 32; i++ )
	{
		if ( otherClients[ i ].playerId == playerId )
			return i;
	}

	return -1;
}

int RakClient::GetFreeOtherClientIndex( void )
{
	unsigned i;

	for ( i = 0; i < 32; i++ )
	{
		if ( otherClients[ i ].isActive == false )
			return i;
	}

	return -1;
}

PlayerIndex RakClient::GetPlayerIndex( void )
{
	return localPlayerIndex;
}
