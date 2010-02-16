/**
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

#include "DataReplicator.h"
#include "PacketEnumerations.h"
#include "RakPeerInterface.h"
#include "GetTime.h"
#include "StringCompressor.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

unsigned short DataReplicator::memoryToStringMappingKey=0;
//ObjectReplicationHandlerRegistry ObjectReplicationHandler::instance;
ObjectReplicationHandler* ObjectReplicationHandler::defaultHandler=0;
ObjectReplicationHandler* ObjectReplicationHandler::handlerRegistryHead=0;
//BasicDataStructures::OrderedList<ObjectReplicationHandler*, const char*> ObjectReplicationHandler::handlerRegistry;

int BaseObjectDataComp( DataReplicator::BaseObjectData* data, ObjectID key )
{
	if (key < data->synchronizedObject->GetNetworkID())
		return -1;
	if (key==data->synchronizedObject->GetNetworkID())
		return 0;
	return 1;
}

int BaseMemoryDataComp( DataReplicator::BaseMemoryData* data, unsigned short key )
{
	if (key < data->localKey)
		return -1;
	if (key==data->localKey)
		return 0;
	return 1;
}

int ExtendedMemoryDataComp( DataReplicator::ExtendedMemoryData* data, DataReplicator::BaseData * key )
{
	if (key < data->baseData)
		return -1;
	if (key==data->baseData)
		return 0;
	return 1;
}

int ExtendedObjectDataComp( DataReplicator::ExtendedObjectData* data, DataReplicator::BaseData * key )
{
	if (key < data->baseData)
		return -1;
	if (key==data->baseData)
		return 0;
	return 1;
}

int ParticipantStructComp( DataReplicator::ParticipantStruct* data, PlayerID key )
{
	if (key < data->playerId)
		return -1;
	if (key==data->playerId)
		return 0;
	return 1;
}
/*
int ObjectReplicationHandlerComp( ObjectReplicationHandler* data, const char* key )
{
	return strcmp(key, data->GetHandlerClassName());
}
*/

DataReplicator::DataReplicator()
{
	rakPeer=0;
	broadcastToAll=false;
}

DataReplicator::~DataReplicator()
{
	Clear();
}

void DataReplicator::SetBroadcastToAll(bool value)
{
	broadcastToAll=value;
}

bool DataReplicator::GetBroadcastToAll(void) const
{
	return broadcastToAll;
}

void DataReplicator::AddParticipant(PlayerID playerId)
{
	if (playerId==UNASSIGNED_PLAYER_ID)
	{
		unsigned short peerIndex;
		
		for (peerIndex=0; peerIndex < rakPeer->GetMaximumNumberOfPeers(); peerIndex++)
		{
			playerId=rakPeer->GetPlayerIDFromIndex(peerIndex);
			if (playerId!=UNASSIGNED_PLAYER_ID)
				AddToParticipantList(playerId);
		}
	}
	else
	{
		AddToParticipantList(playerId);
	}
}
void DataReplicator::RemoveParticipant(PlayerID playerId)
{
	if (playerId==UNASSIGNED_PLAYER_ID)
	{
		RemoveAllParticipants();
	}
	else
	{
		RemoveFromParticipantList(playerId, true);
	}
}
void DataReplicator::SynchronizeMemory(const char *stringId, const char *memory, int memoryByteLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool sendUpdates, bool receiveUpdates, int maxUpdateFrequencyMS, ObjectID synchronizedObjectId)
{
	SynchronizedMemoryDefault *synchronizedMemoryDefault;
	synchronizedMemoryDefault = new SynchronizedMemoryDefault(memory, memoryByteLength);
	SynchronizeMemory(stringId, (SynchronizedMemory*)synchronizedMemoryDefault, priority, reliability, orderingChannel, sendUpdates, receiveUpdates, maxUpdateFrequencyMS, synchronizedObjectId);
}

void DataReplicator::SynchronizeMemory(const char *stringId, SynchronizedMemory *memory, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool sendUpdates, bool receiveUpdates, int maxUpdateFrequencyMS, ObjectID synchronizedObjectId)
{
	assert(sendUpdates || receiveUpdates);

	if (GetBaseMemoryDataByString(stringId))
		return; // Already synchronized
	
	unsigned i;
	char newStringId[256];
	assert(strlen(stringId) < 246);

	if (synchronizedObjectId==UNASSIGNED_OBJECT_ID)
		strcpy(newStringId, stringId);
	else
		sprintf(newStringId, "%s__%i", stringId, synchronizedObjectId);
	
	BaseMemoryData *baseData = new BaseMemoryData;
	// Base class stuff
	baseData->priority=priority;
	baseData->reliability=reliability;
	baseData->orderingChannel=orderingChannel;
	baseData->receiveUpdates=receiveUpdates;
	baseData->sendUpdates=sendUpdates;
	baseData->maxUpdateFrequencyMS=maxUpdateFrequencyMS;
	baseData->lastUpdateTime=0;

	// Extended stuff
	baseData->localIdentifier=new char [strlen(newStringId)+1];
	strcpy(baseData->localIdentifier, newStringId);
	baseData->localKey=GenerateMemoryStringMappingKey();
	baseData->associatedObject=synchronizedObjectId;
	baseData->synchronizedMemory=memory;

	// Add to the list
	memoryList.InsertElement(baseData, baseData->localKey, BaseMemoryDataComp);

	// For each participant
	for (i=0; i < participantList.Size(); i++)
	{
		if (receiveUpdates)
			SendMemoryStartRequest(baseData, participantList[i]);
	}
}
void DataReplicator::UnsynchronizeMemory(const char *memoryPtr, bool sendNotificationPacket)
{
	BaseMemoryData *baseData;
	unsigned i;
	baseData=0;
	for (i=0; i < memoryList.Size(); i++)
	{
		if (memoryList[i]->synchronizedMemory->GetType()==SYNCHRONIZED_MEMORY_TYPE_DEFAULT && ((SynchronizedMemoryDefault*)(memoryList[i]->synchronizedMemory))->GetUserData()==memoryPtr)
		{
			baseData=memoryList[i];
			break;
		}
	}

	if (baseData)
		UnsynchronizeMemoryInt(baseData, sendNotificationPacket);
}

void DataReplicator::UnsynchronizeMemory(SynchronizedMemory *memory, bool sendNotificationPacket)
{
	BaseMemoryData *baseData = GetBaseMemoryDataBySynchronizedMemory(memory);
	if (baseData)
		UnsynchronizeMemoryInt(baseData, sendNotificationPacket);
}

void DataReplicator::RequestObjectCreation(const char *objectName, RakNet::BitStream *context, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId)
{
	assert(objectName);

	// If this assert hits you are trying to request an object you cannot create.  Be sure that for one of your classes,
	// SynchronizedObject::GetClassName returns the exact string passed here.
	assert(ObjectReplicationHandler::GetHandler(objectName));

	RakNet::BitStream outBitStream;
	
	if (context)
		outBitStream.WriteBits(context->GetData(), context->GetNumberOfBitsUsed());

	if (playerId!=UNASSIGNED_PLAYER_ID)
	{
		// If this hits then you are sending to someone that is not a participant in the data replication system, which is not allowed.
		// Either add this player as a participant or call AddParticipant(UNASSIGNED_PLAYER_ID) to allow sends to all.

		ParticipantStruct *participantStruct=GetParticipantByPlayerID(playerId);
		if (participantStruct==0)
		{
#ifdef _DEBUG
			assert(0);
#endif
			return;
		}
		
		participantStruct->objectRequestCount++;
	
		outBitStream.Write((unsigned char) ID_REPLICATOR_OBJECT_CREATION_REQUEST);
		EncodeClassName(objectName, playerId, &outBitStream);

		rakPeer->Send(&outBitStream, priority, reliability, orderingChannel, playerId, false);
	}
	else
	{
		// Send to all participants
		unsigned i;
		for (i=0; i < participantList.Size(); i++)
		{
			outBitStream.Reset();
			outBitStream.Write((unsigned char) ID_REPLICATOR_OBJECT_CREATION_REQUEST);
			EncodeClassName(objectName, participantList[i]->playerId, &outBitStream);
			rakPeer->Send(&outBitStream, priority, reliability, orderingChannel, participantList[i]->playerId, false);

			// Record that we sent a packet asking for an object to be created.  We don't record what particular object we ask for
			// because the recipient can change that.
			participantList[i]->objectRequestCount++;
		}
	}
}

