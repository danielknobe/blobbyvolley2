/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file DataReplicator.h
* @brief Provides a way to automatically update and synchronize shared memory and objects
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

#ifndef __DATA_REPLICATOR_H
#define __DATA_REPLICATOR_H

#include "BitStream.h"
#include "NetworkTypes.h"
#include "OrderedList.h"
#include "MessageHandlerInterface.h"
#include "PacketPriority.h"
#include "NetworkIDGenerator.h"
#include "BitStream.h"
#include "ArrayList.h"

class RakPeerInterface;
class SynchronizedMemory;
class SynchronizedObject;
class SynchronizedObjectFactory;
class ObjectReplicationHandler;

typedef unsigned char strToIndexMapType;
#define STR_TO_INDEX_MAP_MAX (strToIndexMapType)(strToIndexMapType(0)-1)


// The memory synchronizer takes a string identifier for a block of memory and a pointer to that memory.
// The memory can either be an unknown type, in which case memcmp and memcpy is used, or it can be a pointer to a class that
// implements SynchronizedMemory, in which case the implementation does comparisons in ShouldSendUpdate and copying in Serialize, Deserialize, and CopyTo
class DataReplicator : public MessageHandlerInterface
{
public:
	DataReplicator();
	~DataReplicator();

	void SetBroadcastToAll(bool value);
	bool GetBroadcastToAll(void) const;

	void AddParticipant(PlayerID playerId);
	void RemoveParticipant(PlayerID playerId);

	// Associates memory of size memoryByteLength with a string identifier, or a class that implements SynchronizedMemory.
	// stringID and synchronizedObjectId must be a unique pair, where optionalInstanceID
	// is used to differentiate different instances of a class that all registered a memory block with the same stringId.
	// If synchronizedObjectId used, this must be of type SynchronizedObject.  The memory will only be synchronized if that object is in scope
	// maxUpdateFrequencyMS is the maximum update rate in ms to check if memory has changed and to send that update.
	// priority, reliability, and orderingChannel are equivalent to RakNet's send parameters.
	// send and receive updates determine whether we send memory changes to other systems and whether we accept memory changes from other systems
	void SynchronizeMemory(const char *stringId, const char *memory, int memoryByteLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool sendUpdates, bool receiveUpdates, int maxUpdateFrequencyMS, ObjectID synchronizedObjectId);
    void SynchronizeMemory(const char *stringId, SynchronizedMemory *memory, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool sendUpdates, bool receiveUpdates, int maxUpdateFrequencyMS, ObjectID synchronizedObjectId);

	// Unsynchronizes memory.  It's up to the user to unsynchronize memory after it is deleted.  Not doing this will result in a crash.
	void UnsynchronizeMemory(const char *memoryPtr, bool sendNotificationPacket);
	void UnsynchronizeMemory(SynchronizedMemory *memory, bool sendNotificationPacket);

	// Request that the other system create an object with objectName, where objectName matches a string returned by
	// ObjectReplicationHandler::GetHandlerClassName
	// context is any data you want.  It will be sent to the remote system and parsed in ObjectReplicationHandler::OnCreationRequest
	// The remainder of the parameters are used for RakNet's send call.
	// You will get back either ObjectReplicationHandler::OnCreationRequestAcceptance or ObjectReplicationHandler::OnCreationRequestRejection
	void RequestObjectCreation(const char *objectName, RakNet::BitStream *context, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId);

	// Synchronize or unsynchronize a pointer to an object.
	// If a sender, this will will create the same object on other systems.
	// If a receiver, this will allow updates for this object.  Generally you only need to call this for senders, since recievers are automatically
	// created and synchronized by the ObjectReplicationHandler class.
	// This will also relay scope changes, when the value returned by SynchronizedObject::InScope is different than the last call.
	void SynchronizeObject(SynchronizedObject *object, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool sendUpdates, bool receiveUpdates, int maxUpdateFrequencyMS);
	void UnsynchronizeObject(SynchronizedObject *object, bool sendNotificationPacket);

