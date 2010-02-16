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

#include "FullyConnectedMesh.h"
#include "RakPeerInterface.h"
#include "PacketEnumerations.h"
#include "BitStream.h"
#include "StringCompressor.h"
#include "GetTime.h"
#include <string.h>
#include <assert.h>

FullyConnectedMesh::FullyConnectedMesh()
{
    pw=0;
#ifdef _DEBUG
	initialized=false;
#endif
}

FullyConnectedMesh::~FullyConnectedMesh()
{
	unsigned i;

	if (pw)
		delete [] pw;

	for (i=0; i < meshPeerList.size(); i++)
		delete meshPeerList[i];
	meshPeerList.clear();
}

void FullyConnectedMesh::Initialize(const char *password)
{
#ifdef _DEBUG
	assert(initialized==false);
#endif

	if (password && password[0])
	{
		assert(strlen(password)<256);
		pw=new char [strlen(password)+1];
		strcpy(pw, password);
	}
}

#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void FullyConnectedMesh::OnDisconnect(RakPeerInterface *peer)
{
	unsigned i;
	for (i=0; i < meshPeerList.size(); i++)
		delete meshPeerList[i];
	meshPeerList.clear();
}

void FullyConnectedMesh::OnUpdate(RakPeerInterface *peer)
{
	unsigned i;
	bool gotTime;
	unsigned time;
	// Clear out unresponsive connections
	i=0;
	gotTime=false;
	while (i < meshPeerList.size())
	{
		if (meshPeerList[i]->connectionState==FCM_RemotePeer::CONNECTING)
		{
			if (gotTime==false)
			{
				// GetTime is slow so don't call it more than once
				time = RakNet::GetTime();
				gotTime=true;
			}
#pragma warning( disable : 4701 ) // warning C4701: local variable 'time' may be used without having been initialized
			if (time-meshPeerList[i]->timer > 10000 )
			{

				delete meshPeerList[i];
				meshPeerList[i]=meshPeerList[meshPeerList.size()-1];
				meshPeerList.del();			
			}
			else
				i++;
		}
		else
			i++;
	}
}

bool FullyConnectedMesh::OnReceive(RakPeerInterface *peer, Packet *packet)
{
	assert(packet);
	assert(peer);

	switch (packet->data[0])
	{
	case ID_CONNECTION_REQUEST_ACCEPTED:
		HandleConnectionRequestAccepted(peer, packet);
		break;
	case ID_CONNECTION_LOST:
	case ID_DISCONNECTION_NOTIFICATION:
	case ID_CONNECTION_ATTEMPT_FAILED:
		HandleDroppedConnection(peer, packet);
		break;
	case ID_FULLY_CONNECTED_MESH_JOIN_REQUEST:
		HandleMeshJoinRequest(peer, packet);
		break;
	case ID_FULLY_CONNECTED_MESH_JOIN_RESPONSE:
		HandleMeshJoinResponse(peer, packet);
		break;
	default:
		// type not used by FullyConnectedMesh
		return false;
	}

	// If we do not propagate this kind of packet to the game, return true to signal that this class absorbed the packet
	return PropagateToGame(packet)==false;
}

bool FullyConnectedMesh::PropagateToGame(Packet *packet) const
{
	unsigned char packetId = packet->data[0];
	return packetId!=ID_FULLY_CONNECTED_MESH_JOIN_REQUEST &&
		packetId!=ID_FULLY_CONNECTED_MESH_JOIN_RESPONSE;
}

void FullyConnectedMesh::HandleConnectionRequestAccepted(RakPeerInterface *peer, Packet *packet)
{
	assert(packet);
	assert(peer);

	// Send a join mesh request
	RakNet::BitStream bitStream;
	bitStream.Write((unsigned char) ID_FULLY_CONNECTED_MESH_JOIN_REQUEST);
	AppendMeshJoinAuthorization(&bitStream);

	MarkAllRemotelyKnown(false);
	AddUniquePeer(peer, FCM_RemotePeer::CONNECTING, packet->playerId, true);

	// Write every system we know about
	SerializeRemotelyUnknownPeers(&bitStream);
	peer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);
}

