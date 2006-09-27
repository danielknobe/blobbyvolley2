/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @ingroup RAKNET_DNO 
* @file
* @brief Distributed Object Manager Implementation 
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

#include "DistributedNetworkObjectManager.h"

#include "BitStream.h"
#include "DistributedNetworkObjectStub.h"
#include "DistributedNetworkObject.h"
#include "PacketEnumerations.h"
#include "RakClientInterface.h"
#include "RakServerInterface.h"

#include <memory.h>

DistributedNetworkObjectManager DistributedNetworkObjectManager::I;


DistributedNetworkObjectManager::DistributedNetworkObjectManager()
{
	rakServerInterface = 0;
	rakClientInterface = 0;
	localObjectIdentifier = 0;
}

DistributedNetworkObjectManager::~DistributedNetworkObjectManager()
{
	classList.clear();

	unsigned i;

	for ( i = 0; i < distributedNetworkObjectInstanceRegistry.size(); i++ )
		delete distributedNetworkObjectInstanceRegistry[ i ];

	distributedNetworkObjectInstanceRegistry.clear();
}

bool DistributedNetworkObjectManager::ExistsNetworkObject( DistributedNetworkObject *object )
{
	unsigned i;

	for ( i = 0; i < distributedNetworkObjectInstanceRegistry.size(); i++ )
		if ( distributedNetworkObjectInstanceRegistry[ i ]->object == object )
			return true;

	return false;
}

bool DistributedNetworkObjectManager::RegisterNetworkObject( DistributedNetworkObject *object, char classIdentifier[ MAXIMUM_CLASS_IDENTIFIER_LENGTH ], unsigned char &localObjectIndex )
{
	if ( ExistsNetworkObject( object ) == false )
	{
		DistributedNetworkObjectRegistryNode * newNode = new DistributedNetworkObjectRegistryNode;
		newNode->object = object;
		memcpy( newNode->classIdentifier, classIdentifier, MAXIMUM_CLASS_IDENTIFIER_LENGTH );

		if ( rakClientInterface && object->IsLocalObject() )
			localObjectIndex = localObjectIdentifier++;

		distributedNetworkObjectInstanceRegistry.insert( newNode );

		return true;
	}

	return false;
}

void DistributedNetworkObjectManager::UnregisterNetworkObject( DistributedNetworkObject *object )
{
	unsigned i;
	DistributedNetworkObjectRegistryNode *node;

	for ( i = 0; i < distributedNetworkObjectInstanceRegistry.size(); i++ )
		if ( distributedNetworkObjectInstanceRegistry[ i ]->object == object )
		{
			node = distributedNetworkObjectInstanceRegistry[ i ];
			distributedNetworkObjectInstanceRegistry[ i ] = distributedNetworkObjectInstanceRegistry[ distributedNetworkObjectInstanceRegistry.size() - 1 ];
			delete node;
			distributedNetworkObjectInstanceRegistry.del( distributedNetworkObjectInstanceRegistry.size() - 1 );
			break;
		}
}

void DistributedNetworkObjectManager::AddClassStub( DistributedNetworkObjectBaseStub *stub )
{
	// Add stub to a list.
	classList.insert( stub );
}

DistributedNetworkObject *DistributedNetworkObjectManager::GetClassInstanceByIdentifier( char *classIdentifier )
{
	unsigned i;

	if ( classIdentifier[ 0 ] == 0 )
		return 0;

	for ( i = 0; i < classList.size(); i++ )
	{
		// Byte 0 of the class identifier holds the length of the remaining bytes
		// The +1 is because we also compare the first byte

		if ( memcmp( classIdentifier, classList[ i ]->GetEncodedClassName(), classList[ i ]->GetEncodedClassName() [ 0 ] + 1 ) == 0 )
			return classList[ i ]->GetObject();
	}

	return 0;
}