	// Frees all memory and releases all participants
	void Clear(void);

	// --------------------------------------------------------------------------------------------
	// Overridable event callbacks
	// --------------------------------------------------------------------------------------------
	// Too much data was in a packet.  User should override in a derived class to handle this
	virtual void OnDataOverflow(RakPeerInterface *peer, Packet *packet);
	// Not enough data was in a packet.  User should override in a derived class to handle this
	virtual void OnDataUnderflow(RakPeerInterface *peer, Packet *packet);
	// Packet data that just doesn't make sense
	virtual void OnInvalidPacket(RakPeerInterface *peer, Packet *packet);

	// --------------------------------------------------------------------------------------------
	// Used by RakNet
	// --------------------------------------------------------------------------------------------
	void OnAttach(RakPeerInterface *peer);
	void OnUpdate(RakPeerInterface *peer);
	bool OnReceive(RakPeerInterface *peer, Packet *packet);
	void OnDisconnect(RakPeerInterface *peer);
	bool PropagateToGame(Packet *packet) const;


	// Used as a container for all the shared synchronized data - both memory and objects
	struct BaseData
	{
		// Store all the parameters from the synchronize call
		PacketPriority priority;
		PacketReliability reliability;
		char orderingChannel;
		bool receiveUpdates, sendUpdates;

		// Utility variable so we know to do an update for the rest of the calls in OnUpdate.
		bool doUpdate;

		// Keeps track of when we updated so we know when to update again
		int maxUpdateFrequencyMS;
		unsigned int lastUpdateTime;
	};

	// Specific data for locally synchronized memory.  Sorted by localKey for fast lookups on memory updates
	struct BaseMemoryData : public BaseData
	{
		BaseMemoryData();

		// deletes identifier, which is an allocated string
		~BaseMemoryData();

		// Memory can be associated with objectIDs.  This will be used to see if an object is in scope
		ObjectID associatedObject;

		// References the actual memory in the game
		SynchronizedMemory *synchronizedMemory;

		// Memory across systems share unique identifiers so we can do lookups.
		char *localIdentifier;

		// Rather than send identifiers, we send the local key of each system to save bandwidth
		unsigned short localKey;

		// Used for unified memory.  Records if we did a comparison check and a copy or not via didShouldSend.
		// This way only one compare and copy is done, and if one has been done we use the recorded value.
		bool didComparisonAndCopy, dataDifferent;
	};

	// Specific data for locally synchronized objects.  Sorted by ObjectID
	struct BaseObjectData : public BaseData
	{
		// The object referenced in the game.  We do lookups by objectID, which is in inherent property of the object.
		SynchronizedObject *synchronizedObject;
	};

	// Base class for per-remote system data.  Used to track what we last sent to each system so we can have system contextual sends
	// rather than always broadcasting.
	struct ExtendedData
	{
		// Extended data only holds the specific per-system data, and not the base object we are sending.
		// baseData points to the base object so we can do lookups.
		BaseData *baseData;
	};

	// Copies of the memory we sent to each remote system.  If unified, all copies point to the same memory block.
	struct ExtendedMemoryData : public ExtendedData
	{
		
		// Track what we last sent to this system
		SynchronizedMemory *lastMemorySend;
		unsigned int lastSendTime;

		// This is the local key on the remote system. 65535 means undefined.
		// This is initially sent to us when the remote system asks us to send us this memory.
		// We use this, rather than strings, to refer to memory blocks to save bandwidth
		unsigned short remoteKey;
	};

	struct ExtendedObjectData : public ExtendedData
	{
		// Tracks the state of the object on the remote system so we know when to send or not.
		bool objectInstantiated;
		bool objectInScope;
	};

	struct ParticipantStruct
	{
		ParticipantStruct();
		~ParticipantStruct();

		// Which player does this struct represent?
		PlayerID playerId;

		// Sorted by base pointer integer value.
		BasicDataStructures::OrderedList<ExtendedMemoryData*, BaseData *> extendedMemoryList;
		BasicDataStructures::OrderedList<ExtendedObjectData*, BaseData *> extendedObjectList;

