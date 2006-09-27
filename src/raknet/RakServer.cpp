/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file
* @brief RakServer Implementation 
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
#include "RakServer.h"
#include "PacketEnumerations.h"
#include "GetTime.h"
#include "Rand.h"

// Defined in rand.cpp
//extern void seedMT(unsigned int seed);//exported in Rand.h
//extern unsigned int randomMT(void); //exported in Rand.h

RakServer::RakServer()
{
	nextSeedUpdate = 0;
	synchronizedRandomInteger = false;
	relayStaticClientData = false;
	broadcastPingsTime = 0;
}

RakServer::~RakServer()
{}

void RakServer::InitializeSecurity( const char *pubKeyE, const char *pubKeyN )
{
	RakPeer::InitializeSecurity( 0, 0, pubKeyE, pubKeyN );
}

void RakServer::DisableSecurity( void )
{
	RakPeer::DisableSecurity();
}

#pragma warning( disable : 4100 ) // warning C4100: 'depreciated' : unreferenced formal parameter
bool RakServer::Start( unsigned short AllowedPlayers, unsigned int depreciated, int threadSleepTimer, unsigned short port, const char *forceHostAddress )
{
	bool init;

	RakPeer::Disconnect( 30 );

	init = RakPeer::Initialize( AllowedPlayers, port, threadSleepTimer, forceHostAddress );
	RakPeer::SetMaximumIncomingConnections( AllowedPlayers );

	// Random number seed
	long time = RakNet::GetTime();
	seedMT( time );
	seed = randomMT();

	if ( seed % 2 == 0 )   // Even
		seed--; // make odd

	nextSeed = randomMT();

	if ( nextSeed % 2 == 0 )   // Even
		nextSeed--; // make odd

	return init;
}

void RakServer::SetPassword( const char *_password )
{
	if ( _password )
	{
		RakPeer::SetIncomingPassword( _password, ( int ) strlen( _password ) + 1 );
	}

	else
	{
		RakPeer::SetIncomingPassword( 0, 0 );
	}
}

bool RakServer::HasPassword( void )
{
	return GetIncomingPassword()->GetNumberOfBytesUsed() > 0;
}

void RakServer::Disconnect( unsigned int blockDuration )
{
	RakPeer::Disconnect( blockDuration );
}

bool RakServer::Send( const char *data, const long length, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast )
{
	return RakPeer::Send( data, length, priority, reliability, orderingChannel, playerId, broadcast );
}

bool RakServer::Send( RakNet::BitStream *bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast )
{
	return RakPeer::Send( bitStream, priority, reliability, orderingChannel, playerId, broadcast );
}