DistributedNetworkObject* DistributedNetworkObjectManager::HandleDistributedNetworkObjectPacket( Packet *packet )
{
	assert( packet->data[ 0 ] == ID_UPDATE_DISTRIBUTED_NETWORK_OBJECT );
	// Assert that the first byte of the packet is ID_UPDATE_DISTRIBUTED_NETWORK_OBJECT

	char classIdentifier[ MAXIMUM_CLASS_IDENTIFIER_LENGTH ];
	RakNet::BitStream bitStream( ( char* ) packet->data, packet->length, true );
	ObjectID objectId;
	DistributedNetworkObject* object = 0;
	int action;
	PlayerID pid;
	bool isLocalObject;
	unsigned char localObjectIndex;
	pid = UNASSIGNED_PLAYER_ID;


	if ( DistributedNetworkObject::DeserializeClassHeader( &bitStream, action, objectId, classIdentifier, isLocalObject, localObjectIndex, pid ) == false )
		return 0; // Some kind of corrupted packet, or the server was active but not registered, or the client was active but not registered

	bool serverProcess = true;

	if ( DistributedNetworkObjectManager::Instance()->GetRakClientInterface() )
	{
		if ( packet->playerId == DistributedNetworkObjectManager::Instance()->GetRakClientInterface()->GetServerID() )
			serverProcess = false;
	}

	if ( action >= 1 )
	{
		if ( objectId != UNASSIGNED_OBJECT_ID )
		{
			// Either create an object with the specified id (client only), or update an object with the specified id(server or client)
			object = ( DistributedNetworkObject* ) GET_OBJECT_FROM_ID( objectId );

			if ( object == 0 && DistributedNetworkObjectManager::Instance()->GetRakServerInterface() )
			{
				// Tried to update a deleted object.  Ignore the packet
				return 0;
			}
		}

		if ( object == 0 )
		{
			// Not an existing object
			object = GetClassInstanceByIdentifier( classIdentifier );

			if ( object == 0 )
			{
				// If this assert hits then one system tried to create a class that was
				// not registered with REGISTER_DISTRIBUTED_NETWORK_OBJECT on this system.
				// Byte 0 of 0 means no class identifier was encoded
				assert( classIdentifier[ 0 ] == 0 );
				return 0;
			}

			if ( DistributedNetworkObjectManager::Instance()->GetRakClientInterface() &&
				DistributedNetworkObjectManager::Instance()->GetRakServerInterface() == 0 )
				object->SetNetworkID( objectId );

			if ( DistributedNetworkObjectManager::Instance()->GetRakClientInterface() && pid != UNASSIGNED_PLAYER_ID )
				object->SetClientOwnerID( pid ); // So the client knows who is the owner of the object

			object->ProcessDistributedMemoryStack( &bitStream, false, false, serverProcess );

			object->WriteToHeapFromBitstream( &bitStream );

			if ( DistributedNetworkObjectManager::Instance()->GetRakClientInterface() && DistributedNetworkObjectManager::Instance()->GetRakClientInterface()->IsConnected() )
			{
				// Be sure to call OnDistributedObjectCreation before calling UpdateDistributedObject because it unsets the localObject flag

				if ( object->OnDistributedObjectCreation( packet->playerId ) == false )
				{
					delete object;
					object = 0;
				}

				else
				{
					object->SetLocalObject( false );
					object->ReadCreationData( &bitStream );
				}
			}
			else if ( DistributedNetworkObjectManager::Instance()->GetRakServerInterface() && DistributedNetworkObjectManager::Instance()->GetRakServerInterface()->IsActive() )
			{
				// Be sure to call OnDistributedObjectCreation before calling UpdateDistributedObject because it unsets the localObject flag
				if ( object->OnDistributedObjectCreation( packet->playerId ) == false )
				{
					delete object;
					object = 0;

					// Tell the sender that the object creation was rejected
					static const unsigned char objectRejectedID = ID_DISTRIBUTED_NETWORK_OBJECT_CREATION_REJECTED;
					RakNet::BitStream rejectionBitStream;
					rejectionBitStream.Write( objectRejectedID );
					rejectionBitStream.Write( localObjectIndex );

					DistributedNetworkObjectManager::Instance()->GetRakServerInterface()->Send( &rejectionBitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false );
				}
				else
				{
					object->SetLocalObject( false );
					object->ReadCreationData( &bitStream );

					// Receipt of this packet will set the object ID for the client so future data packets can be processed
					static const unsigned char objectAcceptedID = ID_DISTRIBUTED_NETWORK_OBJECT_CREATION_ACCEPTED;
					RakNet::BitStream acceptanceBitStream;
					acceptanceBitStream.Write( objectAcceptedID );
					acceptanceBitStream.Write( localObjectIndex );
					acceptanceBitStream.Write( object->GetNetworkID() );
					acceptanceBitStream.Write( ( char* ) & packet->playerId, sizeof( PlayerID ) );

					DistributedNetworkObjectManager::Instance()->GetRakServerInterface()->Send( &acceptanceBitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false );

					// Not necessary, will broadcast in UpdateDistributedObject anyway
					/*
					// Broadcast the new object
					bitStream.Reset();
					object->SerializeClassHeader(&bitStream, classIdentifier, true, false, 0);
					object->ProcessDistributedMemoryStack(&bitStream, true, true,true);
					object->BroadcastSerializedClass(&bitStream);
					*/

					// If the client creates the object, it is considered the owner
					object->SetClientOwnerID( packet->playerId );

					object->UpdateDistributedObject( classIdentifier, true );

				}
			}
		}
		else
		{
			// Update of existing object

			// If this is the server and the client is trying to do an update, make sure it is either the
			// owner or no owner is set

			if ( serverProcess )
			{
				if ( object->GetClientOwnerID() != UNASSIGNED_PLAYER_ID &&
					object->GetClientOwnerID() != packet->playerId &&
					object->AllowSpectatorUpdate( packet->playerId ) == false )
					// Client trying to update an object it does not own - and it was not allowed to
					return object;
			}

			// Update the synchronized memory
			object->ProcessDistributedMemoryStack( &bitStream, false, false, serverProcess );

			object->WriteToHeapFromBitstream( &bitStream );

			object->OnNetworkUpdate( packet->playerId );

			// If this is the server, force a broadcast, including back to the sender in case the server overrode any data
			if ( DistributedNetworkObjectManager::Instance()->GetRakServerInterface() && DistributedNetworkObjectManager::Instance()->GetRakServerInterface()->IsActive() )
			{
				bitStream.Reset();
				object->SerializeClassHeader( &bitStream, classIdentifier, action, false, 0 );
				object->ProcessDistributedMemoryStack( &bitStream, true, true, true );
				object->WriteToBitstreamFromHeap( &bitStream, true );
				object->WriteCreationData( &bitStream );
				// Send to all but the local client and the sender
				object->BroadcastSerializedClass( &bitStream, packet->playerId );
			}
		}
	}

	else
	{
		object = ( DistributedNetworkObject* ) GET_OBJECT_FROM_ID( objectId );

		if ( object == 0 )
		{
			// Got a packet saying to destroy an object we don't have!
			return 0;
		}

		// If this is the server and the client is trying to destroy it, make sure it is either the
		// owner or no owner is set
		if ( serverProcess )
		{
			if ( object->GetClientOwnerID() != UNASSIGNED_PLAYER_ID &&
				object->GetClientOwnerID() != packet->playerId &&
				object->AllowSpectatorUpdate( packet->playerId ) == false )
				// Client trying to update an object it does not own - and it was not allowed to
				return 0;
		}

		object->OnDistributedObjectDestruction( packet->playerId );
		object = 0;
	}

	return object;
}