		// Unsorted
		BasicDataStructures::List<char*> remoteObjectNameTable;

		unsigned GetExtendedMemoryIndexByKey(unsigned short key);
		unsigned objectRequestCount;
	};

protected:

	void EncodeClassName(const char *strIn, PlayerID playerId, RakNet::BitStream *bitStream);
	void OnStringMapIndex(RakPeerInterface *peer, Packet *packet);
	ObjectReplicationHandler* DecodeClassName(char *strOut, ParticipantStruct *participantStruct, RakNet::BitStream *bitStream, RakPeerInterface *peer, Packet *packet);
	void UnsynchronizeObjectInt(SynchronizedObject *object, bool sendNotificationPacket, PlayerID playerId);
	void SynchronizeObjectInt(SynchronizedObject *object, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool sendUpdates, bool receiveUpdates, int maxUpdateFrequencyMS, PlayerID requestingSystem);
	void AddParticipantInt(PlayerID playerId);
	void AddExtendedObjectDataToParticipant(BaseObjectData *baseObjectData, ParticipantStruct *participantStruct);
	void RemoveExtendedDataFromParticipant(BaseData *baseData, ParticipantStruct *participantStruct, bool isMemory, bool sendNotificationPacket);
	void SendDataStopRequest(BaseData *baseData, ParticipantStruct *participant, bool isMemory);
	void ValidatedSend(ExtendedData *extendedData, ParticipantStruct *participant, bool isMemory);
	BaseMemoryData* GetBaseMemoryDataByString(const char *str);
	ParticipantStruct *GetParticipantByPlayerID(PlayerID playerId);
	unsigned GetUnifiedMemoryCount(SynchronizedMemory *memory);
	SynchronizedMemory* GetSynchronizedMemoryCopy(BaseMemoryData *baseMemoryData);
	unsigned short GenerateMemoryStringMappingKey(void);
	BaseMemoryData *GetMemoryDataByKey(unsigned short key);
	BaseObjectData* GetBaseObjectByObjectID(ObjectID objectId);
	void UnsynchronizeMemoryInt(BaseMemoryData *baseData, bool sendNotificationPacket);
	void OnCreationCommand(SynchronizedObject* newObject, PlayerID playerId, ObjectID objectId);
	void SendMemoryStartRequest(BaseMemoryData *baseData, ParticipantStruct *participant);
	void AddToParticipantList(PlayerID playerId);
	void RemoveFromParticipantList(PlayerID playerId, bool sendDataStopRequests);
	void RemoveAllParticipants(void);
	unsigned GetParticipantIndexByPlayerID(PlayerID playerId);
	ExtendedObjectData* GetExtendedObjectByObjectID(ObjectID objectId, ParticipantStruct *participant);
	BaseMemoryData *GetBaseMemoryDataBySynchronizedMemory(SynchronizedMemory *memory);
	void OnDataReplicateSendMemory(RakPeerInterface *peer, Packet *packet);
	void OnReplicationPushPacket(RakPeerInterface *peer, Packet *packet);
	void OnDataReplicateSendObjectScope(RakPeerInterface *peer, Packet *packet);
	void OnMemoryReplicateStart(RakPeerInterface *peer, Packet *packet);
	void OnDataReplicateStop(RakPeerInterface *peer, Packet *packet);
	void OnNewPlayer(RakPeerInterface *peer, Packet *packet);
	void OnLostPlayer(RakPeerInterface *peer, Packet *packet);
	void OnObjectCreationRequest(RakPeerInterface *peer, Packet *packet);
	void OnObjectCreationRequestResponse(RakPeerInterface *peer, Packet *packet);

	RakPeerInterface *rakPeer;

	// participantList is sorted by PlayerID because we do frequent lookups to make sure packet senders are participants.
	BasicDataStructures::OrderedList<ParticipantStruct*, PlayerID> participantList;
	BasicDataStructures::OrderedList<BaseMemoryData*, unsigned short> memoryList;
	BasicDataStructures::OrderedList<BaseObjectData*, ObjectID> objectList;