void FullyConnectedMesh::HandleMeshJoinRequest(RakPeerInterface *peer, Packet *packet)
{
	assert(packet);
	assert(peer);

	RakNet::BitStream bitStream((char*)packet->data, packet->length, false);
	bitStream.IgnoreBits(8);
    if (!ValidateMeshJoinAuthorization(&bitStream))
	{
		OnJoinMeshAuthorizationFailed(peer,packet);
		return;
	}

	// Mark all current systems as not known to the sender
	MarkAllRemotelyKnown(false);
	AddUniquePeer(peer, FCM_RemotePeer::CONNECTED, packet->playerId, true);

	// Deserialize the peer list from the sender, which will mark those peers as remotelyKnown==true
	// Newly added peers will have connectionState==FCM_RemotePeer::NEWLY_LISTED
	if (!DeserializeMeshPeerList(peer, &bitStream))
	{
		OnDataUnderflow(peer,packet);
		return;
	}

	if (bitStream.GetNumberOfUnreadBits()>7)
		OnDataOverflow(peer,packet);

	// Connect to every new server marked with FCM_RemotePeer::NEWLY_LISTED.
	// Also mark all FCM_RemotePeer::NEWLY_LISTED as CONNECTING
	ConnectToNewlyListedServers(peer);

	RakNet::BitStream outputBitStream;
	// Reply to the sender with all the servers it did not know about
	outputBitStream.Write((unsigned char) ID_FULLY_CONNECTED_MESH_JOIN_RESPONSE);
	AppendMeshJoinAuthorization(&outputBitStream);
	SerializeRemotelyUnknownPeers(&outputBitStream);
	peer->Send(&outputBitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);
}


#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void FullyConnectedMesh::AddUniquePeer(RakPeerInterface *peer, FCM_RemotePeer::FCM_RemotePeerState state, PlayerID playerId, bool remotelyKnown)
{
	int remoteIndex;
	remoteIndex=MeshPeerIndexFromPlayerID(playerId);
	if (remoteIndex!=-1)
	{
		meshPeerList[remoteIndex]->connectionState=state;
		meshPeerList[remoteIndex]->remotelyKnown=remotelyKnown;
	}
	else
	{
		// Add the sender as a validated remote peer to the list of peers
		FCM_RemotePeer *fcm = new FCM_RemotePeer;
		fcm->connectionState=state;
		fcm->playerId=playerId;
		fcm->remotelyKnown=remotelyKnown;
		fcm->timer=RakNet::GetTime();
		meshPeerList.insert(fcm);
	}
}

void FullyConnectedMesh::HandleMeshJoinResponse(RakPeerInterface *peer, Packet *packet)
{
	assert(packet);
	assert(peer);

	int remoteIndex;

	RakNet::BitStream bitStream((char*)packet->data, packet->length, false);
	bitStream.IgnoreBits(8);
	if (!ValidateMeshJoinAuthorization(&bitStream))
	{
		OnJoinMeshAuthorizationFailed(peer,packet);
		return;
	}

	// Find the caller and mark it as verified
	remoteIndex=MeshPeerIndexFromPlayerID(packet->playerId);
	if (remoteIndex==-1)
	{
#ifdef _DEBUG
		assert(remoteIndex!=-1);
#endif
		OnInvalidPacket(peer, packet);
		return;
	}
	meshPeerList[remoteIndex]->connectionState=FCM_RemotePeer::CONNECTED;

	// Mark all current systems as not known to the sender
	MarkAllRemotelyKnown(false);

	// Newly added peers will have connectionState==FCM_RemotePeer::NEWLY_LISTED
	if (!DeserializeMeshPeerList(peer, &bitStream))
	{
		OnDataUnderflow(peer,packet);
		return;
	}

	if (bitStream.GetNumberOfUnreadBits()>7)
		OnDataOverflow(peer,packet);

	// Connect to every new server marked with FCM_RemotePeer::NEWLY_LISTED.
	// Also mark all FCM_RemotePeer::NEWLY_LISTED as CONNECTING
	ConnectToNewlyListedServers(peer);
}

void FullyConnectedMesh::HandleDroppedConnection(RakPeerInterface *peer, Packet *packet)
{
	assert(packet);
	assert(peer);

	DeleteFromPeerList(packet->playerId);
}

void FullyConnectedMesh::AppendMeshJoinAuthorization(RakNet::BitStream *bitStream)
{
	// Plaintext encoding of the password.  If you use a password, use secure connections
	StringCompressor::Instance()->EncodeString(pw, 256, bitStream);
}

bool FullyConnectedMesh::ValidateMeshJoinAuthorization(RakNet::BitStream *bitStream)
{
	if (!pw || pw[0]==0)
		return true;

	char str[256];
	if (!StringCompressor::Instance()->DecodeString(str, 256, bitStream))
		return false;

	if (strcmp(pw, str))
		return false;

	return true;
}