void DataReplicator::SynchronizeObject(SynchronizedObject *object, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool sendUpdates, bool receiveUpdates, int maxUpdateFrequencyMS)
{
	SynchronizeObjectInt(object, priority, reliability, orderingChannel, sendUpdates, receiveUpdates, maxUpdateFrequencyMS, UNASSIGNED_PLAYER_ID);
}

void DataReplicator::SynchronizeObjectInt(SynchronizedObject *object, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool sendUpdates, bool receiveUpdates, int maxUpdateFrequencyMS, PlayerID requestingSystem)
{
	assert(sendUpdates || receiveUpdates);
	assert(object);

	if (GetBaseObjectByObjectID(object->GetNetworkID()))
		return; // Already synchronized

	if (ObjectReplicationHandler::GetHandler(object->GetObjectClassName())==0)
	{
#ifdef _DEBUG
		// Object was not registered.  Every class must have a defined ObjectReplicationHandler that returns the same string for GetClassName as the object.
		assert(0);
#endif	
		return;
	}

	BaseObjectData *baseData = new BaseObjectData;
	// Base class stuff
	baseData->priority=priority;
	baseData->reliability=reliability;
	baseData->orderingChannel=orderingChannel;
	baseData->receiveUpdates=receiveUpdates;
	baseData->sendUpdates=sendUpdates;
	baseData->maxUpdateFrequencyMS=maxUpdateFrequencyMS;
	baseData->lastUpdateTime=0;

	// Extended stuff
	baseData->synchronizedObject=object;

	// Add to the list
#ifdef _DEBUG
	assert(baseData->synchronizedObject->GetNetworkID()!=UNASSIGNED_OBJECT_ID);
#endif
	objectList.InsertElement(baseData, baseData->synchronizedObject->GetNetworkID(), BaseObjectDataComp);

	// For each participant
	unsigned i;
	if (sendUpdates)
	{
		for (i=0; i < participantList.Size(); i++)
		{
			AddExtendedObjectDataToParticipant(baseData, participantList[i]);
		}
	}

	object->OnSynchronization(requestingSystem);
}

void DataReplicator::AddExtendedObjectDataToParticipant(BaseObjectData *baseObjectData, ParticipantStruct *participantStruct)
{
	assert(baseObjectData->sendUpdates);

	ExtendedObjectData *extendedData = new ExtendedObjectData;
	// Base class stuff
	extendedData->baseData=(BaseData*)baseObjectData;

	// Extended stuff
    extendedData->objectInstantiated=false;
	extendedData->objectInScope=false;

	participantStruct->extendedObjectList.InsertElement(extendedData, extendedData->baseData, ExtendedObjectDataComp);
}
void DataReplicator::UnsynchronizeObject(SynchronizedObject *object, bool sendNotificationPacket)
{
	UnsynchronizeObjectInt(object, sendNotificationPacket, UNASSIGNED_PLAYER_ID);

}
void DataReplicator::UnsynchronizeObjectInt(SynchronizedObject *object, bool sendNotificationPacket, PlayerID playerId)
{
	assert(object);
	unsigned i;
	BaseObjectData *baseData = GetBaseObjectByObjectID(object->GetNetworkID());
	if (baseData)
	{
		if (baseData->sendUpdates)
		{
			for (i=0; i < participantList.Size(); i++)
				RemoveExtendedDataFromParticipant((BaseData *)baseData, participantList[i], false, sendNotificationPacket);
		}
		else
		{
			if (sendNotificationPacket)
			{
				for (i=0; i < participantList.Size(); i++)
					SendDataStopRequest(baseData, participantList[i], false);
			}
		}
		

		objectList.RemoveElement(baseData->synchronizedObject->GetNetworkID(), BaseObjectDataComp);
		delete baseData;
	}

	object->OnDesynchronization(playerId);
}
void DataReplicator::RemoveExtendedDataFromParticipant(BaseData *baseData, ParticipantStruct *participantStruct, bool isMemory, bool sendNotificationPacket)
{
	bool objectExists;
	unsigned index;
	if (isMemory)
		index=participantStruct->extendedMemoryList.GetIndexFromKey(baseData, &objectExists, ExtendedMemoryDataComp);
	else
		index=participantStruct->extendedObjectList.GetIndexFromKey(baseData, &objectExists, ExtendedObjectDataComp);
	if (objectExists)
	{
		if (isMemory)
		{
			// If this is unified memory, we need to scan through all memory elements and count how many times this pointer is used.
			BaseMemoryData *baseMemoryData = (BaseMemoryData *) baseData;
			if (participantStruct->extendedMemoryList[index]->lastMemorySend)
			{
				if (baseMemoryData->synchronizedMemory->IsUnifiedMemory())
				{
					unsigned count;
					count = GetUnifiedMemoryCount(participantStruct->extendedMemoryList[index]->lastMemorySend);
					if (count==1)
						participantStruct->extendedMemoryList[index]->lastMemorySend->ReleaseCopy();
				}
				else
					participantStruct->extendedMemoryList[index]->lastMemorySend->ReleaseCopy();
			}			
			
			if (sendNotificationPacket)
				SendDataStopRequest(baseData, participantStruct, isMemory);

			delete participantStruct->extendedMemoryList[index];
			participantStruct->extendedMemoryList.RemoveElementAtIndex(index);
		}
		else
		{
			if (sendNotificationPacket)
			{
				SendDataStopRequest(baseData, participantStruct, isMemory);
			}

			delete participantStruct->extendedObjectList[index];
			participantStruct->extendedObjectList.RemoveElementAtIndex(index);
		}
	}
}
unsigned DataReplicator::GetUnifiedMemoryCount(SynchronizedMemory *memory)
{
	unsigned i,j,count;
	for (i=0, count=0; i < participantList.Size(); i++)
	{
		for (j=0; j < (participantList[i])->extendedMemoryList.Size(); j++)
			if ((participantList[i])->extendedMemoryList[j]->lastMemorySend==memory)
				count++;
	}
	return count;
}
SynchronizedMemory* DataReplicator::GetSynchronizedMemoryCopy(BaseMemoryData *baseMemoryData)
{
	if (baseMemoryData->synchronizedMemory->IsUnifiedMemory())
	{
		unsigned i,j;
		for (i=0; i < participantList.Size(); i++)
		{
			for (j=0; j < participantList[i]->extendedMemoryList.Size(); j++)
			{
				if (participantList[i]->extendedMemoryList[j]->baseData==baseMemoryData && participantList[i]->extendedMemoryList[j]->lastMemorySend)
				{
					return participantList[i]->extendedMemoryList[j]->lastMemorySend;
				}
			}
		}

		return baseMemoryData->synchronizedMemory->MakeSynchronizedDataCopy();
	}
	else
		return baseMemoryData->synchronizedMemory->MakeSynchronizedDataCopy();
}
void DataReplicator::Clear(void)
{
	unsigned i;

	while (memoryList.Size()>0)
		UnsynchronizeMemory(memoryList[memoryList.Size()-1]->synchronizedMemory, false);

	while (objectList.Size()>0)
	{
		if (objectList[objectList.Size()-1]->synchronizedObject)
			UnsynchronizeObject(objectList[objectList.Size()-1]->synchronizedObject, false);
	}

	for (i=0; i < participantList.Size(); i++)
	{
		delete participantList[i];
	}
	// Clear the participant list
	participantList.Clear();

	for (i=0; i < localObjectNameTable.size(); i++)
		delete localObjectNameTable[i];
	localObjectNameTable.clear();

}
void DataReplicator::UnsynchronizeMemoryInt(BaseMemoryData *baseData, bool sendNotificationPacket)
{
	unsigned i;
	for (i=0; i < participantList.Size(); i++)
		RemoveExtendedDataFromParticipant((BaseData *)baseData, participantList[i], true, sendNotificationPacket);
	
	memoryList.RemoveElement(baseData->localKey, BaseMemoryDataComp);
	if (baseData->synchronizedMemory->AutoDeleteOnUnreference())
		delete baseData->synchronizedMemory;
	delete baseData;
}

