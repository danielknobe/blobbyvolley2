/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Helps automate connections for mesh topologies
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

#ifndef __FULLY_CONNECTED_MESH_H
#define __FULLY_CONNECTED_MESH_H

class RakPeerInterface;
#include "NetworkTypes.h"
#include "ArrayList.h"
#include "MessageHandlerInterface.h"
#include "BitStream.h"

// Stores data for each peer in the mesh
struct FCM_RemotePeer
{
	PlayerID playerId;
	enum FCM_RemotePeerState
	{
		NEWLY_LISTED,
		CONNECTING,
		CONNECTED,
	} connectionState;

	bool remotelyKnown; // When a remote peer sends us a list of peers this is used to track which peers we know about and which they know about

	unsigned timer; // Used for timed operations, such as removing dead entries from a list when connections do not succeed
};

class FullyConnectedMesh : public MessageHandlerInterface
{
public:
	FullyConnectedMesh();
	~FullyConnectedMesh();

	// --------------------------------------------------------------------------------------------
	// User functions
	// --------------------------------------------------------------------------------------------
	// Plaintext encoding of the password.  If you use a password, use secure connections
	void Initialize(const char *password);

	// --------------------------------------------------------------------------------------------
	// Packet handling functions
	// --------------------------------------------------------------------------------------------
	virtual void OnDisconnect(RakPeerInterface *peer);
	virtual void OnUpdate(RakPeerInterface *peer);
	virtual bool OnReceive(RakPeerInterface *peer, Packet *packet);
	virtual bool PropagateToGame(Packet *packet) const;
	
	// --------------------------------------------------------------------------------------------
	// Overridable event callbacks
	// --------------------------------------------------------------------------------------------
	// Too much data was in a packet.  User should override in a derived class to handle this
	virtual void OnDataOverflow(RakPeerInterface *peer, Packet *packet);
	// Not enough data was in a packet.  User should override in a derived class to handle this
	virtual void OnDataUnderflow(RakPeerInterface *peer, Packet *packet);
	// Packet data that just doesn't make sense
	virtual void OnInvalidPacket(RakPeerInterface *peer, Packet *packet);
	// Wrong password
	virtual void OnJoinMeshAuthorizationFailed(RakPeerInterface *peer, Packet *packet);

	unsigned GetMeshPeerListSize(void) const;
	PlayerID GetPeerIDAtIndex(unsigned index);
protected:
	// ID_CONNECTION_REQUEST_ACCEPTED
	void HandleConnectionRequestAccepted(RakPeerInterface *peer, Packet *packet);
	// ID_CONNECTION_LOST or ID_DISCONNECTION_NOTIFICATION.  Host may be lost
	void HandleDroppedConnection(RakPeerInterface *peer, Packet *packet);
	void HandleMeshJoinRequest(RakPeerInterface *peer, Packet *packet);
	void HandleMeshJoinResponse(RakPeerInterface *peer, Packet *packet);

	void AddUniquePeer(RakPeerInterface *peer, FCM_RemotePeer::FCM_RemotePeerState state, PlayerID playerId, bool remotelyKnown);
	
	virtual void AppendMeshJoinAuthorization(RakNet::BitStream *bitStream);
	virtual bool ValidateMeshJoinAuthorization(RakNet::BitStream *bitStream);
	void SerializeRemotelyUnknownPeers(RakNet::BitStream *bitStream);
	bool DeserializeMeshPeerList(RakPeerInterface *peer, RakNet::BitStream *bitStream);
	void DeleteFromPeerList(PlayerID playerId);
	void MarkAllRemotelyKnown(bool value);
	int MeshPeerIndexFromPlayerID(PlayerID playerId);
	void ConnectToNewlyListedServers(RakPeerInterface *peer);

	char *pw;
	BasicDataStructures::List<FCM_RemotePeer*> meshPeerList;

#ifdef _DEBUG
	bool initialized;
#endif
};

#endif