	// For objects: This will be undefined at first.  When the remote system sends us an entire object name,
	// We reply with the key to use from then on.
	BasicDataStructures::List<char*> localObjectNameTable;

	bool broadcastToAll;
	static unsigned short memoryToStringMappingKey;
};

// This class provides an interface so RakNet can do object initial serializations and notifications of scope changes.
// It requires that the ObjectReplicationHandler class is also instantiated somewhere and that that class returns the same value
// for the class name as this object
class SynchronizedObject : public NetworkIDGenerator
{
public:
	// Return true if this object is in-scope relevant to another system.
	virtual bool InScope(PlayerID playerId)=0;

	// This is a packet that you get from the sender, that tells you the object you are tracking has changed scope.
	// isNowInScope==true means that the InScope call returns true for your system, while false means it returns false.
	virtual void OnScopeChange(PlayerID sender, bool isNowInScope)=0;

	// Return the name of the class this object represents.  This should match the name returned by ObjectReplicationHandler
	virtual char* GetObjectClassName(void) const=0;

	// Return true to desynchronize an object based on when a remote system desynchronizes an object.  Generally you will return true
	// if the object being desynchronized is owned by that system, and false otherwise
	virtual bool AcceptDesynchronization(PlayerID sender)=0;

	// Called after an object is desynchronized as a result of a remote system.
	virtual bool OnDesynchronization(PlayerID sender)=0;

	// Called after an object is synchronized as the result of a remote system command, such as in a client/server enviroment
	// the server telling us to create an object which we create in the ObjectReplicationHandler.
	virtual void OnSynchronization(PlayerID sender)=0;

	// From NetworkIDGenerator
	virtual bool IsObjectIDAuthority(void) const=0;
	virtual bool IsObjectIDAuthorityActive(void) const=0;
	virtual bool IsObjectIDRecipient(void) const=0;
	virtual bool IsObjectIDRecipientActive(void) const=0;
};

enum SynchronizedMemoryType
{
	SYNCHRONIZED_MEMORY_TYPE_NONE,
	SYNCHRONIZED_MEMORY_TYPE_DEFAULT,
	SYNCHRONIZED_MEMORY_TYPE_COUNT, // I'll keep count here so users can start at that index if you want
};

// The memory synchronizer can synchronize simple data elements by using a simple memcmp and a memcpy
// Data that need to be synchronized more efficiently can derive from and implement SynchronizedMemory
// I suggest you group a set of data elements into a class that implements SynchronizedMemory and selectively write those elements
// using a bitstream since every individual synch incurs a two byte overhead.
class SynchronizedMemory
{
public:
	// Serialize this memory into a bitstream.  lastSendValue and lastSendTime are provided in case you need it.
	virtual void Serialize(RakNet::BitStream *bitstream, SynchronizedMemory *lastSentValue, unsigned int lastSendTime)=0;

	// Update the current data from a bitstream.
	// timePacketSent will not be valid unless IncludeTimestamp returns true
	virtual void Deserialize(RakNet::BitStream *bitstream, unsigned int timePacketSent)=0;

	// Allocate and make a copy of yourself.  Only needs the synchronized data.
	virtual SynchronizedMemory * MakeSynchronizedDataCopy(void)=0;

	// Copy from another instance of this class.  Only needs the synchronized data.
	virtual void CopySynchronizedDataFrom(SynchronizedMemory *source)=0;

	// The copy that RakNet needed is no longer needed.  You probably want to have delete this; in this function.
	virtual void ReleaseCopy(void)=0;

	// Should we send an update to this system?  Return true if the data has changed and we want to send it.  False otherwise
	virtual bool ShouldSendUpdate(PlayerID destinationSystem, SynchronizedMemory *lastSentValue, unsigned int lastSendTime)=0;