// Returns the synchronized memory we sent, if we sent any
void DataReplicator::ValidatedSend(ExtendedData *extendedData, ParticipantStruct *participant, bool isMemory)
{
	BaseData *baseData = extendedData->baseData;
	RakNet::BitStream outBitStream;
	bool validScope;
	ExtendedObjectData *extendedObjectData;
	BaseMemoryData *baseMemoryData;
	ExtendedMemoryData *extendedMemoryData;
	BaseObjectData *baseObjectData;

	if (baseData==0)
		return; // This data request doesn't have a corresponding synchronized element

	if (baseData->sendUpdates==false)
		return;


	// Is this memory?
	if (isMemory)
	{
		bool dataDifferent;
		baseMemoryData = (BaseMemoryData*) baseData;
		extendedMemoryData = (ExtendedMemoryData*) extendedData;
		//   Does this memory have an associated object ID?
		if (baseMemoryData->associatedObject!=UNASSIGNED_OBJECT_ID)
		{
			// Yes, has associated object ID.  Can we find this object in the extended object send list for this participant?
			extendedObjectData = GetExtendedObjectByObjectID(baseMemoryData->associatedObject, participant);
			if (extendedObjectData)
				validScope=extendedObjectData->objectInScope;
			else
			{
				// Can't find the object.  Remove the base memory from the list
#ifdef _DEBUG
				assert(0);
#endif
				UnsynchronizeMemory(baseMemoryData->synchronizedMemory, false);
			}
		}
		else
		{
			//   No associated object.
			validScope=true;
		}

		if (validScope==false)
			return;

		//   Is the data different from what we last sent?
		//   if forcedComparisonDifference, then yes it is different
		//   Otherwise check to see if it is different.
		if (extendedMemoryData->lastMemorySend)
		{
			if (baseMemoryData->synchronizedMemory->IsUnifiedMemory()==false || baseMemoryData->didComparisonAndCopy==false)
				dataDifferent=baseMemoryData->synchronizedMemory->ShouldSendUpdate(participant->playerId, extendedMemoryData->lastMemorySend, extendedMemoryData->lastSendTime);
			else
				dataDifferent=baseMemoryData->dataDifferent;
		}
		else
		{
			// First send so have to make or get a copy of the data
			extendedMemoryData->lastMemorySend = GetSynchronizedMemoryCopy(baseMemoryData);
			dataDifferent=true;
		}

		baseMemoryData->dataDifferent=dataDifferent;

		if (dataDifferent==false)
			return;

		unsigned int time = RakNet::GetTime();

		// Encode timestamp, if requested
		if (baseMemoryData->synchronizedMemory->IncludeTimestamp())
		{
			outBitStream.Write((unsigned char) ID_TIMESTAMP);
			outBitStream.Write(time);
		}

		// Encode packet ID
		outBitStream.Write((unsigned char)ID_REPLICATOR_DATA_SEND_MEMORY);

		// Encode remote key
		outBitStream.Write(extendedMemoryData->remoteKey);

		// Encode new data value
		baseMemoryData->synchronizedMemory->Serialize(&outBitStream, extendedMemoryData->lastMemorySend, extendedMemoryData->lastSendTime);

		// Send the packet
		rakPeer->Send(&outBitStream, baseMemoryData->priority, baseMemoryData->reliability, baseMemoryData->orderingChannel, participant->playerId, false);

		if (baseMemoryData->synchronizedMemory->IsUnifiedMemory()==false || baseMemoryData->didComparisonAndCopy==false)
		{
			extendedMemoryData->lastMemorySend->CopySynchronizedDataFrom(baseMemoryData->synchronizedMemory);
			baseMemoryData->didComparisonAndCopy=true;
		}		

		extendedMemoryData->lastSendTime=time;
	}
	else
	{
		// (this is an object)
		baseObjectData = (BaseObjectData*) baseData;
		extendedObjectData = (ExtendedObjectData*) extendedData;

		// Check to see if this object is currently in scope
		validScope=(bool)baseObjectData->synchronizedObject->InScope(participant->playerId);

		// Is this object instantiated?
		if (extendedObjectData->objectInstantiated)
		{
			// Is the scope different?
			if (validScope!=extendedObjectData->objectInScope)
			{
				outBitStream.Write((unsigned char)ID_REPLICATOR_DATA_SEND_OBJECT_SCOPE);

				// Encode object ID
				outBitStream.Write(baseObjectData->synchronizedObject->GetNetworkID());
				
				// Encode new scope
				outBitStream.Write(validScope);
                
				// Send the packet
				rakPeer->Send(&outBitStream, baseObjectData->priority, baseObjectData->reliability, baseObjectData->orderingChannel, participant->playerId, false);

				// Copy new scope to old scope.
				extendedObjectData->objectInScope=validScope;
				return;
			}
		}
		else
		{
			//  Is in scope?
			if (validScope)
			{
				ObjectReplicationHandler* objectReplicationHandler = ObjectReplicationHandler::GetHandler(baseObjectData->synchronizedObject->GetObjectClassName());
				if (objectReplicationHandler)
				{
					outBitStream.Write((unsigned char)ID_REPLICATOR_DATA_PUSH_OBJECT);

					// Encode objectID that the other system will use.
					outBitStream.Write(baseObjectData->synchronizedObject->GetNetworkID());

					// Encode remote key so the other system can look up the name of the object to create.  The remote
					// system doesn't yet have an assigned objectID.
					EncodeClassName(baseObjectData->synchronizedObject->GetObjectClassName(), participant->playerId, &outBitStream);
					

					int createObject;
					createObject = objectReplicationHandler->OnReplicationPush(participant->playerId, baseObjectData->synchronizedObject->GetObjectClassName(), &outBitStream, baseObjectData->synchronizedObject);
                    if (createObject==OBJECT_REPLICATION_OK)
					{
						// Send the packet
						rakPeer->Send(&outBitStream, baseObjectData->priority, baseObjectData->reliability, baseObjectData->orderingChannel, participant->playerId, false);

						// Record what we last sent by setting objectInScope and objectInstantiated to true
						extendedObjectData->objectInScope=true;
						extendedObjectData->objectInstantiated=true;
						return;
					}
					else if (createObject==OBJECT_REPLICATION_NEVER_PUSH)
					{
						// Never create the object for this player
						RemoveExtendedDataFromParticipant(baseData, participant, false, false);
					}
					// else try again later
				}
				else
				{
#ifdef _DEBUG
					assert(0);
#endif
				}
			}
			//  else do nothing
		}
	}

}
void DataReplicator::SendMemoryStartRequest(BaseMemoryData *baseData, ParticipantStruct *participant)
{
	// Make sure the object associated with this memory gets sent out before the memory does
	if (baseData->associatedObject!=UNASSIGNED_OBJECT_ID)
		OnUpdate(rakPeer);

	RakNet::BitStream outBitStream;

	// Encode the packet ID
	outBitStream.Write((unsigned char)ID_REPLICATOR_MEMORY_START);

	// Encode the local key
	outBitStream.Write(baseData->localKey);

	// Encode the string ID
	stringCompressor->EncodeString(baseData->localIdentifier, 256, &outBitStream);
	
	// Send the packet
	rakPeer->Send(&outBitStream, baseData->priority, baseData->reliability==RELIABLE_ORDERED ? RELIABLE_ORDERED : RELIABLE, baseData->orderingChannel, participant->playerId, false);	
}
void DataReplicator::SendDataStopRequest(BaseData *baseData, ParticipantStruct *participant, bool isMemory)
{
	RakNet::BitStream outBitStream;

	// Encode the packet ID
	outBitStream.Write((unsigned char)ID_REPLICATOR_DATA_STOP);

	// Encode if this is memory or not
	outBitStream.Write(isMemory);

	if (isMemory)
	{
		BaseMemoryData *baseMemoryData = (BaseMemoryData *) baseData;

		// Encode the local identifier (not the key, in case the other system didn't get the key yet).
		stringCompressor->EncodeString(baseMemoryData->localIdentifier, 256, &outBitStream);
	}
	else
	{
		// Encode the object ID
		BaseObjectData *baseObjectData = (BaseObjectData *) baseData;
		outBitStream.Write(baseObjectData->synchronizedObject->GetNetworkID());
	}

	// Send the packet
	rakPeer->Send(&outBitStream, baseData->priority, baseData->reliability==RELIABLE_ORDERED ? RELIABLE_ORDERED : RELIABLE, baseData->orderingChannel, participant->playerId, false);	
}
DataReplicator::BaseMemoryData* DataReplicator::GetBaseMemoryDataByString(const char *str)
{
	unsigned i;
	for (i=0; i < memoryList.Size(); i++)
		if (strcmp(memoryList[i]->localIdentifier, str)==0)
			return memoryList[i];
	return 0;
}
void DataReplicator::AddToParticipantList(PlayerID playerId)
{
	// If this participant already exists, don't add because this call needs to be re-entrant
	if (GetParticipantByPlayerID(playerId))
		return;

	// If this participant is not connected, don't add them.  This reduces processing and fixes problems determining what to do for
	// participants with no data.
	if (rakPeer->GetIndexFromPlayerID(playerId)==(unsigned)-1)
		return;

	unsigned i;

	// Allocate a new ParticipantStruct
	ParticipantStruct *participantStruct;
	participantStruct = new ParticipantStruct;
	
	// Set the playerID of that ParticipantStruct
	participantStruct->playerId=playerId;

	// Add the new ParticipantStruct to participantList
	participantList.InsertElement(participantStruct, participantStruct->playerId, ParticipantStructComp);

	// For each memory item data, if we receive updates, then send a data start request
	for (i=0; i < memoryList.Size(); i++)
		if (memoryList[i]->receiveUpdates)
			SendMemoryStartRequest(memoryList[i], participantStruct);

	// For each object, if we send updates, add it to the list.
	for (i=0; i < objectList.Size(); i++)
		if (objectList[i]->sendUpdates)
			AddExtendedObjectDataToParticipant(objectList[i], participantStruct);
}
void DataReplicator::RemoveFromParticipantList(PlayerID playerId, bool sendDataStopRequests)
{
	ParticipantStruct *participantStruct;
	unsigned index;

	index = GetParticipantIndexByPlayerID(playerId);

	if (index==(unsigned)-1) // Yes, I know this is unsigned
		return;

	participantStruct=participantList[index];

	while (participantStruct->extendedMemoryList.Size())
		RemoveExtendedDataFromParticipant(participantStruct->extendedMemoryList[participantStruct->extendedMemoryList.Size()-1]->baseData, participantStruct, true, sendDataStopRequests);

	while (participantStruct->extendedObjectList.Size())
		RemoveExtendedDataFromParticipant(participantStruct->extendedObjectList[participantStruct->extendedObjectList.Size()-1]->baseData, participantStruct, false, sendDataStopRequests);
           
	delete participantStruct;
	participantList.RemoveElementAtIndex(index);
}