#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void FullyConnectedMesh::OnDataOverflow(RakPeerInterface *peer, Packet *packet)
{
	// Derived class should implement this if it cares about it
#ifdef _DEBUG
	// Should never hit this
	assert(0);
#endif
}

#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void FullyConnectedMesh::OnDataUnderflow(RakPeerInterface *peer, Packet *packet)
{
	// Derived class should implement this if it cares about it
#ifdef _DEBUG
	// Should never hit this
	assert(0);
#endif
}

#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void FullyConnectedMesh::OnInvalidPacket(RakPeerInterface *peer, Packet *packet)
{
	// Packet data that just doesn't make sense
#ifdef _DEBUG
	// Should never hit this
	assert(0);
#endif
}

#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void FullyConnectedMesh::OnJoinMeshAuthorizationFailed(RakPeerInterface *peer, Packet *packet)
{
	// Derived class should implement this if it cares about it
#ifdef _DEBUG
	// Should never hit this
	assert(0);
#endif
}

void FullyConnectedMesh::SerializeRemotelyUnknownPeers(RakNet::BitStream *bitStream)
{
	unsigned short count;
	unsigned i;
	count=0;

	for (i=0; i < meshPeerList.size(); i++)
		if (meshPeerList[i]->remotelyKnown==false)
			count++;

	bitStream->WriteCompressed(count);
	for (i=0; i < meshPeerList.size(); i++)
	{
		if (meshPeerList[i]->remotelyKnown==false)
		{
			bitStream->Write(meshPeerList[i]->playerId.binaryAddress);
			bitStream->Write(meshPeerList[i]->playerId.port);
		}
	}
}

bool FullyConnectedMesh::DeserializeMeshPeerList(RakPeerInterface *peer, RakNet::BitStream *bitStream)
{
	unsigned short count;
	int i, foundIndex;
	PlayerID playerId;
	count=(unsigned short)meshPeerList.size();
	if (!bitStream->ReadCompressed(count))
		return false;

	for (i=0; i < count; i++)
	{
		bitStream->Read(playerId.binaryAddress);
		if (!bitStream->Read(playerId.port))
			return false;

		foundIndex=MeshPeerIndexFromPlayerID(playerId);

		if (foundIndex!=-1)
			meshPeerList[foundIndex]->remotelyKnown=true;
		else
		{
			AddUniquePeer(peer, FCM_RemotePeer::NEWLY_LISTED, playerId, true);
		}
	}
	return true;
}

void FullyConnectedMesh::DeleteFromPeerList(PlayerID playerId)
{
	unsigned i;
	for (i=0; i < meshPeerList.size(); i++)
	{
		if (meshPeerList[i]->playerId==playerId)
		{
			// Copy to middle of list from end and delete end of list
			delete meshPeerList[i];
			meshPeerList[i]=meshPeerList[meshPeerList.size()-1];
			meshPeerList.del();
		}
	}
}

void FullyConnectedMesh::MarkAllRemotelyKnown(bool value)
{
	unsigned i;
	for (i=0; i < meshPeerList.size(); i++)
		meshPeerList[i]->remotelyKnown=value;
}

int FullyConnectedMesh::MeshPeerIndexFromPlayerID(PlayerID playerId)
{
	unsigned i;
	for (i=0; i < meshPeerList.size(); i++)
		if (meshPeerList[i]->playerId==playerId)
			return (int)i;
	return -1;
}

void FullyConnectedMesh::ConnectToNewlyListedServers(RakPeerInterface *peer)
{
	unsigned time = RakNet::GetTime();
	unsigned i;
	for (i=0; i < meshPeerList.size(); i++)
	{
		if (meshPeerList[i]->connectionState==FCM_RemotePeer::NEWLY_LISTED)
		{
			 // This assumes individual servers are not using passwords on top of the mesh password.  Modify this code if they do
			peer->Connect((char*)peer->PlayerIDToDottedIP(meshPeerList[i]->playerId), meshPeerList[i]->playerId.port, 0, 0);
			meshPeerList[i]->connectionState=FCM_RemotePeer::CONNECTING;
			meshPeerList[i]->timer=time;
		}
	}
}
unsigned FullyConnectedMesh::GetMeshPeerListSize(void) const
{
	return meshPeerList.size();
}

PlayerID FullyConnectedMesh::GetPeerIDAtIndex(unsigned index)
{
	if (index < meshPeerList.size())
		return meshPeerList[index]->playerId;
	else
		return UNASSIGNED_PLAYER_ID;
}