	// - Return true if the same value will be sent to all remote systems at the same time.  This is useful for data that is always serialized to everyone
	//   the same way regardless of context, such as the current game score.  If true, the same copy of the last sent data is used for all systems which saves memory and speed.
	// - Return false if different destination systems will have different values sent to them or at different times.
	//   This should be used for contextual data, such as position updates which may not be sent to everyone, or may be sent more or less frequently
	//   This will store an individual copy of the last sent data per remote system
	virtual bool IsUnifiedMemory(void)=0;

	// Return true if you want a valid time value sent to Deserialize.  Only set to true if you need it as bandwidth will be wasted otherwise
	virtual bool IncludeTimestamp(void)=0;

	// Return true if you want this class to be deleted when it is no longer referenced by the data replicator
	virtual bool AutoDeleteOnUnreference(void)=0;

	// For casting.  Overload if you want.  Start at SYNCHRONIZED_MEMORY_TYPE_COUNT
	virtual SynchronizedMemoryType GetType(void) const;
};

//  Use this if you want, but it is mostly for internal use
class SynchronizedMemoryDefault : SynchronizedMemory
{
public:
	~SynchronizedMemoryDefault();
	SynchronizedMemoryDefault(const char *data, int byteLength);
	void Serialize(RakNet::BitStream *bitstream, SynchronizedMemory *lastSentValue, unsigned int lastSendTime);
	void Deserialize(RakNet::BitStream *bitstream, unsigned int timePacketSent);
	SynchronizedMemory * MakeSynchronizedDataCopy(void);
	void CopySynchronizedDataFrom(SynchronizedMemory *source);
	void ReleaseCopy(void);
	bool ShouldSendUpdate(PlayerID destinationSystem, SynchronizedMemory *lastSentValue, unsigned int lastSendTime);
	bool IsUnifiedMemory(void);
	bool IncludeTimestamp(void);
	bool AutoDeleteOnUnreference(void);
	SynchronizedMemoryType GetType(void) const;
	char *GetUserData(void) const;

protected:
	SynchronizedMemoryDefault();

	char *userData;
	int userByteLength;
};


enum ObjectReplicationPushResult
{
	OBJECT_REPLICATION_OK,
	OBJECT_REPLICATION_PUSH_LATER,
	OBJECT_REPLICATION_NEVER_PUSH,
};

#include <string.h>

class ObjectReplicationHandler
{
public:
	// Call RegisterHandler(this); in your derived constructor
	// Call UnregisterHandler(this); in your derived destructor (optional since you never really delete these)

	// Another system has asked that you create an object with name className.  The bitstream passed by the user is included in inContext.
	// Either allocate the object and return it to accept the request or pass 0 to refuse the request.
	// You can write a bitstream to outContext which the remote system will receive with your reply - either in OnCreationRequestAcceptance or OnCreationRequestRejection
	// The remainder of the parameters are used to send the reply packet and the object updates (assuming you create an object).
	virtual SynchronizedObject* OnCreationRequest(PlayerID sender, const char *className, RakNet::BitStream *inContext, RakNet::BitStream *outContext, PacketPriority *outPriority, PacketReliability *outReliability, char *outOrderingChannel, unsigned short *outMaxUpdateFrequencyMS)=0;

	// DataReplicator::RequestObject creation call has been accepted.  You should create the object and return it.
	// inContext contains data passed to the outContext parameter of OnCreationRequest.
	// The object, when created, will automatically be passed to DataReplicator::SerializeObject.
	// You can also return 0 if you decide you don't want the object after all.
	virtual SynchronizedObject* OnCreationRequestAcceptance(PlayerID sender, const char *className, ObjectID newObjectId, RakNet::BitStream *inContext)=0;

	// DataReplicator::RequestObject creation call has been rejected.
	// inContext contains data passed to the outContext parameter of OnCreationRequest.
	virtual void OnCreationRequestRejection(PlayerID sender, const char *className, RakNet::BitStream *inContext)=0;