void DataReplicator::RemoveAllParticipants(void)
{
	while (participantList.Size())
		RemoveFromParticipantList(participantList[participantList.Size()-1]->playerId, false);
}
unsigned short DataReplicator::GenerateMemoryStringMappingKey(void)
{
	unsigned short newKey;

	// Increment the key as long as the key is in use.  Return the new key
	do {
		newKey=memoryToStringMappingKey++;
	} while(newKey==65535 || GetMemoryDataByKey(newKey));

	return newKey;
}
DataReplicator::BaseMemoryData *DataReplicator::GetMemoryDataByKey(unsigned short key)
{
	bool objectExists;
	unsigned index;

	index = memoryList.GetIndexFromKey(key, &objectExists, BaseMemoryDataComp);
	if (objectExists==false)
		return 0;

	return memoryList[index];
}
unsigned DataReplicator::GetParticipantIndexByPlayerID(PlayerID playerId)
{
	bool objectExists;
	unsigned index = participantList.GetIndexFromKey(playerId, &objectExists, ParticipantStructComp);
	if (objectExists==false)
		return (unsigned)-1;
	return index;
}
DataReplicator::ParticipantStruct *DataReplicator::GetParticipantByPlayerID(PlayerID playerId)
{
	unsigned index = GetParticipantIndexByPlayerID(playerId);
	if (index==(unsigned)-1)
		return 0;
	return participantList[index];
}
DataReplicator::ExtendedObjectData* DataReplicator::GetExtendedObjectByObjectID(ObjectID objectId, ParticipantStruct *participant)
{
	unsigned index;
	ExtendedObjectData *extendedObjectData;
	BaseObjectData *baseObjectData;
	for (index=0; index < participant->extendedObjectList.Size(); index++)
	{
		extendedObjectData=participant->extendedObjectList[index];
		baseObjectData=(BaseObjectData *)extendedObjectData->baseData;
		if (baseObjectData->synchronizedObject->GetNetworkID()==objectId)
			return extendedObjectData;
	}
	return 0;
}
DataReplicator::BaseObjectData* DataReplicator::GetBaseObjectByObjectID(ObjectID objectId)
{
	bool objectExists;
	unsigned index = objectList.GetIndexFromKey(objectId, &objectExists, BaseObjectDataComp);
	if (objectExists==false)
		return 0;
	return objectList[index];
}
DataReplicator::BaseMemoryData *DataReplicator::GetBaseMemoryDataBySynchronizedMemory(SynchronizedMemory *memory)
{
	unsigned i;
	for (i=0; i < memoryList.Size(); i++)
		if (memoryList[i]->synchronizedMemory==memory)
			return memoryList[i];
	return 0;
}

