/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_DNO 
 * @file 
 * @brief Distributed Object System 
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

/**
 * @defgroup RAKNET_DNO Distributed Network Object Subsystem 
 * This is the higher level construction within RakNet. 
 * It provides the user with synchronized distributed network object. 
 */

#ifndef __DISTRIBUTED_NETWORK_OBJECT
#define __DISTRIBUTED_NETWORK_OBJECT

#include "NetworkObject.h"
#include "EncodeClassName.h"
#include "BitStream.h"

/**
 * @ingroup RAKNET_DNO 
 * @brief Distributed Network Object base class
 * 
 * The distributed Network Object System relies mostly on this
 * class. It's the base class for all Distributed Object. You should
 * specialize this class to define DNO objects.
 *
 */

class DistributedNetworkObject : public NetworkObject
{

public:
	/**
	 * Default Constructor
	 */
	DistributedNetworkObject();
	/**
	 * Destructor
	 */
	virtual ~DistributedNetworkObject();
	/**
	 * Call this every update cycle for every distributed object that you want updated over the network and to interpolate
	 * classID should be a unique identifier for the class that matches the parameter to REGISTER_DISTRIBUTED_CLASS
	 * The obvious choice is the name of the class - however you can use something shorter if you wish to save a bit of bandwidth
	 */
	virtual void UpdateDistributedObject( char *classID, bool isClassIDEncoded = false );
	/**
	 * Sets the maximum frequency with which memory synchronization packets can be sent.
	 * Lower values increase granularity but require more bandwidth
	 */
	virtual void SetMaximumUpdateFrequency( unsigned int frequency );
	/**
	 * This broadcasts a request to destroy an object on the network.  OnDistributedObjectDestruction will be called.
	 * If you wish to block deletion, override OnDistributedObjectDestruction
	 */
	virtual void DestroyObjectOnNetwork( void );
	/**
	 * Server only function 
	 * By default, when a client creates an object only it can update the client authoritative members
	 * Of the class it creates.  You can also set this manually with SetClientOwnerID
	 * This function is called when a client that does not own an object tries to change any fields in that object
	 * Return true to allow the update.
	 * Return false (default) to not allow the update.
	 */
	virtual bool AllowSpectatorUpdate( PlayerID sender );
	
	/**
	 * 
	 * Runtime memory synchronization
	 *
	 * This system allows you to flag blocks of memory for synchronization between systems.
	 * This is different from REGISTER_X_DISTRIBUTED_OBJECT_MEMBERS in that you can set this up at runtime.
	 *
	 * All synchronized blocks of memory are identified by a unique unsigned char.
	 * This char should match between systems for the same memory block.
	 * You can use any value for the identifier as long as it is unique.
	 *
	 * Tags memory to be synchronized. You can set the server or the client as the authority for this block.
	 * Only the authority will write this memory to the network when it is changed.
	 */
	void SynchronizeMemory( unsigned char memoryBlockIndex, char* memoryBlock, unsigned short memoryBlockSize, bool serverAuthority );
	/**
	 * Untags memory that was formerly synchronized.
	 */
	void DesynchronizeMemory( unsigned char memoryBlockIndex );
	/**
	 * Changes the authority for memory.  You probably will never need this.
	 */
	void SetAuthority( unsigned char memoryBlockIndex, bool serverAuthority );
	/**
	 * Tells you if a block of memory was formerly used.  You probably will never need this.
	 */
	bool IsMemoryBlockIndexUsed( unsigned char memoryBlockIndex );
	/**
	 * Use this to set a maximum update frequency higher than what was specified to SetMaximumUpdateFrequency
	 * Lower values have no effect.
	 */
	void SetMaxMemoryBlockUpdateFrequency( unsigned char memoryBlockIndex, int max );
	/**
	 * --------------------------------------------------------------------------
	 * Events
	 * --------------------------------------------------------------------------
	 * When object creation data is needed, WriteCreationData is called.
	 * This function is for you to write any data that is needed to create or initialize the object
	 * On remote systems
	 */
	virtual void WriteCreationData( RakNet::BitStream *initialData );
	/**
	 * When an object is created, ReadCreationData is called immediately after a 
	 * successful call to OnDistributedObjectCreation
	 * This function is for you to read any data written from WriteCreationData
	 * on remote systems.  If the object is created by the client, this function is
	 * also called by the creator of the object when sent back from the server in case the
	 * server overrode any settings
	 */
	virtual void ReadCreationData( RakNet::BitStream *initialData );
	/**
	 * When distributed data changes for an object, this function gets called. Default behavior is to do nothing.
	 * Override it if you want to perform updates when data is changed
	 * On the server it is also important to override this to make sure the data the client just sent you is reasonable.
	 */
	virtual void OnNetworkUpdate( PlayerID sender );
	/**
	 * This is called when the object is created by the network.  Return true to accept the new object, false to reject it.
	 * The return value is primarily useful for the server to reject objects created by the client.  On the client you would normally return true
	 * senderID is the playerID of the player that created the object (or the server, which you can get from RakClientInterface::GetServerID)
	 */
	virtual bool OnDistributedObjectCreation( PlayerID senderID );
	
