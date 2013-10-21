/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief RakServer Implementation
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
#include "RakServer.h"
#include "PacketEnumerations.h"
#include "GetTime.h"

RakServer::RakServer()
{
}

RakServer::~RakServer()
{}

bool RakServer::Start( unsigned short AllowedPlayers, int threadSleepTimer, unsigned short port, const char *forceHostAddress )
{
	bool init;

	RakPeer::Disconnect( 30 );

	init = RakPeer::Initialize( AllowedPlayers, port, threadSleepTimer, forceHostAddress );
	RakPeer::SetMaximumIncomingConnections( AllowedPlayers );

	return init;
}

void RakServer::Disconnect( unsigned int blockDuration )
{
	RakPeer::Disconnect( blockDuration );
}

bool RakServer::Send( const char *data, const long length, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast )
{
	return RakPeer::Send( data, length, priority, reliability, orderingChannel, playerId, broadcast );
}

bool RakServer::Send( const RakNet::BitStream *bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast )
{
	return RakPeer::Send( bitStream, priority, reliability, orderingChannel, playerId, broadcast );
}

packet_ptr RakServer::Receive( void )
{
	packet_ptr packet = RakPeer::Receive();

	if ( packet )
	{
		if ( packet->data[ 0 ] == ID_DISCONNECTION_NOTIFICATION || packet->data[ 0 ] == ID_CONNECTION_LOST || packet->data[ 0 ] == ID_NEW_INCOMING_CONNECTION )
		{
			// Relay the disconnection
			RakNet::BitStream bitStream( packet->length + PlayerID_Size );
			unsigned char typeId;

			if ( packet->data[ 0 ] == ID_DISCONNECTION_NOTIFICATION )
				typeId = ID_REMOTE_DISCONNECTION_NOTIFICATION;
			else
				if ( packet->data[ 0 ] == ID_CONNECTION_LOST )
					typeId = ID_REMOTE_CONNECTION_LOST;
				else
					typeId = ID_REMOTE_NEW_INCOMING_CONNECTION;

			bitStream.Write( typeId );
			bitStream.Write( packet->playerId.binaryAddress );
			bitStream.Write( packet->playerId.port );
			bitStream.Write( ( unsigned short& ) packet->playerIndex );

			Send( &bitStream, SYSTEM_PRIORITY, RELIABLE, 0, packet->playerId, true );

			if ( packet->data[ 0 ] == ID_NEW_INCOMING_CONNECTION )
			{
				unsigned i;

				for ( i = 0; i < remoteSystemListSize; i++ )
				{
					if ( remoteSystemList[ i ].playerId != UNASSIGNED_PLAYER_ID && packet->playerId != remoteSystemList[ i ].playerId )
					{
						bitStream.Reset();
						typeId = ID_REMOTE_EXISTING_CONNECTION;
						bitStream.Write( typeId );
						bitStream.Write( remoteSystemList[ i ].playerId.binaryAddress );
						bitStream.Write( remoteSystemList[ i ].playerId.port );
						bitStream.Write( ( unsigned short ) i );
						// One send to tell them of the connection
						Send( &bitStream, SYSTEM_PRIORITY, RELIABLE, 0, packet->playerId, false );
					}
				}
			}
		}
	}

	return packet;
}

void RakServer::Kick( PlayerID playerId )
{
	RakPeer::NotifyAndFlagForDisconnect(playerId, false);
}

void RakServer::SetAllowedPlayers( unsigned short AllowedPlayers )
{
	RakPeer::SetMaximumIncomingConnections( AllowedPlayers );
}

unsigned short RakServer::GetAllowedPlayers( void ) const
{
	return RakPeer::GetMaximumIncomingConnections();
}

unsigned short RakServer::GetConnectedPlayers( void )
{
	unsigned short numberOfSystems;

	RakPeer::GetConnectionList( 0, &numberOfSystems );
	return numberOfSystems;
}

void RakServer::GetPlayerIPFromID( PlayerID playerId, char returnValue[ 22 ], unsigned short *port )
{
	*port = playerId.port;
	strcpy( returnValue, RakPeer::PlayerIDToDottedIP( playerId ) );
}

void RakServer::PingPlayer( PlayerID playerId )
{
	RakPeer::Ping( playerId );
}

int RakServer::GetLastPing( PlayerID playerId )
{
	return RakPeer::GetLastPing( playerId );
}

int RakServer::GetLowestPing( PlayerID playerId )
{
	return RakPeer::GetLowestPing( playerId );
}

bool RakServer::IsActive( void ) const
{
	return RakPeer::IsActive();
}

unsigned int RakServer::GetNumberOfAddresses( void )
{
	return RakPeer::GetNumberOfAddresses();
}

const char* RakServer::GetLocalIP( unsigned int index )
{
	return RakPeer::GetLocalIP( index );
}

void RakServer::PushBackPacket( Packet *packet )
{
	RakPeer::PushBackPacket( packet );
}

int RakServer::GetIndexFromPlayerID( PlayerID playerId )
{
	return RakPeer::GetIndexFromPlayerID( playerId );
}

PlayerID RakServer::GetPlayerIDFromIndex( int index )
{
	return RakPeer::GetPlayerIDFromIndex( index );
}

bool RakServer::IsActivePlayerID( PlayerID playerId )
{
	return RakPeer::GetRemoteSystemFromPlayerID( playerId ) != 0;
}

bool RakServer::SetMTUSize( int size )
{
	return RakPeer::SetMTUSize( size );
}

int RakServer::GetMTUSize( void ) const
{
	return RakPeer::GetMTUSize();
}

void RakServer::AdvertiseSystem( char *host, unsigned short remotePort, const char *data, int dataLength )
{
	RakPeer::AdvertiseSystem( host, remotePort, data, dataLength );
}

RakNetStatisticsStruct * const RakServer::GetStatistics( PlayerID playerId )
{
	return RakPeer::GetStatistics( playerId );
}
