/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_DNO
 * @file 
 * @brief Manager for distributed Objects. 
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

