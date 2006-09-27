/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_DNO
 * @file 
 * @brief Manager for distributed Objects. 
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

#ifndef __DISTRIBUTED_NETWORK_OBJECT_MANAGER_H
#define __DISTRIBUTED_NETWORK_OBJECT_MANAGER_H

#include "ArrayList.h"
#include "NetworkTypes.h"
#include "EncodeClassName.h"


class DistributedNetworkObject;

class DistributedNetworkObjectBaseStub;

struct Packet;

class RakServerInterface;

class RakClientInterface;

/**
 * @ingroup RAKNET_DNO 
 * @brief Distributed Network Object Manager class 
 *
 * Manage all Distributed Network Object. It's in charge of creating
 * on peer remote ditributed object.
 */

class DistributedNetworkObjectManager
{

public:
	/**
	 * Default constructor 
	 */
	DistributedNetworkObjectManager();
	/**
	 * Destructor
	 */
	~DistributedNetworkObjectManager();
	/**
	 * You need to register your instance of RakServer and/or RakClient
	 * to be usable by the DistributedNetworkObjectManager.
	 * @param _rakServerInterface A RakServer instance.
	 */
	void RegisterRakServerInterface( RakServerInterface *_rakServerInterface );
	
	/**
	 * You need to register your instance of RakClient and/of RakServer 
	 * to be usable by the DistributedNetworkObjectManager.
	 * @param _rakClientInterface A RakClient instance.
	 */
	void RegisterRakClientInterface( RakClientInterface *_rakClientInterface );
	
	/**
	 * Retrive the associated RakServer 
	 * @return The Previously associated RakServer 
	 */
	RakServerInterface *GetRakServerInterface( void ) const;
	
	/**
	 * Retrieve the associated RakClient 
	 * @return The previously associated RakClient 
	 */
	RakClientInterface *GetRakClientInterface( void ) const;
	
	DistributedNetworkObject* HandleDistributedNetworkObjectPacket( Packet *packet );
	
	void HandleDistributedNetworkObjectPacketCreationAccepted( Packet *packet );
	
	void HandleDistributedNetworkObjectPacketCreationRejected( Packet *packet );
	
	bool ExistsNetworkObject( DistributedNetworkObject *object );
	
	/**
	 * Register a new Network object. 
	 * The maximum number of registration is 256. 
	 * @param object The obejct to manage.
	 * @param classIdentifier The class identifier of the object 
	 * @param localObjectIndex the ObjectId associated to this object by the manager. 
	 * @return True on successful add, false on already exists
	 */
	bool RegisterNetworkObject( DistributedNetworkObject *object, char classIdentifier[ MAXIMUM_CLASS_IDENTIFIER_LENGTH ], unsigned char &localObjectIndex );
	
	/**
	 * Unregister this instance of the Distributed Network Object
	 * @param object The object to remove. 
	 */
	void UnregisterNetworkObject( DistributedNetworkObject *object );
	
	
	/**
	 * This will send all registered network objects to the specified player (server only)
	 * @param playerId Id of the player to send all data. 
	 */
	void SendAllDistributedObjects( PlayerID playerId );
	
	/**
	 * Singleton pattern for Distributed Network Object Manager
	 * @return Retrieve the singleton Instance associated to the manager 
	 * @todo Make default constructor private to ensure single instance 
	 */
	static inline DistributedNetworkObjectManager* Instance()
	{
		return & I;
	}
	
	/**
	 * Register a new Stub 
	 */
	void AddClassStub( DistributedNetworkObjectBaseStub *stub );
	
	/**
	 * Retrieve an object using its class identifier.
	 * @param classIdentifier The encoded class identifier. 
	 * @return The object if found NULL otherwise. 
	 */
	DistributedNetworkObject *GetClassInstanceByIdentifier( char *classIdentifier );
	
protected:
	static DistributedNetworkObjectManager I;
	
	DistributedNetworkObject* GetObjectByLocalObjectIndex( unsigned char localObjectIndex );
	/**
	 * @brief Association between a DNO Object and a class Identifier 
	 * 
	 * Associate a class identifier to an object.
	 */
	
	struct DistributedNetworkObjectRegistryNode
	{
		/**
		 *  The DNO Object 
		 */
		DistributedNetworkObject* object;
		/**
		 * The class IDentifier 
		 */
		char classIdentifier[ MAXIMUM_CLASS_IDENTIFIER_LENGTH ];
	};
	
	BasicDataStructures::List<DistributedNetworkObjectBaseStub*> classList;
	BasicDataStructures::List<DistributedNetworkObjectRegistryNode*> distributedNetworkObjectInstanceRegistry;
	
	RakServerInterface *rakServerInterface;
	RakClientInterface *rakClientInterface;
	/**
	 * This cycles from 0 to 255 and around giving identifiers to objects that don't have IDs set 
	 * by the server yet.  This way we can still access them
	 */
	unsigned char localObjectIdentifier;
};

#endif