void DistributedNetworkObjectManager::HandleDistributedNetworkObjectPacketCreationAccepted( Packet *packet )
{
	unsigned char localObjectIndex;
	DistributedNetworkObject* object;
	ObjectID objectID;
	PlayerID pid;

	RakNet::BitStream bitStream( ( char* ) packet->data, packet->length, false );
	bitStream.IgnoreBits( sizeof( unsigned char ) * 8 ); // Skip the packet identifier

	if ( bitStream.Read( localObjectIndex ) == 0 )
		return ;

	if ( bitStream.Read( objectID ) == 0 )
		return ;

	if ( bitStream.Read( ( char* ) & pid, sizeof( PlayerID ) ) == 0 )
		return ;

	// Find the object with this index
	object = GetObjectByLocalObjectIndex( localObjectIndex );

	if ( object == 0 )
	{
		// Can't find the object specified by the packet.  It may have been locally deleted already
		return ;
	}

	object->SetNetworkID( objectID );

	if ( object->OnDistributedObjectCreation( packet->playerId ) == false )
	{
		delete object;
		object = 0;
	}

	else
	{
		object->SetLocalObject( false );
		object->SetClientOwnerID( pid );
	}

}

void DistributedNetworkObjectManager::HandleDistributedNetworkObjectPacketCreationRejected( Packet *packet )
{
	unsigned char localObjectIndex;
	DistributedNetworkObject* object;

	RakNet::BitStream bitStream( ( char* ) packet->data, packet->length, false );
	bitStream.IgnoreBits( sizeof( unsigned char ) * 8 ); // Skip the packet identifier

	if ( bitStream.Read( localObjectIndex ) == 0 )
		return ;

	// Find the object with this index
	object = GetObjectByLocalObjectIndex( localObjectIndex );

	if ( object == 0 )
	{
		assert( 0 ); // Can't find the object specified by the packet
		return ;
	}

	object->OnDistributedObjectRejection();
}