Packet* RakServer::Receive( void )
{
	Packet * packet = RakPeer::Receive();

	// This is just a regular time based update.  Nowhere else good to put it

	if ( RakPeer::IsActive() && occasionalPing )
	{
		unsigned int time = RakNet::GetTime();

		if ( time > broadcastPingsTime || ( packet && packet->data[ 0 ] == ID_RECEIVED_STATIC_DATA ) )
		{
			if ( time > broadcastPingsTime )
				broadcastPingsTime = time + 30000; // Broadcast pings every 30 seconds

			unsigned i, count;

			RemoteSystemStruct *remoteSystem;
			RakNet::BitStream bitStream( ( PlayerID_Size + sizeof( short ) ) * 32 + sizeof(unsigned char) );
			unsigned char typeId = ID_BROADCAST_PINGS;

			bitStream.Write( typeId );

			for ( i = 0, count = 0; count < 32 && i < remoteSystemListSize; i++ )
			{
				remoteSystem = remoteSystemList + i;

				if ( remoteSystem->playerId != UNASSIGNED_PLAYER_ID )
				{
					bitStream.Write( remoteSystem->playerId.binaryAddress );
					bitStream.Write( remoteSystem->playerId.port );
					bitStream.Write( remoteSystem->pingAndClockDifferential[ remoteSystem->pingAndClockDifferentialWriteIndex ].pingTime );
					count++;
				}
			}

			if ( count > 0 )   // If we wrote anything
			{

				if ( packet && packet->data[ 0 ] == ID_NEW_INCOMING_CONNECTION )   // If this was a new connection
					Send( &bitStream, SYSTEM_PRIORITY, RELIABLE, 0, packet->playerId, false ); // Send to the new connection
				else
					Send( &bitStream, SYSTEM_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true ); // Send to everyone
			}
		}
	}

	// This is just a regular time based update.  Nowhere else good to put it
	if ( RakPeer::IsActive() && synchronizedRandomInteger )
	{
		unsigned int time = RakNet::GetTime();

		if ( time > nextSeedUpdate || ( packet && packet->data[ 0 ] == ID_NEW_INCOMING_CONNECTION ) )
		{
			if ( time > nextSeedUpdate )
				nextSeedUpdate = time + 9000; // Seeds are updated every 9 seconds

			seed = nextSeed;

			nextSeed = randomMT();

			if ( nextSeed % 2 == 0 )   // Even
				nextSeed--; // make odd

			/*
			SetRandomNumberSeedStruct s;

			s.ts = ID_TIMESTAMP;
			s.timeStamp = RakNet::GetTime();
			s.typeId = ID_SET_RANDOM_NUMBER_SEED;
			s.seed = seed;
			s.nextSeed = nextSeed;
			RakNet::BitStream s_BitS( SetRandomNumberSeedStruct_Size );
			s.Serialize( s_BitS );
			*/

			RakNet::BitStream outBitStream(sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned int));
			outBitStream.Write((unsigned char) ID_TIMESTAMP);
			outBitStream.Write((unsigned int) RakNet::GetTime());
			outBitStream.Write((unsigned char) ID_SET_RANDOM_NUMBER_SEED);
			outBitStream.Write(seed);
			outBitStream.Write(nextSeed);

			if ( packet && packet->data[ 0 ] == ID_NEW_INCOMING_CONNECTION )
				Send( &outBitStream, SYSTEM_PRIORITY, RELIABLE, 0, packet->playerId, false );
			else
				Send( &outBitStream, SYSTEM_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true );
		}
	}

	if ( packet )
	{
		// Intercept specific client / server feature packets. This will do an extra send and still pass on the data to the user

		if ( packet->data[ 0 ] == ID_RECEIVED_STATIC_DATA )
		{
			if ( relayStaticClientData )
			{
				// Relay static data to the other systems but the sender
				RakNet::BitStream bitStream( packet->length + PlayerID_Size );
				unsigned char typeId = ID_REMOTE_STATIC_DATA;
				bitStream.Write( typeId );
				bitStream.Write( packet->playerId.binaryAddress );
				bitStream.Write( packet->playerId.port );
				bitStream.Write( packet->playerIndex );
				bitStream.Write( ( char* ) packet->data + sizeof(unsigned char), packet->length - sizeof(unsigned char) );
				Send( &bitStream, SYSTEM_PRIORITY, RELIABLE, 0, packet->playerId, true );
			}
		}

		else
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

							if ( relayStaticClientData )
							{
								bitStream.Reset();
								typeId = ID_REMOTE_STATIC_DATA;
								bitStream.Write( typeId );
								bitStream.Write( remoteSystemList[ i ].playerId.binaryAddress );
								bitStream.Write( remoteSystemList[ i ].playerId.port );
								bitStream.Write( (unsigned short) i );
								bitStream.Write( ( char* ) remoteSystemList[ i ].staticData.GetData(), remoteSystemList[ i ].staticData.GetNumberOfBytesUsed() );
								// Another send to tell them of the static data
								Send( &bitStream, SYSTEM_PRIORITY, RELIABLE, 0, packet->playerId, false );
							}
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

void RakServer::DeallocatePacket( Packet *packet )
{
	RakPeer::DeallocatePacket( packet );
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

int RakServer::GetAveragePing( PlayerID playerId )
{
	return RakPeer::GetAveragePing( playerId );
}

int RakServer::GetLastPing( PlayerID playerId )
{
	return RakPeer::GetLastPing( playerId );
}

int RakServer::GetLowestPing( PlayerID playerId )
{
	return RakPeer::GetLowestPing( playerId );
}

void RakServer::StartOccasionalPing( void )
{
	RakPeer::SetOccasionalPing( true );
}

void RakServer::StopOccasionalPing( void )
{
	RakPeer::SetOccasionalPing( false );
}

bool RakServer::IsActive( void ) const
{
	return RakPeer::IsActive();
}

unsigned int RakServer::GetSynchronizedRandomInteger( void ) const
{
	return seed;
}

void RakServer::StartSynchronizedRandomInteger( void )
{
	synchronizedRandomInteger = true;
}

void RakServer::StopSynchronizedRandomInteger( void )
{
	synchronizedRandomInteger = false;
}

/*

void RakServer::SynchronizeMemory(UniqueIDType uniqueIdentifier, char *memoryBlock, unsigned short size, bool isAuthority, bool (*synchronizationRules) (char*,char*),ObjectID secondaryUniqueIdentifier)
{
RakPeer::SynchronizeMemory(uniqueIdentifier, memoryBlock, size, isAuthority, synchronizationRules, secondaryUniqueIdentifier);
}

void RakServer::DesynchronizeMemory(UniqueIDType uniqueIdentifier, ObjectID secondaryUniqueIdentifier)
{
RakPeer::DesynchronizeMemory(uniqueIdentifier, secondaryUniqueIdentifier);
}

void RakServer::DesynchronizeAllMemory(void)
{
RakPeer::DesynchronizeAllMemory();
}
*/

bool RakServer::GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer )
{
	return RakPeer::GenerateCompressionLayer( inputFrequencyTable, inputLayer );
}

bool RakServer::DeleteCompressionLayer( bool inputLayer )
{
	return RakPeer::DeleteCompressionLayer( inputLayer );
}

void RakServer::RegisterAsRemoteProcedureCall( char* uniqueID, void ( *functionPointer ) ( RPCParameters *rpcParms ) )
{
	RakPeer::RegisterAsRemoteProcedureCall( uniqueID, functionPointer );
}

void RakServer::RegisterClassMemberRPC( char* uniqueID, void *functionPointer )
{
	RakPeer::RegisterClassMemberRPC( uniqueID, functionPointer );
}

void RakServer::UnregisterAsRemoteProcedureCall( char* uniqueID )
{
	RakPeer::UnregisterAsRemoteProcedureCall( uniqueID );
}

bool RakServer::RPC( char* uniqueID, const char *data, unsigned int bitLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool shiftTimestamp, ObjectID objectID )
{
	return RakPeer::RPC( uniqueID, data, bitLength, priority, reliability, orderingChannel, playerId, broadcast, shiftTimestamp, objectID );
}

bool RakServer::RPC( char* uniqueID, RakNet::BitStream *parameters, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool shiftTimestamp, ObjectID objectID )
{
	return RakPeer::RPC( uniqueID, parameters, priority, reliability, orderingChannel, playerId, broadcast, shiftTimestamp, objectID );
}

/*
bool RakServer::HandleRPCPacket(Packet* packet)
{
return RakPeer::HandleRPCPacket(packet->data, packet->length, packet->playerId);
}*/

void RakServer::SetTrackFrequencyTable( bool b )
{
	RakPeer::SetCompileFrequencyTable( b );
}

bool RakServer::GetSendFrequencyTable( unsigned int outputFrequencyTable[ 256 ] )
{
	return RakPeer::GetOutgoingFrequencyTable( outputFrequencyTable );
}

float RakServer::GetCompressionRatio( void ) const
{
	return RakPeer::GetCompressionRatio();
}

float RakServer::GetDecompressionRatio( void ) const
{
	return RakPeer::GetDecompressionRatio();
}

void RakServer::AttachMessageHandler( MessageHandlerInterface *messageHandler )
{
	RakPeer::AttachMessageHandler(messageHandler);
}

void RakServer::DetachMessageHandler( MessageHandlerInterface *messageHandler )
{
	RakPeer::DetachMessageHandler(messageHandler);
}

RakNet::BitStream * RakServer::GetStaticServerData( void )
{
	return RakPeer::GetRemoteStaticData( myPlayerId );
}

void RakServer::SetStaticServerData( const char *data, const long length )
{
	RakPeer::SetRemoteStaticData( myPlayerId, data, length );
}

void RakServer::SetRelayStaticClientData( bool b )
{
	relayStaticClientData = b;
}

void RakServer::SendStaticServerDataToClient( PlayerID playerId )
{
	RakPeer::SendStaticData( playerId );
}

void RakServer::SetOfflinePingResponse( const char *data, const unsigned int length )
{
	RakPeer::SetOfflinePingResponse( data, length );
}

RakNet::BitStream * RakServer::GetStaticClientData( PlayerID playerId )
{
	return RakPeer::GetRemoteStaticData( playerId );
}

void RakServer::SetStaticClientData( PlayerID playerId, const char *data, const long length )
{
	RakPeer::SetRemoteStaticData( playerId, data, length );
}

// This will read the data from playerChangedId and send it to playerToSendToId
void RakServer::ChangeStaticClientData( PlayerID playerChangedId, PlayerID playerToSendToId )
{
	RemoteSystemStruct * remoteSystem = GetRemoteSystemFromPlayerID( playerChangedId );

	if ( remoteSystem == 0 )
		return ; // No such playerChangedId

	// Relay static data to the other systems but the sender
	RakNet::BitStream bitStream( remoteSystem->staticData.GetNumberOfBytesUsed() + PlayerID_Size + sizeof(unsigned char) );

	unsigned char typeId = ID_REMOTE_STATIC_DATA;

	bitStream.Write( typeId );

	bitStream.Write( playerChangedId.binaryAddress );

	bitStream.Write( playerChangedId.port );

	bitStream.Write( ( char* ) remoteSystem->staticData.GetData(), remoteSystem->staticData.GetNumberOfBytesUsed() );

	Send( &bitStream, SYSTEM_PRIORITY, RELIABLE, 0, playerToSendToId, true );
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

void RakServer::AddToBanList( const char *IP )
{
	RakPeer::AddToBanList( IP );
}

void RakServer::RemoveFromBanList( const char *IP )
{
	RakPeer::RemoveFromBanList( IP );
}

void RakServer::ClearBanList( void )
{
	RakPeer::ClearBanList();
}

bool RakServer::IsBanned( const char *IP )
{
	return RakPeer::IsBanned( IP );
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
