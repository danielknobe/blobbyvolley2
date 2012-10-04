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
	unsigned i;
	
	for ( i = 0; i < 32; i++ )
		otherClients[ i ].isActive = false;
		
	nextSeedUpdate = 0;
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
		otherClients[ i ].staticData.Reset();
	}
	
	// ignore depreciated. A pointless variable
	return RakPeer::Connect( host, serverPort, ( char* ) password.GetData(), password.GetNumberOfBytesUsed() );
}

void RakClient::Disconnect( unsigned int blockDuration )
{
	RakPeer::Disconnect( blockDuration );
}

void RakClient::InitializeSecurity( const char *privKeyP, const char *privKeyQ )
{
	RakPeer::InitializeSecurity( privKeyP, privKeyQ, 0, 0 );
}

void RakClient::SetPassword( const char *_password )
{
	if ( _password == 0 || _password[ 0 ] == 0 )
		password.Reset();
	else
	{
		password.Reset();
		password.Write( _password, ( int ) strlen( _password ) + 1 );
	}
}

bool RakClient::HasPassword( void ) const
{
	return password.GetNumberOfBytesUsed() > 0;
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

Packet* RakClient::Receive( void )
{
	Packet * packet = RakPeer::Receive();
	
	// Intercept specific client / server feature packets
	
	if ( packet )
	{
		RakNet::BitStream bitStream( ( char* ) packet->data, packet->length, false );
		int i;
		
		if ( packet->data[ 0 ] == ID_CONNECTION_REQUEST_ACCEPTED )
		{
//			ConnectionAcceptStruct cas;
//			cas.Deserialize(bitStream);
		//	unsigned short remotePort;
		//	PlayerID externalID;
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
				DeallocatePacket( packet );
				return 0;
			}
			
			
			if ( packet->data[ 0 ] == ID_REMOTE_DISCONNECTION_NOTIFICATION ||
				packet->data[ 0 ] == ID_REMOTE_CONNECTION_LOST )
			{
				i = GetOtherClientIndexByPlayerID( packet->playerId );
				
				if ( i >= 0 )
					otherClients[ i ].isActive = false;
			}
		}			
		else if ( packet->data[ 0 ] == ID_REMOTE_STATIC_DATA )
		{
			bitStream.IgnoreBits( 8 ); // Ignore identifier
			bitStream.Read( packet->playerId.binaryAddress );
			bitStream.Read( packet->playerId.port );
			bitStream.Read( packet->playerIndex ); // ADDED BY KURI 
			
			i = GetOtherClientIndexByPlayerID( packet->playerId );
			
			if ( i < 0 )
				i = GetFreeOtherClientIndex();
				
			if ( i >= 0 )
			{
				otherClients[ i ].playerId = packet->playerId;
				otherClients[ i ].isActive = true;
				otherClients[ i ].staticData.Reset();
				// The static data is what is left over in the stream
				otherClients[ i ].staticData.Write( ( char* ) bitStream.GetData() + BITS_TO_BYTES( bitStream.GetReadOffset() ), bitStream.GetNumberOfBytesUsed() - BITS_TO_BYTES( bitStream.GetReadOffset() ) );
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
						otherClients[ index ].staticData.Reset();
					}
					
					else
						bitStream.IgnoreBits( sizeof( short ) * 8 );
				}
			}
			
			DeallocatePacket( packet );
			return 0;
		}
        else
		if ( packet->data[ 0 ] == ID_TIMESTAMP &&
			packet->length == sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned int) )
		{
			/*
			RakNet::BitStream s_BitS( (char *)packet->data, SetRandomNumberSeedStruct_Size, false );
			SetRandomNumberSeedStruct s;
			s.Deserialize( s_BitS );
			*/

			RakNet::BitStream inBitStream((char *)packet->data, packet->length, false);
			/*
			unsigned char ts;
			unsigned int timeStamp;
			unsigned char typeId;
			unsigned int seed;
			unsigned int nextSeed;
			*/

			unsigned int timeStamp;
			unsigned char typeId;
			unsigned int in_seed;
			unsigned int in_nextSeed;
			inBitStream.IgnoreBits(8); // ID_TIMESTAMP
			inBitStream.Read(timeStamp);
			inBitStream.Read(typeId); // ID_SET_RANDOM_NUMBER_SEED ?

			// Check to see if this is a user TIMESTAMP message which
			// accidentally has length SetRandomNumberSeedStruct_Size
			if ( typeId != ID_SET_RANDOM_NUMBER_SEED )
				return packet;

			inBitStream.Read(in_seed);
			inBitStream.Read(in_nextSeed);
			
			seed = in_seed;
			nextSeed = in_nextSeed;
			nextSeedUpdate = timeStamp + 9000; // Seeds are updated every 9 seconds
			
			DeallocatePacket( packet );
			return 0;
		}
	}
	
	return packet;
}