#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void DataReplicator::OnDataOverflow(RakPeerInterface *peer, Packet *packet)
{
#ifdef _DEBUG
	assert(0);
#endif
}
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void DataReplicator::OnDataUnderflow(RakPeerInterface *peer, Packet *packet)
{
#ifdef _DEBUG
	assert(0);
#endif
}
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void DataReplicator::OnInvalidPacket(RakPeerInterface *peer, Packet *packet)
{
#ifdef _DEBUG
	assert(0);
#endif
}
void DataReplicator::OnAttach(RakPeerInterface *peer)
{
	rakPeer=peer;
}
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void DataReplicator::OnUpdate(RakPeerInterface *peer)
{
	unsigned i,j;
	ParticipantStruct *participantStruct;
	ExtendedData *extendedData;
	bool didMemorySend;
	unsigned int currentTime;

	for (i=0; i < memoryList.Size(); i++)
	{
		memoryList[i]->doUpdate=false;
		memoryList[i]->didComparisonAndCopy=false;
	}

	for (i=0; i < objectList.Size(); i++)
		objectList[i]->doUpdate=false;

	if (participantList.Size()>0)
	{
		currentTime=RakNet::GetTime(); // Do this because the GetTime call is slow

		for (i=0; i < participantList.Size(); i++)
		{
			participantStruct=participantList[i];

			for (j=0; j < participantStruct->extendedObjectList.Size(); j++)
			{
				extendedData=participantStruct->extendedObjectList[j];

				// If time to update then do a validated send
				if ( extendedData->baseData->doUpdate || extendedData->baseData->lastUpdateTime + extendedData->baseData->maxUpdateFrequencyMS < currentTime)
				{
					extendedData->baseData->doUpdate=true;
					ValidatedSend(extendedData, participantStruct, false);
					extendedData->baseData->lastUpdateTime=currentTime;
				}
			}

			for (didMemorySend=false, j=0; j < participantStruct->extendedMemoryList.Size(); j++)
			{
				extendedData=participantStruct->extendedMemoryList[j];

				if ( extendedData->baseData->doUpdate || extendedData->baseData->lastUpdateTime + extendedData->baseData->maxUpdateFrequencyMS < currentTime)
				{
					extendedData->baseData->doUpdate=true;

					ValidatedSend(extendedData, participantStruct, true);
					
					extendedData->baseData->lastUpdateTime=currentTime;
				}
			}			
		}
	}
}
// We got a memory update
void DataReplicator::OnDataReplicateSendMemory(RakPeerInterface *peer, Packet *packet)
{
	RakNet::BitStream incomingBitStream((char*)packet->data, packet->length, false);
	unsigned char typeId;

	incomingBitStream.Read(typeId);

	unsigned int timePacketSent;
	if (typeId==ID_TIMESTAMP)
	{
		incomingBitStream.Read(timePacketSent);
		incomingBitStream.IgnoreBits(8);
	}
	else
		timePacketSent=0;

	unsigned short transmittedKey;
	if (!incomingBitStream.Read(transmittedKey))
	{
		OnDataUnderflow(peer, packet);
		return;
	}

	ParticipantStruct *participantStruct=GetParticipantByPlayerID(packet->playerId);
	if (participantStruct==0)
	{
		// Got an update from someone that is not a participant
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	BaseMemoryData *baseMemoryData = (BaseMemoryData *)GetMemoryDataByKey(transmittedKey);
	if (baseMemoryData && baseMemoryData->receiveUpdates)
	{
		// Decode new data value
		baseMemoryData->synchronizedMemory->Deserialize(&incomingBitStream, timePacketSent);
	}
	else
	{
#ifdef _DEBUG
		// This only matters in debug for my own testing - we got a memory update for unknown or unwritable memory.  This happens normally in the regular situation
		assert(0);
#endif
	}

}
// We got a packet telling us to create an object and that that object is in scope
void DataReplicator::OnReplicationPushPacket(RakPeerInterface *peer, Packet *packet)
{
	RakNet::BitStream incomingBitStream((char*)packet->data, packet->length, false);
	incomingBitStream.IgnoreBits(8);

	ObjectID objectId;

	ParticipantStruct *participantStruct=GetParticipantByPlayerID(packet->playerId);
	if (participantStruct==0)
	{
		// Got an update from someone that is not a participant
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

    incomingBitStream.Read(objectId);
	if (objectId==UNASSIGNED_OBJECT_ID)
	{
		OnInvalidPacket(peer, packet);
		return;
	}
	
	char className[256];
	ObjectReplicationHandler* objectReplicationHandler = DecodeClassName(className, participantStruct, &incomingBitStream, peer, packet);
	if (objectReplicationHandler==0)
	{
#ifdef _DEBUG
		// Only matters in debug for an unknown string.  In production this could happen anytime from hackers or cheaters.
		assert(0);
#endif
		return;
	}

	SynchronizedObject* newObject = objectReplicationHandler->OnReplicationPushNotification(packet->playerId, className, &incomingBitStream, objectId);
	OnCreationCommand(newObject, packet->playerId, objectId);
}
// Packet telling us an object's scope has changed
void DataReplicator::OnDataReplicateSendObjectScope(RakPeerInterface *peer, Packet *packet)
{
	RakNet::BitStream incomingBitStream((char*)packet->data, packet->length, false);

	// SetWriteOffset is used here to get around a design flaw, where I should have had the bitstream constructor take bits, rather than bytes
	// It sets the actual number of bits in the packet
	incomingBitStream.SetWriteOffset(packet->bitSize);
	incomingBitStream.IgnoreBits(8);

	ObjectID objectId;
	bool newScope;

	ParticipantStruct *participantStruct=GetParticipantByPlayerID(packet->playerId);
	if (participantStruct==0)
	{
		// Got an update from someone that is not a participant
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	incomingBitStream.Read(objectId);
	if (!incomingBitStream.Read(newScope))
	{
		OnDataUnderflow(peer, packet);
		return;
	}

	BaseObjectData *baseObjectData = GetBaseObjectByObjectID(objectId);

	if (baseObjectData && baseObjectData->receiveUpdates)
	{
		baseObjectData->synchronizedObject->OnScopeChange(packet->playerId, newScope);

		if (incomingBitStream.GetNumberOfUnreadBits()!=0)
			OnDataOverflow(peer,packet);
	}
	else
	{
#ifdef _DEBUG
		// This only matters in debug for my own testing - we got a memory update for unknown or unwritable memory.  This happens normally in the regular situation
		assert(0);
#endif
	}
}
// Packet telling us to start sending data.  This will add an extended element to
// the participant list assuming that extended element is set to send.
// It also sets up a string to remote key mapping
void DataReplicator::OnMemoryReplicateStart(RakPeerInterface *peer, Packet *packet)
{
	char identifier[256];
	unsigned short transmittedKey;

	ParticipantStruct *participantStruct=GetParticipantByPlayerID(packet->playerId);
	if (participantStruct==0)
	{
		// Got an update from someone that is not a participant
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	identifier[0]=0;

	RakNet::BitStream incomingBitStream((char*)packet->data, packet->length, false);
	incomingBitStream.SetWriteOffset(packet->bitSize);
	incomingBitStream.IgnoreBits(8);

	incomingBitStream.Read(transmittedKey);

	if (!stringCompressor->DecodeString(identifier, 256, &incomingBitStream))
	{
		OnDataUnderflow(peer, packet);
		return;
	}

	// Make sure the base data does not already have this player set to send data
	BaseMemoryData *baseData = GetBaseMemoryDataByString(identifier);
	if (baseData==0)
	{
		// We don't have memory registered with this string
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	if (baseData->sendUpdates==false)
	{
		// Asked to start sending updates when this is receive only
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	// If this participant already has this key, this is a duplicate packet
	if (participantStruct->GetExtendedMemoryIndexByKey(transmittedKey)!=(unsigned)-1)
	{
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	ExtendedMemoryData *extendedData = new ExtendedMemoryData;
	extendedData->remoteKey=transmittedKey;
	extendedData->baseData=baseData;
	extendedData->lastMemorySend=0;
	extendedData->lastSendTime=0;
	
	participantStruct->extendedMemoryList.InsertElement(extendedData, extendedData->baseData, ExtendedMemoryDataComp);

	if (incomingBitStream.GetNumberOfUnreadBits()!=0)
		OnDataOverflow(peer, packet);
}
// Packet telling us to stop sending data.  This will remove an extended element in
// the participant list assuming that extended element is set to send
void DataReplicator::OnDataReplicateStop(RakPeerInterface *peer, Packet *packet)
{
	RakNet::BitStream incomingBitStream((char*)packet->data, packet->length, false);
	incomingBitStream.SetWriteOffset(packet->bitSize);
	incomingBitStream.IgnoreBits(8);

	ParticipantStruct *participantStruct=GetParticipantByPlayerID(packet->playerId);
	if (participantStruct==0)
	{
		// Got an update from someone that is not a participant
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	bool isMemory;
	BaseData *baseData;
	incomingBitStream.Read(isMemory);
	if (isMemory)
	{
		char str[256];
		if (!stringCompressor->DecodeString(str, 256, &incomingBitStream))
		{
			OnDataUnderflow(peer, packet);
			return;
		}
		baseData = GetBaseMemoryDataByString(str);
	}
	else
	{
		ObjectID objectId;
		if (!incomingBitStream.Read(objectId))
		{
			OnDataUnderflow(peer, packet);
			return;
		}
		baseData = GetBaseObjectByObjectID(objectId);
	}

	if (baseData==0)
	{
		// Don't know what they are referring to
		return;
	}

	
	if (isMemory)
	{
		if (baseData->sendUpdates)
			RemoveExtendedDataFromParticipant(baseData, participantStruct, true, false);
		else
			UnsynchronizeMemory(((BaseMemoryData*)baseData)->synchronizedMemory, false);			
	}
	else
	{
		SynchronizedObject *object = ((BaseObjectData*)baseData)->synchronizedObject;
		bool unsynchronizeObject = object->AcceptDesynchronization(packet->playerId);

		if (baseData->sendUpdates)
		{
			if (unsynchronizeObject)
			{
				RemoveExtendedDataFromParticipant(baseData, participantStruct, false, false);
				UnsynchronizeObjectInt(object, true, packet->playerId);				
			}
			else
			{
				// Remove the object for only the sender.
				RemoveExtendedDataFromParticipant(baseData, participantStruct, false, false);
			}
		}
		else
		{
			if (unsynchronizeObject)
			{
				// Remove the object for ourselves
				UnsynchronizeObjectInt(object, false, packet->playerId);
			}
		}
	}
}
void DataReplicator::OnNewPlayer(RakPeerInterface *peer, Packet *packet)
{
	// AddParticipant for this player if broadcast to all.
	// Otherwise it's up to the user to add them.
	if (broadcastToAll)
		AddParticipant(packet->playerId);	
}
void DataReplicator::OnLostPlayer(RakPeerInterface *peer, Packet *packet)
{
	// RemoveParticipant for this playerId
	RemoveParticipant(packet->playerId);
}
void DataReplicator::EncodeClassName(const char *strIn, PlayerID playerId, RakNet::BitStream *bitStream)
{
	ParticipantStruct *participantStruct=GetParticipantByPlayerID(playerId);
	if (participantStruct==0)
	{
		// Got an update from someone that is not a participant
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	unsigned i;
	for (i=0; i < participantStruct->remoteObjectNameTable.size(); i++)
	{
		if (participantStruct->remoteObjectNameTable[i] && strcmp(strIn, participantStruct->remoteObjectNameTable[i])==0)
		{
			bitStream->Write(false);
			bitStream->Write((strToIndexMapType) i);
			return;
		}
	}

	bitStream->Write(true);
	stringCompressor->EncodeString((char*)strIn, 256, bitStream);
}
void DataReplicator::OnStringMapIndex(RakPeerInterface *peer, Packet *packet)
{
	RakNet::BitStream incomingBitStream((char*)packet->data, packet->length, false);
	incomingBitStream.SetWriteOffset(packet->bitSize);
	incomingBitStream.IgnoreBits(8);

	strToIndexMapType index;
	char str[256];

	incomingBitStream.Read(index);
	if (!stringCompressor->DecodeString(str, 256, &incomingBitStream))
	{
		OnDataUnderflow(peer, packet);
		return;
	}

	if (index==STR_TO_INDEX_MAP_MAX)
	{
		OnInvalidPacket(peer, packet);
		return;
	}

	if (incomingBitStream.GetNumberOfUnreadBits()!=0)
	{
		OnDataOverflow(peer,packet);
		return;
	}

	if (str[0]==0)
	{
		OnInvalidPacket(peer, packet);
		return;
	}

	ParticipantStruct *participantStruct=GetParticipantByPlayerID(packet->playerId);
	if (participantStruct==0)
	{
		// Got an update from someone that is not a participant
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	if (index < participantStruct->remoteObjectNameTable.size() && participantStruct->remoteObjectNameTable[index]!=0)
	{
		// Make sure the same string is sent to the same slot
#ifdef _DEBUG
		assert(strcmp(participantStruct->remoteObjectNameTable[index], str)==0);
#endif
		return;
	}

	char *allocatedStr = new char [strlen(str)+1];
	strcpy(allocatedStr, str);
	participantStruct->remoteObjectNameTable.replace(allocatedStr,0,index);
}
ObjectReplicationHandler* DataReplicator::DecodeClassName(char *strOut, ParticipantStruct *participantStruct, RakNet::BitStream *bitStream, RakPeerInterface *peer, Packet *packet)
{
	bool hasEncodedString;
	strToIndexMapType indexToSend=STR_TO_INDEX_MAP_MAX;
	bitStream->Read(hasEncodedString);
	if (hasEncodedString)
	{
		if (!stringCompressor->DecodeString(strOut, 256, bitStream))
		{
			OnDataUnderflow(peer, packet);
			return 0;
		}

		if (strOut[0]==0)
			return 0;

		if (strlen(strOut)>=256)
			return 0;

		unsigned i;
		bool alreadyHasString=false;
		for (i=0; i < localObjectNameTable.size(); i++)
		{
			if (strcmp(localObjectNameTable[i], strOut)==0)
			{
				alreadyHasString=true;
				break;
			}
		}

        if (alreadyHasString==false)
		{
			// Add a string to the table and send the index
			if (localObjectNameTable.size()+1<STR_TO_INDEX_MAP_MAX)
			{
				char *strCpy;
				strCpy = new char [strlen(strOut)+1];
				strcpy(strCpy, strOut);
				localObjectNameTable.insert(strCpy);

				// Send the index localObjectNameTable.size() with the string
				indexToSend=localObjectNameTable.size()-1;
			}
			else
			{
#ifdef _DEBUG
				// strToIndexMapType is too small
				assert(0);
#endif
			}

		}
		else
		{
			// Send the index i with the string
			indexToSend=i;
		}
	}
	else
	{
		strToIndexMapType indexMap;
		bitStream->Read(indexMap);
		if (indexMap >= localObjectNameTable.size() )
		{
			// Invalid index
#ifdef _DEBUG
			assert(0);
#endif
			return 0;
		}

		if (localObjectNameTable[indexMap]==0)
		{
			// Unknown string
#ifdef _DEBUG
			assert(0);
#endif
			return 0;
		}

		strcpy(strOut, localObjectNameTable[indexMap]);
	}

	ObjectReplicationHandler* objectReplicationHandler = ObjectReplicationHandler::GetHandler(strOut);
	if (objectReplicationHandler==0)
	{
		// Unknown request.  Only something to be concerned about in development.  In release this could happen from cheaters or hackers.
#ifdef _DEBUG
		assert(0);
#endif
		return 0;
	}

	if (indexToSend!=STR_TO_INDEX_MAP_MAX)
	{
		RakNet::BitStream outBitStream;
		outBitStream.Write((unsigned char)ID_REPLICATOR_STR_MAP_INDEX);
		outBitStream.Write(indexToSend);
		stringCompressor->EncodeString(strOut, 256, &outBitStream);
		rakPeer->Send(&outBitStream, HIGH_PRIORITY, UNRELIABLE, 0, participantStruct->playerId, false);
	}

	return objectReplicationHandler;
}
void DataReplicator::OnObjectCreationRequest(RakPeerInterface *peer, Packet *packet)
{
	RakNet::BitStream incomingBitStream((char*)packet->data, packet->length, false);
	incomingBitStream.SetWriteOffset(packet->bitSize);
	incomingBitStream.IgnoreBits(8);

	ParticipantStruct *participantStruct=GetParticipantByPlayerID(packet->playerId);
	if (participantStruct==0)
	{
		// Got an update from someone that is not a participant
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	char className[256];
	ObjectReplicationHandler* objectReplicationHandler = DecodeClassName(className, participantStruct, &incomingBitStream, peer, packet);

	if (objectReplicationHandler==0)
	{
		// Unknown request.  Only something to be concerned about in development.  In release this could happen from cheaters or hackers.
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	RakNet::BitStream outBitStream, context;
	SynchronizedObject* newObject;
	PacketPriority priority;
	PacketReliability reliability;
	char orderingChannel;
	unsigned short maxUpdateFrequencyMS;

	priority=HIGH_PRIORITY;
	reliability=RELIABLE_ORDERED;
	orderingChannel=0;
	maxUpdateFrequencyMS=30;

	newObject = objectReplicationHandler->OnCreationRequest(packet->playerId, className, &incomingBitStream, &context, &priority, &reliability, &orderingChannel, &maxUpdateFrequencyMS);
#ifdef _DEBUG
	// Not a good idea to instantiate objects that are not in scope to the requester or it will just immediately go out of scope.
	// By definition instantiated objects are in-scope
	assert(newObject->InScope(packet->playerId));
#endif

	outBitStream.Write((unsigned char)ID_REPLICATOR_OBJECT_CREATION_REQUEST_RESPONSE);
	//stringCompressor->EncodeString(newObject->GetObjectClassName(), 256, &outBitStream);
	EncodeClassName(newObject->GetObjectClassName(), packet->playerId, &outBitStream);
	outBitStream.Write((bool) (newObject!=0));
	if (newObject)
	{
		outBitStream.Write(newObject->GetNetworkID());
		SynchronizeObjectInt(newObject, priority, reliability, orderingChannel, true, false, maxUpdateFrequencyMS, packet->playerId);
		ExtendedObjectData *extendedObjectData = GetExtendedObjectByObjectID(newObject->GetNetworkID(), GetParticipantByPlayerID(packet->playerId));
		extendedObjectData->objectInScope=true;
		extendedObjectData->objectInstantiated=true;
	}

	outBitStream.WriteBits(context.GetData(), context.GetNumberOfBitsUsed());
	
	rakPeer->Send(&outBitStream, priority, reliability, orderingChannel, packet->playerId, false);
}
void DataReplicator::OnObjectCreationRequestResponse(RakPeerInterface *peer, Packet *packet)
{
	RakNet::BitStream incomingBitStream((char*)packet->data, packet->length, false);
	incomingBitStream.SetWriteOffset(packet->bitSize);
	incomingBitStream.IgnoreBits(8);
	bool requestAccepted;
	ObjectID objectId;

	ParticipantStruct *participantStruct=GetParticipantByPlayerID(packet->playerId);
	if (participantStruct==0)
	{
		// Got an update from someone that is not a participant
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	// Record requests so we don't accept responses when we never sent a request to begin with.
	// Record request by player rather than by object so the player can change what kind of object is created
	if (participantStruct->objectRequestCount==0)
	{
		// Got more object request responses than we sent requests for!
		OnInvalidPacket(peer, packet);
		return;
	}

	participantStruct->objectRequestCount--;

	char className[256];
	ObjectReplicationHandler* objectReplicationHandler = DecodeClassName(className, participantStruct, &incomingBitStream, peer, packet);

	if (objectReplicationHandler==0)
	{
		// Unknown class type response.  Only something to be concerned about in development.  In release this could happen from cheaters or hackers.
#ifdef _DEBUG
		assert(0);
#endif
		return;
	}

	if (!incomingBitStream.Read(requestAccepted))
	{
		OnDataUnderflow(peer, packet);
		return;
	}

	if (requestAccepted)
	{
		if (!incomingBitStream.Read(objectId))
		{
			OnDataUnderflow(peer, packet);
			return;
		}

		SynchronizedObject* newObject = objectReplicationHandler->OnCreationRequestAcceptance(packet->playerId, className, objectId, &incomingBitStream);
		OnCreationCommand(newObject, packet->playerId, objectId);
	}
	else
	{
		objectReplicationHandler->OnCreationRequestRejection(packet->playerId, className, &incomingBitStream);
	}
}
void DataReplicator::OnCreationCommand(SynchronizedObject* newObject, PlayerID playerId, ObjectID objectId)
{
	if (newObject)
	{
		// If the user didn't set the newtork ID, set it now.
		if (newObject->GetNetworkID()==UNASSIGNED_OBJECT_ID)
			newObject->SetNetworkID(objectId);

		// Synchronize the object so we receive scoping and destruction updates.  The passed parameters don't matter because this is receive only
		// If the user wants a send object they can call SynchronizeObject themselves in OnReplicationPushNotification in which case
		// this call will return silently.
		SynchronizeObjectInt(newObject, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, true, 30, playerId);
	}
	else
	{
		RakNet::BitStream outBitStream;
		// Encode the packet ID
		outBitStream.Write((unsigned char)ID_REPLICATOR_DATA_STOP);
		// False means this is not memory
		outBitStream.Write(false);
		outBitStream.Write(objectId);
		rakPeer->Send(&outBitStream, HIGH_PRIORITY, RELIABLE, 0, playerId, false);
		return;
	}
}
bool DataReplicator::OnReceive(RakPeerInterface *peer, Packet *packet)
{
	assert(packet);
	assert(peer);

	unsigned char packetIdentifier;
	if ( ( unsigned char ) packet->data[ 0 ] == ID_TIMESTAMP )
	{
		if ( packet->length > sizeof( unsigned char ) + sizeof( unsigned int ) )
			packetIdentifier = ( unsigned char ) packet->data[ sizeof( unsigned char ) + sizeof( unsigned int ) ];
		else
			return false;
	}
	else
		packetIdentifier = ( unsigned char ) packet->data[ 0 ];

	switch (packetIdentifier)
	{
	case ID_NEW_INCOMING_CONNECTION:
	case ID_CONNECTION_REQUEST_ACCEPTED:
		OnNewPlayer(peer, packet);
		break;
	case ID_DISCONNECTION_NOTIFICATION:
	case ID_CONNECTION_LOST:
		OnLostPlayer(peer, packet);
		break;
	case ID_REPLICATOR_DATA_SEND_MEMORY:
		OnDataReplicateSendMemory(peer, packet);
		break;
	case ID_REPLICATOR_DATA_PUSH_OBJECT:
		OnReplicationPushPacket(peer, packet);
		break;
	case ID_REPLICATOR_DATA_SEND_OBJECT_SCOPE:
		OnDataReplicateSendObjectScope(peer, packet);
		break;
	case ID_REPLICATOR_MEMORY_START:
		OnMemoryReplicateStart(peer, packet);
		break;
	case ID_REPLICATOR_DATA_STOP:
		OnDataReplicateStop(peer, packet);
		break;
	case ID_REPLICATOR_OBJECT_CREATION_REQUEST:
		OnObjectCreationRequest(peer, packet);
		break;
	case ID_REPLICATOR_OBJECT_CREATION_REQUEST_RESPONSE:
		OnObjectCreationRequestResponse(peer, packet);
        break;
	case ID_REPLICATOR_STR_MAP_INDEX:
		OnStringMapIndex(peer, packet);
		break;
	default:
		// type not used
		return false;
	}
	// If we do not propagate this kind of packet to the game, return true to signal that this class absorbed the packet
	return PropagateToGame(packet)==false;
}
void DataReplicator::OnDisconnect(RakPeerInterface *peer)
{
}
bool DataReplicator::PropagateToGame(Packet *packet) const
{
	unsigned char packetId = packet->data[0];
	return packetId!=ID_REPLICATOR_DATA_PUSH_OBJECT &&
		packetId!=ID_REPLICATOR_DATA_SEND_MEMORY &&
		packetId!=ID_REPLICATOR_DATA_SEND_OBJECT_SCOPE &&
		packetId!=ID_REPLICATOR_MEMORY_START &&
		packetId!=ID_REPLICATOR_DATA_STOP &&
		packetId!=ID_REPLICATOR_OBJECT_CREATION_REQUEST &&
		packetId!=ID_REPLICATOR_OBJECT_CREATION_REQUEST_RESPONSE &&
		packetId!=ID_REPLICATOR_STR_MAP_INDEX;
}
unsigned DataReplicator::ParticipantStruct::GetExtendedMemoryIndexByKey(unsigned short key)
{
	unsigned index;
	for (index=0; index < extendedMemoryList.Size(); index++)
	{
		if (extendedMemoryList[index]->remoteKey==key)
			return index;
	}
	return (unsigned)-1;
}

SynchronizedMemoryType SynchronizedMemory::GetType(void) const
{
	return SYNCHRONIZED_MEMORY_TYPE_NONE;
}

SynchronizedMemoryDefault::SynchronizedMemoryDefault()
{
}

SynchronizedMemoryDefault::~SynchronizedMemoryDefault()
{
}

SynchronizedMemoryDefault::SynchronizedMemoryDefault(const char *data, int byteLength)
{
	userData=(char*)data;
	userByteLength=byteLength;
	assert(data && byteLength);
}

void SynchronizedMemoryDefault::Serialize(RakNet::BitStream *bitstream, SynchronizedMemory *lastSentValue, unsigned int lastSendTime)
{
	bitstream->Write(userData, userByteLength);
}
void SynchronizedMemoryDefault::Deserialize(RakNet::BitStream *bitstream, unsigned int timePacketSent)
{
	bitstream->Read(userData, userByteLength);
}
SynchronizedMemory * SynchronizedMemoryDefault::MakeSynchronizedDataCopy(void)
{
	SynchronizedMemoryDefault *synchronizedMemoryDefault;
	char *dataCopy;
	dataCopy=new char[userByteLength];
	memcpy(dataCopy, userData, userByteLength);
	synchronizedMemoryDefault = new SynchronizedMemoryDefault(dataCopy, userByteLength);
	return synchronizedMemoryDefault;
}
void SynchronizedMemoryDefault::CopySynchronizedDataFrom(SynchronizedMemory *source)
{
	SynchronizedMemoryDefault *synchronizedMemoryDefault = (SynchronizedMemoryDefault *)source;
	memcpy(userData, synchronizedMemoryDefault->userData, userByteLength);
}
void SynchronizedMemoryDefault::ReleaseCopy(void)
{
	delete [] userData;
}
bool SynchronizedMemoryDefault::ShouldSendUpdate(PlayerID destinationSystem, SynchronizedMemory *lastSentValue, unsigned int lastSendTime)
{
	return memcmp(userData, ((SynchronizedMemoryDefault*)lastSentValue)->userData, userByteLength )!=0;
}
bool SynchronizedMemoryDefault::IsUnifiedMemory(void)
{
	return true;
}
bool SynchronizedMemoryDefault::IncludeTimestamp(void)
{
	return false;
}
bool SynchronizedMemoryDefault::AutoDeleteOnUnreference(void)
{
	return true;
}
SynchronizedMemoryType SynchronizedMemoryDefault::GetType(void) const
{
	return SYNCHRONIZED_MEMORY_TYPE_DEFAULT;
}
char *SynchronizedMemoryDefault::GetUserData(void) const
{
	return userData;
}
DataReplicator::BaseMemoryData::BaseMemoryData()
{
	localIdentifier=0;
}
DataReplicator::BaseMemoryData::~BaseMemoryData()
{
	if (localIdentifier)
		delete [] localIdentifier;
}

DataReplicator::ParticipantStruct::ParticipantStruct()
{
	objectRequestCount=0;
}
DataReplicator::ParticipantStruct::~ParticipantStruct()
{
	unsigned i;
	for (i=0; i < extendedMemoryList.Size(); i++)
		delete extendedMemoryList[i];
	for (i=0; i < extendedObjectList.Size(); i++)
		delete extendedObjectList[i];
	for (i=0; i < remoteObjectNameTable.size(); i++)
		delete remoteObjectNameTable[i];
	extendedMemoryList.Clear();
	extendedObjectList.Clear();
	remoteObjectNameTable.clear();
}
void ObjectReplicationHandler::RegisterHandler(ObjectReplicationHandler *handler)
{
	assert(GetHandler(handler->GetHandlerClassName())==0);
	//handlerRegistry.InsertElement(handler, handler->GetHandlerClassName(), ObjectReplicationHandlerComp);
	if (handlerRegistryHead==0)
		handlerRegistryHead=handler;
	else
	{
		ObjectReplicationHandler *ptr = handlerRegistryHead;
		while (ptr->next)
			ptr=ptr->next;
		ptr->next=handler;
	}
	handler->next=0;

	if (defaultHandler==0)
	{
		if (handler->IsDefaultHandler())
			defaultHandler=handler;
	}
	else
	{
		// Can't have more than one default handler
		assert(handler->IsDefaultHandler()==false);
	}
}

void ObjectReplicationHandler::UnregisterHandler(ObjectReplicationHandler *handler)
{
	assert(handler);
//	assert(handlerRegistry.HasData(handler->GetHandlerClassName(), ObjectReplicationHandlerComp)==true);
//	handlerRegistry.RemoveElement(handler->GetHandlerClassName(),  ObjectReplicationHandlerComp);

	if (handlerRegistryHead==0)
		return;

	if (handlerRegistryHead==handler)
	{
		handlerRegistryHead=handlerRegistryHead->next;
	}
	else
	{
		ObjectReplicationHandler *ptr = handlerRegistryHead;
		while (ptr)
		{
			if (ptr->next==handler)
			{
				ptr->next=handler->next;
				break;
			}
			ptr=ptr->next;
		}
	}

	if (defaultHandler==handler)
		defaultHandler=0;
}

ObjectReplicationHandler* ObjectReplicationHandler::GetHandler(const char *str)
{
	ObjectReplicationHandler *ptr = handlerRegistryHead;
	while (ptr)
	{
		if (strcmp(ptr->GetHandlerClassName(), str)==0)
			return ptr;
		ptr=ptr->next;
	}

//	bool objectExists;
//	unsigned index = handlerRegistry.GetIndexFromKey(str, &objectExists, ObjectReplicationHandlerComp);
//	if (objectExists)
//		return handlerRegistry[index];
	if (defaultHandler)
		return defaultHandler;
	return 0;
}

bool ObjectReplicationHandler::IsDefaultHandler(void) const
{
	return false;
}