DistributedNetworkObject* DistributedNetworkObjectManager::GetObjectByLocalObjectIndex( unsigned char localObjectIndex )
{
	unsigned i;

	for ( i = 0; i < distributedNetworkObjectInstanceRegistry.size(); i++ )
		if ( distributedNetworkObjectInstanceRegistry[ i ]->object->IsLocalObject() && distributedNetworkObjectInstanceRegistry[ i ]->object->GetLocalObjectIdentifier() == localObjectIndex )
			return distributedNetworkObjectInstanceRegistry[ i ]->object;

	return 0;
}


// This will send all registered network objects to the specified player
void DistributedNetworkObjectManager::SendAllDistributedObjects( PlayerID playerId )
{
	if ( DistributedNetworkObjectManager::Instance()->GetRakServerInterface() && DistributedNetworkObjectManager::Instance()->GetRakServerInterface()->IsActive() )
	{
		if ( DistributedNetworkObjectManager::Instance()->GetRakClientInterface() && DistributedNetworkObjectManager::Instance()->GetRakClientInterface()->IsConnected() )
		{
			PlayerID localClient = DistributedNetworkObjectManager::Instance()->GetRakClientInterface()->GetPlayerID();

			if ( localClient == playerId )
			{
				// It is a mistake to force a send to the local client, because all objects already exist on the server
				return ;
			}
		}

		RakNet::BitStream bitStream;
		unsigned i;

		for ( i = 0; i < distributedNetworkObjectInstanceRegistry.size(); i++ )
		{
			bitStream.Reset();
			distributedNetworkObjectInstanceRegistry[ i ]->object->SerializeClassHeader( &bitStream, distributedNetworkObjectInstanceRegistry[ i ]->classIdentifier, 2, false, 255 );

			// Note to self - ProcessDistributedMemoryStack with write to true will update network memory. During the next normal updates
			// any changes that occured in the meantime won't be distributed.  Is this something to be concerned about?
			distributedNetworkObjectInstanceRegistry[ i ]->object->ProcessDistributedMemoryStack( &bitStream, true, true, true );
			distributedNetworkObjectInstanceRegistry[ i ]->object->WriteToBitstreamFromHeap( &bitStream, true );
			distributedNetworkObjectInstanceRegistry[ i ]->object->WriteCreationData( &bitStream );
			DistributedNetworkObjectManager::Instance()->GetRakServerInterface()->Send( &bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, playerId, false );
		}
	}
}

void DistributedNetworkObjectManager::RegisterRakServerInterface( RakServerInterface *_rakServerInterface )
{
	rakServerInterface = _rakServerInterface;
}

void DistributedNetworkObjectManager::RegisterRakClientInterface( RakClientInterface *_rakClientInterface )
{
	rakClientInterface = _rakClientInterface;
}

RakServerInterface *DistributedNetworkObjectManager::GetRakServerInterface( void ) const
{
	return rakServerInterface;
}

RakClientInterface *DistributedNetworkObjectManager::GetRakClientInterface( void ) const
{
	return rakClientInterface;
}