	/**
	 * This is called when the object is destroyed by the network.
	 * Default behavior is to delete itself.  You can override this if you want to delete it elsewhere, or at a later time.
	 * If you don't delete the object, you should call DestroyObjectOnNetwork manually to remove it from the network
	 * Note it is important to override this on the server for objects you don't want clients to delete
	 * senderID is the playerID of the player that created the object (or the server, which you can get from RakClientInterface::GetServerID)
	 */
	virtual void OnDistributedObjectDestruction( PlayerID senderID );
	/**
	 * This is called when the server rejects an object the client created.  Default behavior is to destroy the object.
	 * You can override this to destroy the object yourself.
	 */
	virtual void OnDistributedObjectRejection( void );
	/**
	 * 
	 * --------------------------------------------------------------------------
	 * All functions below are used by the API and do not need to be called or modified
	 * --------------------------------------------------------------------------
	 * You don't need to modify this
	 * Sends the bitstream to other systems
	 */
	virtual void BroadcastSerializedClass( RakNet::BitStream *bitStream, PlayerID doNotSendTo );
	
	/**
	 *
	 * You don't need to modify this
	 * Writes the header identifying the class, objectID, and whether the object is being created or destroyed
	 * action of 0 means destruction, 1 means update, 2 means create
	 */
	virtual void SerializeClassHeader( RakNet::BitStream *bitStream, char *classIdentifier, int action, bool localObject, unsigned char localObjectIndex );
	/**
	 *
	 * You don't need to modify this
	 * Writes the header identifying the class, objectID, and whether the object is being created or destroyed
	 * action of 0 means destruction, 1 means update, 2 means create
	 */
	static bool DeserializeClassHeader( RakNet::BitStream *bitStream, int &action, ObjectID &objectId, char classIdentifier[ MAXIMUM_CLASS_IDENTIFIER_LENGTH ], bool &localObject, unsigned char &localObjectIndex, PlayerID &pid );
	/**
	 * You don't need to modify this
	 * Returns true if this object was locally created, as opposed to created by the network
	 */
	bool IsLocalObject( void ) const;
	/**
	 * You don't need to modify this
	 * Returns true if this object was locally created, as opposed to created by the network
	 */
	void SetLocalObject( bool b );
	/**
	 * Writes to or reads from a bitstream for all distributed memory on the stack.  This function does not need to be modified by the end-user
	 * Returns true if any data was written
	 */
	virtual bool ProcessDistributedMemoryStack( RakNet::BitStream *bitStream, bool isWrite, bool forceWrite, bool isServerAuthoritative );
	/**
	 * Same as ProcessDistributedMemoryStack with isWrite to false, but for the heapNodeList.
	 * Returns true if data was written
	 */
	bool WriteToBitstreamFromHeap( RakNet::BitStream *bitStream, bool forceWrite );
	/**
	 * Same as ProcessDistributedMemoryStack with isWrite to true, but for the heapNodeList.
	 */
	void WriteToHeapFromBitstream( RakNet::BitStream *bitStream );
	/**
	 * You don't need to modify this
	 * Returns the identifier for an object that has not yet had SetID called on it
	 */
	unsigned char GetLocalObjectIdentifier( void ) const;
	/**
	 * This is set to who created this object.
	 * On the server, this also determines who can update the client authoritative members of this object.
	 * You can manually set it to UNASSIGNED_PLAYER_ID so no client owns the object, so any client can update the object.
	 */
	PlayerID GetClientOwnerID( void ) const;
	/**
	 * This is set to who created this object.
	 * On the server, this also determines who can update the client authoritative members of this object.
	 * You can manually set it to UNASSIGNED_PLAYER_ID so no client owns the object, so any client can update the object.
	 */
	void SetClientOwnerID( PlayerID id );
	
protected:
	/**
	 * You don't need to modify this
	 * Initializes distributed object the first time it is used
	 */
	virtual void DistributedMemoryInit( bool isServerAuthoritative );
	/**
	 * You don't need to modify this
	 * Interpolates between all variables on the network that are set to interpolate.  This function does not need to be modified by the end-user
	 */
	virtual void InterpolateDistributedMemory( bool isServerAuthoritative );
	/**
	 * lastBroadcastTime is the last time we sent a packet from this class
	 */
	unsigned int maximumUpdateFrequency, lastBroadcastTime;
	/**
	 * localObject means this object was created on the local system.  If false, it means it was created by the network
	 */
	bool localObject;
	/**
	 * If this is a local object, this identifies it uniquely
	 */
	unsigned char localObjectIdentifier;
	/**
	 * This records if the distributed memory was initialized via DistributedMemoryInit
	 */
	bool distributedMemoryInitialized;
	/**
	 * Which client is allowed to change the client authoritative members of this object
	 */
	PlayerID clientOwnerID;
	
	struct
	{
		char *watchedData;
		char *lastWriteValue;
		int allocatedBlockSize, maximumUpdateInterval;
		unsigned short usedBlockSize;
		bool serverAuthority;
		unsigned int nextUpdateTime;
	}
	
	heapNodeList[ 256 ];
};

#endif