void RakClient::DeallocatePacket( Packet *packet )
{
	RakPeer::DeallocatePacket( packet );
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

int RakClient::GetAveragePing( void )
{
	if ( remoteSystemList == 0 )
		return -1;
		
	return RakPeer::GetAveragePing( remoteSystemList[ 0 ].playerId );
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

void RakClient::StartOccasionalPing( void )
{
	RakPeer::SetOccasionalPing( true );
}

void RakClient::StopOccasionalPing( void )
{
	RakPeer::SetOccasionalPing( false );
}

bool RakClient::IsConnected( void ) const
{
	unsigned short numberOfSystems;
	
	RakPeer::GetConnectionList( 0, &numberOfSystems );
	return numberOfSystems == 1;
}

unsigned int RakClient::GetSynchronizedRandomInteger( void ) const
{
	if ( RakNet::GetTime() > nextSeedUpdate )
		return nextSeed;
	else
		return seed;
}

bool RakClient::GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer )
{
	return RakPeer::GenerateCompressionLayer( inputFrequencyTable, inputLayer );
}

bool RakClient::DeleteCompressionLayer( bool inputLayer )
{
	return RakPeer::DeleteCompressionLayer( inputLayer );
}

void RakClient::SetTrackFrequencyTable( bool b )
{
	RakPeer::SetCompileFrequencyTable( b );
}

bool RakClient::GetSendFrequencyTable( unsigned int outputFrequencyTable[ 256 ] )
{
	return RakPeer::GetOutgoingFrequencyTable( outputFrequencyTable );
}

float RakClient::GetCompressionRatio( void ) const
{
	return RakPeer::GetCompressionRatio();
}

float RakClient::GetDecompressionRatio( void ) const
{
	return RakPeer::GetDecompressionRatio();
}

void RakClient::AttachMessageHandler( MessageHandlerInterface *messageHandler )
{
	RakPeer::AttachMessageHandler(messageHandler);
}

void RakClient::DetachMessageHandler( MessageHandlerInterface *messageHandler )
{
	RakPeer::DetachMessageHandler(messageHandler);
}

RakNet::BitStream * RakClient::GetStaticServerData( void )
{
	if ( remoteSystemList == 0 )
		return 0;
		
	return RakPeer::GetRemoteStaticData( remoteSystemList[ 0 ].playerId );
}

void RakClient::SetStaticServerData( const char *data, const long length )
{
	if ( remoteSystemList == 0 )
		return ;
		
	RakPeer::SetRemoteStaticData( remoteSystemList[ 0 ].playerId, data, length );
}

RakNet::BitStream * RakClient::GetStaticClientData( PlayerID playerId )
{
	int i;
	
	if ( playerId == UNASSIGNED_PLAYER_ID )
	{
		return & localStaticData;
	}
	
	else
	{
		// KevinJ - No this is not correct.  The client does not know about any system other than the server.
		// The caller needs to check for a NULL return value

		/// \todo Cirilo: Check that this is the CORRECT implementation
		/// An alternative is to fall through and return this expression
		/// rather than directly returning '0'.
	//	return RakPeer::GetRemoteStaticData( playerId );

		i = GetOtherClientIndexByPlayerID( playerId );
//		printf("OtherClientIndexByPlayerID: %d\n", i);
		
		if ( i >= 0 )
		{
			return & ( otherClients[ i ].staticData );
		}

	}
	
	return 0;
}

void RakClient::SetStaticClientData( PlayerID playerId, const char *data, const long length )
{
	int i;
	
	if ( playerId == UNASSIGNED_PLAYER_ID )
	{
		localStaticData.Reset();
		localStaticData.Write( data, length );
	}
	
	else
	{
		i = GetOtherClientIndexByPlayerID( playerId );
		
		if ( i >= 0 )
		{
			otherClients[ i ].staticData.Reset();
			otherClients[ i ].staticData.Write( data, length );
		}
		
		else
			RakPeer::SetRemoteStaticData( playerId, data, length );
	}
	
}

void RakClient::SendStaticClientDataToServer( void )
{
	if ( remoteSystemList == 0 )
		return ;
		
	RakPeer::SendStaticData( remoteSystemList[ 0 ].playerId );
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