	// The destination system needs a copy of objectToReplicate.
	// Pass OBJECT_REPLICATION_OK to send a serialized copy of SynchronizedObject to that system immediately.
	// Pass OBJECT_REPLICATION_PUSH_LATER to delay send.  OnReplicationPush will be called again the next Update assuming that the object is still in scope. 
	// Pass OBJECT_REPLICATION_NEVER_PUSH to permanently remove that system as a potential recipient of objectToReplicate
	// You can pass data to the outContext bitstream, which will passed to inContext in OnReplicationPushNotification
	virtual ObjectReplicationPushResult OnReplicationPush(PlayerID destination, const char *className, RakNet::BitStream *outContext, SynchronizedObject *objectToReplicate)=0;

	// The other system wants to send an object to us that we didn't request.
	// Either allocate the object and return it to accept the request or pass 0 to refuse the request.
	// newObjectId contains the object ID that should be assigned to the new object (This will be done automatically if you do not do it yourself).
	// inContext is the data that was passed to OnReplicationPush
	virtual SynchronizedObject* OnReplicationPushNotification(PlayerID sender, const char *className, RakNet::BitStream *inContext, ObjectID newObjectId)=0;

	// Return the name of the class that this handler provides algorithms for.
	// This should match the name returned by SynchronizedObject::GetObjectClassName
	virtual char *GetHandlerClassName(void) const=0;

	virtual bool IsDefaultHandler(void) const;

	static void RegisterHandler(ObjectReplicationHandler *handler);
	static void UnregisterHandler(ObjectReplicationHandler *handler);
	static ObjectReplicationHandler* GetHandler(const char *str);
protected:
	//static BasicDataStructures::OrderedList<ObjectReplicationHandler*, const char*> handlerRegistry;
	//static ObjectReplicationHandlerRegistry instance;
	static ObjectReplicationHandler* handlerRegistryHead;
	ObjectReplicationHandler* next;
	static ObjectReplicationHandler* defaultHandler;
};

// The default handler always creates, sends, and destroys whenever requested.  This is poor security and shouldn't be used except for
// testing and early development.
#define DEFAULT_REPLICATION_HANDLER(CLASSNAME) \
class ObjectReplicationHandler_##CLASSNAME : public ObjectReplicationHandler \
{ \
public: \
	ObjectReplicationHandler_##CLASSNAME() {ObjectReplicationHandler::RegisterHandler(this);} \
	~ObjectReplicationHandler_##CLASSNAME() {ObjectReplicationHandler::UnregisterHandler(this);} \
	SynchronizedObject* OnCreationRequest(PlayerID sender, const char *className, RakNet::BitStream *inContext, RakNet::BitStream *outContext, PacketPriority *outPriority, PacketReliability *outReliability, char *outOrderingChannel, unsigned short *outMaxUpdateFrequencyMS) {return new CLASSNAME ;} \
	SynchronizedObject* OnCreationRequestAcceptance(PlayerID sender, const char *className, ObjectID newObjectId, RakNet::BitStream *inContext) {return new CLASSNAME;} \
	void OnCreationRequestRejection(PlayerID sender, const char *className, RakNet::BitStream *inContext) {} \
	ObjectReplicationPushResult OnReplicationPush(PlayerID destination, const char *className, RakNet::BitStream *outContext, SynchronizedObject *objectToReplicate) {return OBJECT_REPLICATION_OK;} \
	SynchronizedObject* OnReplicationPushNotification(PlayerID sender, const char *className, RakNet::BitStream *inContext, ObjectID newObjectId) {return new CLASSNAME;} \
	char *GetHandlerClassName(void) const {return #CLASSNAME;} \
} static objectReplicationHandlerInstance_##CLASSNAME;

/*
class ObjectReplicationHandlerRegistry
{
public:
	ObjectReplicationHandlerRegistry();
	~ObjectReplicationHandlerRegistry();
	static void RegisterHandler(ObjectReplicationHandler *handler);
	static void UnregisterHandler(ObjectReplicationHandler *handler);
	static ObjectReplicationHandler* GetHandler(const char *str);

protected:
	BasicDataStructures::OrderedList<ObjectReplicationHandler*, const char*> handlerRegistry;
	static ObjectReplicationHandlerRegistry instance;
	static ObjectReplicationHandler* defaultHandler;
};
*/


#endif
