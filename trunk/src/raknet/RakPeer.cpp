/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief RakPeer Implementation
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
#include "RakPeer.h"
#include "NetworkTypes.h"

#ifdef _WIN32
#include <process.h>
#else
#define closesocket close
#include <unistd.h>
#include <pthread.h>
#endif
#include <ctype.h> // toupper

#include "GetTime.h"
#include "PacketEnumerations.h"
#include "PacketPool.h"

// alloca
#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <cstring>

// On a Little-endian machine the RSA key and message are mangled, but we're
// trying to be friendly to the little endians, so we do byte order
// mangling on Big-Endian machines.  Note that this mangling is independent
// of the byte order used on the network (which also defaults to little-end).
#ifdef HOST_ENDIAN_IS_BIG
	void __inline BSWAPCPY(unsigned char *dest, unsigned char *source, int bytesize)
	{
	#ifdef _DEBUG
		assert( (bytesize % 4 == 0)&&(bytesize)&& "Something is wrong with your exponent or modulus size.");
	#endif
		int i;
		for (i=0; i<bytesize; i+=4)
		{
			dest[i] = source[i+3];
			dest[i+1] = source[i+2];
			dest[i+2] = source[i+1];
			dest[i+3] = source[i];
		}
	}
	void __inline BSWAPSELF(unsigned char *source, int bytesize)
	{
	#ifdef _DEBUG
		assert( (bytesize % 4 == 0)&&(bytesize)&& "Something is wrong with your exponent or modulus size.");
	#endif
		int i;
		unsigned char a, b;
		for (i=0; i<bytesize; i+=4)
		{
			a = source[i];
			b = source[i+1];
			source[i] = source[i+3];
			source[i+1] = source[i+2];
			source[i+2] = b;
			source[i+3] = a;
		}
	}
#endif

static const unsigned int SYN_COOKIE_OLD_RANDOM_NUMBER_DURATION = 5000;
static const int MAX_OFFLINE_DATA_LENGTH=400; // I set this because I limit ID_CONNECTION_REQUEST to 512 bytes, and the password is appended to that packet.

//#define _DO_PRINTF

// UPDATE_THREAD_POLL_TIME is how often the update thread will poll to see
// if receive wasn't called within UPDATE_THREAD_UPDATE_TIME.  If it wasn't called within that time,
// the updating thread will activate and take over network communication until Receive is called again.
//static const unsigned int UPDATE_THREAD_UPDATE_TIME=30;
//static const unsigned int UPDATE_THREAD_POLL_TIME=30;


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RakPeer()
{
	connectionSocket = INVALID_SOCKET;
	MTUSize = DEFAULT_MTU_SIZE;
	maximumIncomingConnections = 0;
	maximumNumberOfPeers = 0;
	remoteSystemListSize=0;
	remoteSystemList = 0;
	bytesSentPerSecond = bytesReceivedPerSecond = 0;
	endThreads = true;
	isMainLoopThreadActive = false;
	connectionSocket = INVALID_SOCKET;
	myPlayerId = UNASSIGNED_PLAYER_ID;
	allowConnectionResponseIPMigration = false;
	incomingPacketQueue.clearAndForceAllocation(128);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Destructor
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::~RakPeer()
{
	Disconnect( 0 );
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Starts the network threads, opens the listen port
// You must call this before calling SetMaximumIncomingConnections or Connect
// Multiple calls while already active are ignored.  To call this function again with different settings, you must first call Disconnect()
//
// Parameters:
// MaximumNumberOfPeers:  Required so the network can preallocate and for thread safety.
// - A pure client would set this to 1.  A pure server would set it to the number of allowed clients.
// - A hybrid would set it to the sum of both types of connections
// localPort: The port to listen for connections on.
// _threadSleepTimer: >=0 for how many ms to Sleep each internal update cycle (recommended 30 for low performance, 0 for regular)
 // forceHostAddress Can force RakNet to use a particular IP to host on.  Pass 0 to automatically pick an IP
// Returns:
// False on failure (can't create socket or thread), true on success.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::Initialize( unsigned short MaximumNumberOfPeers, unsigned short localPort, int _threadSleepTimer, const char *forceHostAddress )
{
	// Maximum number of peers must be > 0
	assert(MaximumNumberOfPeers > 0);

	if (MaximumNumberOfPeers <= 0)
	{
		return false;
	}

	if ( connectionSocket == INVALID_SOCKET )
	{
		connectionSocket = SocketLayer::Instance()->CreateBoundSocket( localPort, true, forceHostAddress );

		if ( connectionSocket == INVALID_SOCKET )
			return false;
	}

	if ( _threadSleepTimer < 0 )
		return false;

	if ( maximumNumberOfPeers == 0 )
	{
		// Don't allow more incoming connections than we have peers.
		if ( maximumIncomingConnections > MaximumNumberOfPeers )
			maximumIncomingConnections = MaximumNumberOfPeers;

		maximumNumberOfPeers = MaximumNumberOfPeers;
		// Allocate a few extra remote systems to handle new connections from players trying to connect when the server is full
		remoteSystemListSize = MaximumNumberOfPeers + 1 + MaximumNumberOfPeers/8;

	//	rakPeerMutexes[ RakPeer::remoteSystemList_Mutex ].Lock();
		remoteSystemList = new RemoteSystemStruct[ remoteSystemListSize ];
	//	rakPeerMutexes[ RakPeer::remoteSystemList_Mutex ].Unlock();
		for ( unsigned i = 0; i < remoteSystemListSize; i++ )
		{
			remoteSystemList[ i ].playerId = UNASSIGNED_PLAYER_ID;
	//		remoteSystemList[ i ].allowPlayerIdAssigment=true;
		}
	}

	// For histogram statistics
	// nextReadBytesTime=0;
	// lastSentBytes=lastReceivedBytes=0;

	if ( endThreads )
	{
		lastUserUpdateCycle = 0;

		updateCycleIsRunning = false;
		endThreads = false;
		// Create the threads
		threadSleepTimer = _threadSleepTimer;

		ClearBufferedCommands();

		char ipList[ 10 ][ 16 ];
		SocketLayer::Instance()->GetMyIP( ipList );
		myPlayerId.port = localPort;
		myPlayerId.binaryAddress = inet_addr( ipList[ 0 ] );
		{
#ifdef _WIN32

			if ( isMainLoopThreadActive == false )
			{
				unsigned ProcessPacketsThreadID = 0;
				processPacketsThreadHandle = ( HANDLE ) _beginthreadex( NULL, 0, UpdateNetworkLoop, this, 0, &ProcessPacketsThreadID );

				if ( processPacketsThreadHandle == 0 )
				{
					Disconnect( 0 );
					return false;
				}

				// SetThreadPriority(processPacketsThreadHandle, THREAD_PRIORITY_HIGHEST);

				CloseHandle( processPacketsThreadHandle );

				processPacketsThreadHandle = 0;

			}

#else
			pthread_attr_t attr;
			pthread_attr_init( &attr );
			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

			//  sched_param sp;
			//  sp.sched_priority = sched_get_priority_max(SCHED_OTHER);
			//  pthread_attr_setschedparam(&attr, &sp);

			if ( isMainLoopThreadActive == false )
			{
				int error = pthread_create( &processPacketsThreadHandle, &attr, &UpdateNetworkLoop, this );

				if ( error )
				{
					Disconnect( 0 );
					return false;
				}
			}

			processPacketsThreadHandle = 0;
#endif


			// Wait for the threads to activate.  When they are active they will set these variables to true

			while (  /*isRecvfromThreadActive==false || */isMainLoopThreadActive == false )
#ifdef _WIN32
				Sleep( 10 );
#else
				usleep( 10 * 1000 );
#endif

		}

		/* else
		{
		#ifdef __USE_IO_COMPLETION_PORTS
		AsynchronousFileIO::Instance()->IncreaseUserCount();
		#endif
		}*/
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Sets how many incoming connections are allowed.  If this is less than the number of players currently connected, no
// more players will be allowed to connect.  If this is greater than the maximum number of peers allowed, it will be reduced
// to the maximum number of peers allowed.  Defaults to 0.
//
// Parameters:
// numberAllowed - Maximum number of incoming connections allowed.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetMaximumIncomingConnections( unsigned short numberAllowed )
{
	maximumIncomingConnections = numberAllowed;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the maximum number of incoming connections, which is always <= MaximumNumberOfPeers
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned short RakPeer::GetMaximumIncomingConnections( void ) const
{
	return maximumIncomingConnections;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Call this to connect to the specified host (ip or domain name) and server port.
// Calling Connect and not calling SetMaximumIncomingConnections acts as a dedicated client.  Calling both acts as a true peer.
// This is a non-blocking connection.  You know the connection is successful when IsConnected() returns true
// or receive gets a packet with the type identifier ID_CONNECTION_ACCEPTED.  If the connection is not
// successful, such as rejected connection or no response then neither of these things will happen.
// Requires that you first call Initialize
//
// Parameters:
// host: Either a dotted IP address or a domain name
// remotePort: Which port to connect to on the remote machine.
// passwordData: A data block that must match the data block on the server.  This can be just a password, or can be a stream of data
// passwordDataLength: The length in bytes of passwordData
//
// Returns:
// True on successful initiation. False on incorrect parameters, internal error, or too many existing peers
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::Connect( const char* host, unsigned short remotePort )
{
	// If endThreads is true here you didn't call Initialize() first.
	if ( host == 0 || endThreads || connectionSocket == INVALID_SOCKET )
		return false;

	// If the host starts with something other than 0, 1, or 2 it's (probably) a domain name.
	if ( host[ 0 ] < '0' || host[ 0 ] > '2' )
	{
		host = ( char* ) SocketLayer::Instance()->DomainNameToIP( host );

		if (host==0)
			return false;
	}

	// Connecting to ourselves in the same instance of the program?
	if ( ( strcmp( host, "127.0.0.1" ) == 0 || strcmp( host, "0.0.0.0" ) == 0 ) && remotePort == myPlayerId.port )
	{
		// Feedback loop.
		if ( !AllowIncomingConnections() )
		{
			// Tell the game that this person has connected
			Packet * p;
			p = packetPool.GetPointer();

			p->data = new unsigned char [ 1 ];
			p->data[ 0 ] = (unsigned char) ID_NO_FREE_INCOMING_CONNECTIONS;
			p->playerId = myPlayerId;
			p->playerIndex = ( PlayerIndex ) GetIndexFromPlayerID( myPlayerId );
			p->length = 1;
			p->bitSize = 8;

#ifdef _DEBUG

			assert( p->data );
#endif

			incomingQueueMutex.Lock();
			incomingPacketQueue.push( p );
			incomingQueueMutex.Unlock();
		}
		else
		{
			// Just assume we are connected.  This is really just for testing.
		//	RemoteSystemStruct* remoteSystem = AssignPlayerIDToRemoteSystemList( myPlayerId, RemoteSystemStruct::CONNECTED );

		//	if ( remoteSystem != 0 )
		//	{
				RakNet::BitStream bitStream(sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned short));
				bitStream.Write((unsigned char)ID_NEW_INCOMING_CONNECTION);
				bitStream.Write(myPlayerId.binaryAddress);
				bitStream.Write(myPlayerId.port);
		//		Send( &bitStream, SYSTEM_PRIORITY, RELIABLE, 0, myPlayerId, false );
				SendBuffered(&bitStream, SYSTEM_PRIORITY, RELIABLE, 0, myPlayerId, false, RemoteSystemStruct::CONNECTED);
				return true;
		//	}
		//	else
		//		return false;
		}
	}

	return SendConnectionRequest( host, remotePort );
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Stops the network threads and close all connections.  Multiple calls are ok.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::Disconnect( unsigned int blockDuration )
{
	unsigned i;
	unsigned short systemListSize = remoteSystemListSize; // This is done for threading reasons

	if ( blockDuration > 0 )
	{
		for ( i = 0; i < systemListSize; i++ )
		{
			NotifyAndFlagForDisconnect(remoteSystemList[i].playerId, false);
		}

		unsigned time = RakNet::GetTime();
		unsigned stopWaitingTime = time + blockDuration;
		while ( time < stopWaitingTime )
		{
			bool anyActive = false;
			for (unsigned j = 0; j < systemListSize; j++)
			{
				if (remoteSystemList[j].playerId!=UNASSIGNED_PLAYER_ID)
				{
					anyActive=true;
					break;
				}
			}

			// If this system is out of packets to send, then stop waiting
			if ( anyActive==false )
				break;

			// This will probably cause the update thread to run which will probably
			// send the disconnection notification
#ifdef _WIN32
			Sleep( 15 );
#else
			usleep( 15 * 1000 );
#endif
			time = RakNet::GetTime();
		}
	}

	if ( endThreads == false )
	{
		// Stop the threads
		endThreads = true;

		// Normally the thread will call DecreaseUserCount on termination but if we aren't using threads just do it
		// manually
#ifdef __USE_IO_COMPLETION_PORTS
		AsynchronousFileIO::Instance()->DecreaseUserCount();
#endif
	}

	while ( isMainLoopThreadActive )
#ifdef _WIN32
		Sleep( 15 );
#else
		usleep( 15 * 1000 );
#endif

	// Reset the remote system list after the threads are known to have stopped so threads do not add or update data to them after they are reset
	//rakPeerMutexes[ RakPeer::remoteSystemList_Mutex ].Lock();
	for ( i = 0; i < systemListSize; i++ )
	{
		// Reserve this reliability layer for ourselves
		remoteSystemList[ i ].playerId = UNASSIGNED_PLAYER_ID;

		// Remove any remaining packets
		remoteSystemList[ i ].reliabilityLayer.Reset();
	}
	//rakPeerMutexes[ remoteSystemList_Mutex ].Unlock();

	// Setting maximumNumberOfPeers to 0 allows remoteSystemList to be reallocated in Initialize.
	// Setting remoteSystemListSize prevents threads from accessing the reliability layer
	maximumNumberOfPeers = 0;
	remoteSystemListSize = 0;

	if ( connectionSocket != INVALID_SOCKET )
	{
		closesocket( connectionSocket );
		connectionSocket = INVALID_SOCKET;
	}

	// Clear out the queues
	while ( incomingPacketQueue.size() )
		packetPool.ReleasePointer( incomingPacketQueue.pop() );

	ClearBufferedCommands();
	bytesSentPerSecond = bytesReceivedPerSecond = 0;

	ClearRequestedConnectionList();


	// Clear out the reliabilty layer list in case we want to reallocate it in a successive call to Init.
	RemoteSystemStruct * temp = remoteSystemList;
	remoteSystemList = 0;
	delete [] temp;

	packetPool.ClearPool();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns true if the network threads are running
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::IsActive( void ) const
{
	return endThreads == false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Fills the array remoteSystems with the playerID of all the systems we are connected to
//
// Parameters:
// remoteSystems (out): An array of PlayerID structures to be filled with the PlayerIDs of the systems we are connected to
// - pass 0 to remoteSystems to only get the number of systems we are connected to
// numberOfSystems (int, out): As input, the size of remoteSystems array.  As output, the number of elements put into the array
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::GetConnectionList( PlayerID *remoteSystems, unsigned short *numberOfSystems ) const
{
	int count, index;
	count=0;

	if ( remoteSystemList == 0 || endThreads == true )
	{
		*numberOfSystems = 0;
		return false;
	}

	// This is called a lot so I unrolled the loop
	if ( remoteSystems )
	{
		for ( count = 0, index = 0; index < remoteSystemListSize; ++index )
			if ( remoteSystemList[ index ].playerId != UNASSIGNED_PLAYER_ID && remoteSystemList[ index ].connectMode==RemoteSystemStruct::CONNECTED)
			{
				if ( count < *numberOfSystems )
					remoteSystems[ count ] = remoteSystemList[ index ].playerId;

				++count;
			}
	}
	else
	{
		for ( count = 0, index = 0; index < remoteSystemListSize; ++index )
			if ( remoteSystemList[ index ].playerId != UNASSIGNED_PLAYER_ID && remoteSystemList[ index ].connectMode==RemoteSystemStruct::CONNECTED)
				++count;
	}

	*numberOfSystems = ( unsigned short ) count;

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Sends a block of data to the specified system that you are connected to.
// This function only works while the client is connected (Use the Connect function).
//
// Parameters:
// data: The block of data to send
// length: The size in bytes of the data to send
// bitStream: The bitstream to send
// priority: What priority level to send on.
// reliability: How reliability to send this data
// orderingChannel: When using ordered or sequenced packets, what channel to order these on.
// - Packets are only ordered relative to other packets on the same stream
// playerId: Who to send this packet to, or in the case of broadcasting who not to send it to. Use UNASSIGNED_PLAYER_ID to specify none
// broadcast: True to send this packet to all connected systems.  If true, then playerId specifies who not to send the packet to.
// Returns:
// False if we are not connected to the specified recipient.  True otherwise
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::Send( const char *data, const long length, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast )
{
#ifdef _DEBUG
	assert( data && length > 0 );
#endif

	if ( data == 0 || length < 0 )
		return false;

	RakNet::BitStream temp( (char*)data, length, false );
	return Send( &temp, priority, reliability, orderingChannel, playerId, broadcast );

}

bool RakPeer::Send( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast )
{
#ifdef _DEBUG
	assert( bitStream->GetNumberOfBytesUsed() > 0 );
#endif

	if ( bitStream->GetNumberOfBytesUsed() == 0 )
		return false;

	if ( remoteSystemList == 0 || endThreads == true )
		return false;

	if ( broadcast == false && playerId == UNASSIGNED_PLAYER_ID )
		return false;

	if (ValidSendTarget(playerId, broadcast))
	{
		// Sends need to be buffered and processed in the update thread because the playerID associated with the reliability layer can change,
		// from that thread, resulting in a send to the wrong player!  While I could mutex the playerID, that is much slower than doing this
		SendBuffered(bitStream, priority, reliability, orderingChannel, playerId, broadcast, RemoteSystemStruct::NO_ACTION);
		return true;
	}

	return false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Gets a packet from the incoming packet queue. Use DeallocatePacket to deallocate the packet after you are done with it.
// Check the Packet struct at the top of CoreNetworkStructures.h for the format of the struct
//
// Returns:
// 0 if no packets are waiting to be handled, otherwise an allocated packet
// If the client is not active this will also return 0, as all waiting packets are flushed when the client is Disconnected
// This also updates all memory blocks associated with synchronized memory and distributed objects
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
packet_ptr RakPeer::Receive( void )
{
	if ( !( IsActive() ) )
		return packet_ptr();

	Packet *val;

	#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
	while ( true )
	{
		if (incomingPacketQueue.size() > 0) // First fast unlocked check
		{
			incomingQueueMutex.Lock();
			if ( incomingPacketQueue.size() > 0 ) // Second safe locked check
			{
				val = incomingPacketQueue.pop();
			}

			else
			{
				incomingQueueMutex.Unlock();
				return packet_ptr();
			}
			incomingQueueMutex.Unlock();
		}
		else
			return packet_ptr();

		break;
	}


#ifdef _DEBUG
	assert( val->data );
#endif

	struct deleter
	{
		RakPeer* peer;
		void operator()(Packet* p)
		{
			if ( p == 0 )
				return ;

			peer->packetPool.ReleasePointer( p );
		}
	};

	deleter del;
	del.peer = this;
	return packet_ptr(val, del);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Return the total number of connections we are allowed
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned short RakPeer::GetMaximumNumberOfPeers( void ) const
{
	return maximumNumberOfPeers;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Close the connection to another host (if we initiated the connection it will disconnect, if they did it will kick them out).
//
// Parameters:
// target: Which connection to close
// sendDisconnectionNotification: True to send ID_DISCONNECTION_NOTIFICATION to the recipient. False to close it silently.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::CloseConnection( PlayerID target, bool sendDisconnectionNotification )
{
	CloseConnectionInternalBuffered(target, sendDisconnectionNotification);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Given a playerID, returns an index from 0 to the maximum number of players allowed - 1.
//
// Parameters
// playerId - The playerID to search for
//
// Returns
// An integer from 0 to the maximum number of peers -1, or -1 if that player is not found
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetIndexFromPlayerID( PlayerID playerId )
{
	unsigned i;

	if ( playerId == UNASSIGNED_PLAYER_ID )
		return -1;

	for ( i = 0; i < maximumNumberOfPeers; i++ )
		if ( remoteSystemList[ i ].playerId == playerId )
			return i;

	return -1;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// This function is only useful for looping through all players.
//
// Parameters
// index - an integer between 0 and the maximum number of players allowed - 1.
//
// Returns
// A valid playerID or UNASSIGNED_PLAYER_ID if no such player at that index
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
PlayerID RakPeer::GetPlayerIDFromIndex( int index )
{
	if ( index >= 0 && index < remoteSystemListSize )
		if (remoteSystemList[ index ].connectMode==RakPeer::RemoteSystemStruct::CONNECTED) // Don't give the user players that aren't fully connected, since sends will fail
			return remoteSystemList[ index ].playerId;

	return UNASSIGNED_PLAYER_ID;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Send a ping to the specified connected system.
//
// Parameters:
// target - who to ping
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::Ping( PlayerID target )
{
	PingInternal(target, false);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Send a ping to the specified unconnected system.
// The remote system, if it is Initialized, will respond with ID_PONG.
// The final ping time will be encoded in the following 4 bytes (2-5) as an unsigned int
//
// Requires:
// The sender and recipient must already be started via a successful call to Initialize
//
// Parameters:
// host: Either a dotted IP address or a domain name.  Can be 255.255.255.255 for LAN broadcast.
// remotePort: Which port to connect to on the remote machine.
// onlyReplyOnAcceptingConnections: Only request a reply if the remote system has open connections
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::Ping( const char* host, unsigned short remotePort, bool onlyReplyOnAcceptingConnections )
{
	if ( host == 0 )
		return;

	if ( IsActive() == false )
		return;

	// If the host starts with something other than 0, 1, or 2 it's (probably) a domain name.
	if ( host[ 0 ] < '0' || host[ 0 ] > '2' )
	{
		host = ( char* ) SocketLayer::Instance()->DomainNameToIP( host );
	}

	if ( host == NULL )
		return;

	PlayerID playerId;
	IPToPlayerID( host, remotePort, &playerId );

	// disabled so can send while shutting down
//	if (GetRemoteSystemFromPlayerID(playerId))
//		return;

	if (strcmp(host, "255.255.255.255")==0)
	{
		RakNet::BitStream bitStream( sizeof(unsigned char) + sizeof(unsigned int) );
		if ( onlyReplyOnAcceptingConnections )
			bitStream.Write((unsigned char)ID_UNCONNECTED_PING_OPEN_CONNECTIONS);
		else
			bitStream.Write((unsigned char)ID_UNCONNECTED_PING);
		// No timestamp for 255.255.255.255
		SocketLayer::Instance()->SendTo( connectionSocket, (const char*)bitStream.GetData(), bitStream.GetNumberOfBytesUsed(), ( char* ) host, remotePort );
	}
	else
	{
		RequestedConnectionStruct *rcs = requestedConnectionList.WriteLock();
		rcs->playerId=playerId;
		rcs->nextRequestTime=RakNet::GetTime();
		rcs->requestsMade=0;
		rcs->data=0;
		if (onlyReplyOnAcceptingConnections)
			rcs->actionToTake=RequestedConnectionStruct::PING_OPEN_CONNECTIONS;
		else
			rcs->actionToTake=RequestedConnectionStruct::PING;
		requestedConnectionList.WriteUnlock();
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the last ping time read for the specific player or -1 if none read yet
//
// Parameters:
// target - whose time to read
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetLastPing( PlayerID playerId ) const
{
	RemoteSystemStruct * remoteSystem = GetRemoteSystemFromPlayerID( playerId );

	if ( remoteSystem == 0 )
		return -1;

	return remoteSystem->pingTime;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the lowest ping time read or -1 if none read yet
//
// Parameters:
// target - whose time to read
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetLowestPing( PlayerID playerId ) const
{
	RemoteSystemStruct * remoteSystem = GetRemoteSystemFromPlayerID( playerId );

	if ( remoteSystem == 0 )
		return -1;

	return remoteSystem->lowestPing;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Return the unique PlayerID that represents you on the the network
// Note that unlike in previous versions, this is a struct and is not sequential
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
PlayerID RakPeer::GetInternalID( void ) const
{
	return myPlayerId;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Return the unique address identifier that represents you on the the network and is based on your external
// IP / port (the IP / port the specified player uses to communicate with you)
// Note that unlike in previous versions, this is a struct and is not sequential
//
// Parameters:
// target: Which remote system you are referring to for your external ID
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
PlayerID RakPeer::GetExternalID( PlayerID target ) const
{
	RemoteSystemStruct * remoteSystem = GetRemoteSystemFromPlayerID( target );

	if ( remoteSystem == 0 )
		return UNASSIGNED_PLAYER_ID;
	else
		return remoteSystem->myExternalPlayerId;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Change the MTU size in order to improve performance when sending large packets
// This can only be called when not connected.
// A too high of value will cause packets not to arrive at worst and be fragmented at best.
// A too low of value will split packets unnecessarily.
//
// Parameters:
// size: Set according to the following table:
// 1500. The largest Ethernet packet size
// This is the typical setting for non-PPPoE, non-VPN connections. The default value for NETGEAR routers, adapters and switches.
// 1492. The size PPPoE prefers.
// 1472. Maximum size to use for pinging. (Bigger packets are fragmented.)
// 1468. The size DHCP prefers.
// 1460. Usable by AOL if you don't have large email attachments, etc.
// 1430. The size VPN and PPTP prefer.
// 1400. Maximum size for AOL DSL.
// 576. Typical value to connect to dial-up ISPs. (Default)
//
// Returns:
// False on failure (we are connected).  True on success.  Maximum allowed size is MAXIMUM_MTU_SIZE
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::SetMTUSize( int size )
{
	if ( IsActive() )
		return false;

	if ( size < 512 )
		size = 512;
	else if ( size > MAXIMUM_MTU_SIZE )
		size = MAXIMUM_MTU_SIZE;

	MTUSize = size;

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the current MTU size
//
// Returns:
// The MTU sized specified in SetMTUSize
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetMTUSize( void ) const
{
	return MTUSize;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the number of IP addresses we have
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::GetNumberOfAddresses( void )
{
	char ipList[ 10 ][ 16 ];
	memset( ipList, 0, sizeof( char ) * 16 * 10 );
	SocketLayer::Instance()->GetMyIP( ipList );

	int i = 0;

	while ( ipList[ i ][ 0 ] )
		i++;

	return i;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Given a PlayerID struct, returns the dotted IP address string this binaryAddress field represents
//
// Returns:
// Null terminated dotted IP address string.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
const char* RakPeer::PlayerIDToDottedIP( PlayerID playerId ) const
{
	in_addr in;
	in.s_addr = playerId.binaryAddress;
	return inet_ntoa( in );
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns an IP address at index 0 to GetNumberOfAddresses-1
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
const char* RakPeer::GetLocalIP( unsigned int index )
{
	static char ipList[ 10 ][ 16 ];

	if ( index >= 10 )
		index = 9;

	memset( ipList, 0, sizeof( char ) * 16 * 10 );

	SocketLayer::Instance()->GetMyIP( ipList );

	return ipList[ index ];
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Allow or disallow connection responses from any IP. Normally this should be false, but may be necessary
// when connection to servers with multiple IP addresses
//
// Parameters:
// allow - True to allow this behavior, false to not allow.  Defaults to false.  Value persists between connections
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::AllowConnectionResponseIPMigration( bool allow )
{
	allowConnectionResponseIPMigration = allow;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Sends a one byte message ID_ADVERTISE_SYSTEM to the remote unconnected system.
// This will tell the remote system our external IP outside the LAN, and can be used for NAT punch through
//
// Requires:
// The sender and recipient must already be started via a successful call to Initialize
//
// host: Either a dotted IP address or a domain name
// remotePort: Which port to connect to on the remote machine.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::AdvertiseSystem( char *host, unsigned short remotePort, const char *data, int dataLength )
{
	if ( IsActive() == false )
		return ;

	// This is a security measure.  Don't send data longer than this value
	assert(dataLength <= MAX_OFFLINE_DATA_LENGTH);
	assert(dataLength>=0);

	// If the host starts with something other than 0, 1, or 2 it's (probably) a domain name.
	if ( host[ 0 ] < '0' || host[ 0 ] > '2' )
	{
		host = ( char* ) SocketLayer::Instance()->DomainNameToIP( host );
	}

	PlayerID playerId;
	IPToPlayerID( host, remotePort, &playerId );

	// disabled so can send while shutting down
	//if (GetRemoteSystemFromPlayerID(playerId))
	//	return;

	RequestedConnectionStruct *rcs = requestedConnectionList.WriteLock();
	rcs->playerId=playerId;
	rcs->nextRequestTime=RakNet::GetTime();
	rcs->requestsMade=0;
	if (data && dataLength>0)
	{
		rcs->data=new char [dataLength];
		rcs->dataLength=dataLength;
		memcpy(rcs->data, data, dataLength);
	}
	else
	{
		rcs->data=0;
		rcs->dataLength=0;
	}
	rcs->actionToTake=RequestedConnectionStruct::ADVERTISE_SYSTEM;
	requestedConnectionList.WriteUnlock();

//	unsigned char c = ID_ADVERTISE_SYSTEM;
//	RakNet::BitStream temp(sizeof(c));
//	temp.Write((unsigned char)c);
//	if (data && dataLength>0)
//		temp.Write(data, dataLength);
//	Send(&temp, SYSTEM_PRIORITY, UNRELIABLE, 0, playerId, false);
	//SendBuffered(&temp, SYSTEM_PRIORITY, UNRELIABLE, 0, playerId, false, RemoteSystemStruct::DISCONNECT_ASAP);
//	SocketLayer::Instance()->SendTo( connectionSocket, (const char*)temp.GetData(), temp.GetNumberOfBytesUsed(), ( char* ) host, remotePort );
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Put a packet back at the end of the receive queue in case you don't want to deal with it immediately
//
// Parameters
// packet: The packet you want to push back.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::PushBackPacket( Packet *packet )
{
	if ( packet )
	{
#ifdef _DEBUG
		assert( packet->data );
#endif

		incomingPacketQueue.pushAtHead( packet );
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakNetStatisticsStruct * const RakPeer::GetStatistics( PlayerID playerId )
{
	RemoteSystemStruct * rss;
	rss = GetRemoteSystemFromPlayerID( playerId );

	if ( rss && endThreads==false )
		return rss->reliabilityLayer.GetStatistics();

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::SendConnectionRequest( const char* host, unsigned short remotePort )
{
	PlayerID playerId;
//	RemoteSystemStruct *rss;
//	bool success;
	IPToPlayerID( host, remotePort, &playerId );
//	rss=AssignPlayerIDToRemoteSystemList(playerId, RemoteSystemStruct::REQUESTED_CONNECTION);
//	if (rss==0)
//		return false; // full or already connected

	// Already connected?
	if (GetRemoteSystemFromPlayerID(playerId))
		return false;

	RequestedConnectionStruct *rcs = requestedConnectionList.WriteLock();
	rcs->playerId=playerId;
	rcs->nextRequestTime=RakNet::GetTime();
	rcs->requestsMade=0;
	rcs->data=0;
	rcs->actionToTake=RequestedConnectionStruct::CONNECT;
	requestedConnectionList.WriteUnlock();

	// Request will be sent in the other thread

	//char c = ID_OPEN_CONNECTION_REQUEST;
	//SocketLayer::Instance()->SendTo( connectionSocket, (char*)&c, 1, ( char* ) host, remotePort );


	/*
	RakNet::BitStream temp( sizeof(unsigned char) + outgoingPasswordBitStream.GetNumberOfBytesUsed() );
	temp.Write( (unsigned char) ID_CONNECTION_REQUEST );
	if ( outgoingPasswordBitStream.GetNumberOfBytesUsed() > 0 )
		temp.Write( ( char* ) outgoingPasswordBitStream.GetData(), outgoingPasswordBitStream.GetNumberOfBytesUsed() );
//	success=Send(&temp, SYSTEM_PRIORITY, RELIABLE, 0, playerId, false);
	SendBuffered(&temp, SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, RemoteSystemStruct::REQUESTED_CONNECTION);
//#ifdef _DEBUG
//	assert(success);
//#endif
*/

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::IPToPlayerID( const char* host, unsigned short remotePort, PlayerID *playerId )
{
	if ( host == 0 )
		return ;

	playerId->binaryAddress = inet_addr( host );

	playerId->port = remotePort;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RemoteSystemStruct *RakPeer::GetRemoteSystemFromPlayerID( PlayerID playerID ) const
{
	unsigned i;

	if ( playerID == UNASSIGNED_PLAYER_ID )
		return 0;

	for ( i = 0; i < remoteSystemListSize; i++ )
		if ( remoteSystemList[ i ].playerId == playerID )
			return remoteSystemList + i;

	return 0;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ParseConnectionRequestPacket( RakPeer::RemoteSystemStruct *remoteSystem, PlayerID playerId, const char *data, int byteSize )
{
	// If we are full tell the sender.
	if ( !AllowIncomingConnections() )
	{
		unsigned char c = ID_NO_FREE_INCOMING_CONNECTIONS;
		// SocketLayer::Instance()->SendTo( rakPeer->connectionSocket, ( char* ) & c, sizeof( char ), playerId.binaryAddress, playerId.port );
		SendImmediate(( char* ) & c, sizeof( char )*8, SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, false, RakNet::GetTime());
		remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
	}
	else
	{
		remoteSystem->connectMode=RemoteSystemStruct::HANDLING_CONNECTION_REQUEST;

		// Connect this player assuming we have open slots
		OnConnectionRequest( remoteSystem );
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::OnConnectionRequest( RakPeer::RemoteSystemStruct *remoteSystem )
{
	if ( AllowIncomingConnections() )
	{


		RakNet::BitStream bitStream(sizeof(unsigned char)+sizeof(unsigned short)+sizeof(unsigned int)+sizeof(unsigned short)+sizeof(PlayerIndex));
		bitStream.Write((unsigned char)ID_CONNECTION_REQUEST_ACCEPTED);
		bitStream.Write((unsigned short)myPlayerId.port);

		bitStream.Write(remoteSystem->playerId.binaryAddress);
		bitStream.Write(remoteSystem->playerId.port);
		bitStream.Write(( PlayerIndex ) GetIndexFromPlayerID( remoteSystem->playerId ));

		SendImmediate((char*)bitStream.GetData(), bitStream.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, remoteSystem->playerId, false, false, RakNet::GetTime());
	}
	else
	{
		unsigned char c = ID_NO_FREE_INCOMING_CONNECTIONS;

		SendImmediate((char*)&c, sizeof(c)*8, SYSTEM_PRIORITY, RELIABLE, 0, remoteSystem->playerId, false, false, RakNet::GetTime());
		remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::NotifyAndFlagForDisconnect( PlayerID playerId, bool performImmediate )
{
	RakNet::BitStream temp( sizeof(unsigned char) );
	temp.Write( (unsigned char) ID_DISCONNECTION_NOTIFICATION );
	if (performImmediate)
	{
		SendImmediate((char*)temp.GetData(), temp.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, false, RakNet::GetTime());
		RemoteSystemStruct *rss=GetRemoteSystemFromPlayerID(playerId);
		rss->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
	}
	else
	{
		SendBuffered(&temp, SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, RemoteSystemStruct::DISCONNECT_ASAP);
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned short RakPeer::GetNumberOfRemoteInitiatedConnections( void ) const
{
	unsigned short i, numberOfIncomingConnections;

	if ( remoteSystemList == 0 || endThreads == true )
		return 0;

	numberOfIncomingConnections = 0;

	for ( i = 0; i < remoteSystemListSize; i++ )
	{
		if ( remoteSystemList[ i ].playerId != UNASSIGNED_PLAYER_ID && remoteSystemList[ i ].weInitiatedTheConnection == false && remoteSystemList[i].connectMode==RemoteSystemStruct::CONNECTED)
			numberOfIncomingConnections++;
	}

	return numberOfIncomingConnections;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RemoteSystemStruct * RakPeer::AssignPlayerIDToRemoteSystemList( PlayerID playerId, RemoteSystemStruct::ConnectMode connectionMode )
{
	RemoteSystemStruct * remoteSystem = 0;
	unsigned i;
	unsigned int time = RakNet::GetTime();

	// If this guy is already connected, return 0. This needs to be checked inside the mutex
	// because threads may call the connection routine multiple times at the same time
	for ( i = 0; i < remoteSystemListSize; i++ )
	{
		if ( remoteSystemList[ i ].playerId == playerId )
		{
			return 0;
		}
	}

	for ( i = 0; i < remoteSystemListSize; i++ )
	{
		if ( remoteSystemList[ i ].playerId == UNASSIGNED_PLAYER_ID)
		{
			remoteSystem=remoteSystemList+i;
			remoteSystem->playerId = playerId; // This one line causes future incoming packets to go through the reliability layer

			remoteSystem->pingTime = -1;

			remoteSystem->connectMode=connectionMode;
			remoteSystem->lowestPing = -1;
			remoteSystem->nextPingTime = 0; // Ping immediately
			remoteSystem->weInitiatedTheConnection = false;
			remoteSystem->connectionTime = time;
			remoteSystem->myExternalPlayerId = UNASSIGNED_PLAYER_ID;
			remoteSystem->lastReliableSend=time;

			// Reserve this reliability layer for ourselves.
			remoteSystem->reliabilityLayer.Reset();

			return remoteSystem;
		}
	}

	return remoteSystem;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::PushPortRefused( PlayerID target )
{
	// Tell the game we can't connect to this host
	Packet * p;
	p = packetPool.GetPointer();
	p->data = new unsigned char[ 1 ];
	p->data[ 0 ] = ID_REMOTE_PORT_REFUSED;
	p->length = sizeof( char );
	p->playerId = target; // We don't know this!
	p->playerIndex = ( PlayerIndex ) GetIndexFromPlayerID( p->playerId );

#ifdef _DEBUG

	assert( p->data );
#endif
	// Relay this message to the game
	incomingQueueMutex.Lock();
	incomingPacketQueue.push( p );
	incomingQueueMutex.Unlock();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::AllowIncomingConnections(void) const
{
	return GetNumberOfRemoteInitiatedConnections() < GetMaximumIncomingConnections();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::PingInternal( PlayerID target, bool performImmediate )
{
	if ( IsActive() == false )
		return ;

	RakNet::BitStream bitStream(sizeof(unsigned char)+sizeof(unsigned int));
	bitStream.Write((unsigned char)ID_CONNECTED_PING);
	unsigned int currentTime = RakNet::GetTime();
	bitStream.Write(currentTime);
	if (performImmediate)
		SendImmediate( (char*)bitStream.GetData(), bitStream.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, UNRELIABLE, 0, target, false, false, currentTime );
	else
		Send( &bitStream, SYSTEM_PRIORITY, UNRELIABLE, 0, target, false );
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::CloseConnectionInternalBuffered( PlayerID target, bool sendDisconnectionNotification )
{
	if ( remoteSystemList == 0 || endThreads == true )
		return;

	if (sendDisconnectionNotification)
	{
		NotifyAndFlagForDisconnect(target, false);
	}
	else
	{
		BufferedCommandStruct *bcs;
		bcs=bufferedCommands.WriteLock();
		bcs->command=BufferedCommandStruct::BCS_CLOSE_CONNECTION;
		bcs->playerId=target;
		bcs->data=0;
		bufferedCommands.WriteUnlock();
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::CloseConnectionInternalImmediate( PlayerID target)
{
	if ( remoteSystemList == 0 || endThreads == true )
		return;

	for (unsigned int i = 0 ; i < remoteSystemListSize; i++ )
	{
		if ( remoteSystemList[ i ].playerId == target )
		{
			// Reserve this reliability layer for ourselves
			remoteSystemList[ i ].playerId = UNASSIGNED_PLAYER_ID;
			//	remoteSystemList[ i ].allowPlayerIdAssigment=false;

			// Remove any remaining packets.
			remoteSystemList[ i ].reliabilityLayer.Reset();
			break;
		}
	}

}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::ValidSendTarget(PlayerID playerId, bool broadcast)
{
	unsigned remoteSystemIndex;
	for ( remoteSystemIndex = 0; remoteSystemIndex < remoteSystemListSize; remoteSystemIndex++ )
	{
		if ( remoteSystemList[ remoteSystemIndex ].playerId != UNASSIGNED_PLAYER_ID &&
			remoteSystemList[ remoteSystemIndex ].connectMode==RakPeer::RemoteSystemStruct::CONNECTED && // Not fully connected players are not valid user-send targets because the reliability layer wasn't reset yet
			( ( broadcast == false && remoteSystemList[ remoteSystemIndex ].playerId == playerId ) ||
			( broadcast == true && remoteSystemList[ remoteSystemIndex ].playerId != playerId ) )
			)
			return true;
	}

	return false;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SendBuffered( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, RemoteSystemStruct::ConnectMode connectionMode )
{
	BufferedCommandStruct *bcs;
	bcs=bufferedCommands.WriteLock();

	bcs->data = new char[bitStream->GetNumberOfBytesUsed()]; // Making a copy doesn't lose efficiency because I tell the reliability layer to use this allocation for its own copy
	memcpy(bcs->data, bitStream->GetData(), bitStream->GetNumberOfBytesUsed());
    bcs->numberOfBitsToSend=bitStream->GetNumberOfBitsUsed();
	bcs->priority=priority;
	bcs->reliability=reliability;
	bcs->orderingChannel=orderingChannel;
	bcs->playerId=playerId;
	bcs->broadcast=broadcast;
	bcs->connectionMode=connectionMode;
	bcs->command=BufferedCommandStruct::BCS_SEND;
	bufferedCommands.WriteUnlock();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::SendImmediate( char *data, int numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool useCallerDataAllocation, unsigned int currentTime )
{
	unsigned *sendList;
	unsigned sendListSize;
	bool callerDataAllocationUsed;
	unsigned remoteSystemIndex, sendListIndex; // Iterates into the list of remote systems
	callerDataAllocationUsed=false;

	sendList=(unsigned *)alloca(sizeof(unsigned)*remoteSystemListSize);
	sendListSize=0;

	for ( remoteSystemIndex = 0; remoteSystemIndex < remoteSystemListSize; remoteSystemIndex++ )
	{
		if ( remoteSystemList[ remoteSystemIndex ].playerId != UNASSIGNED_PLAYER_ID &&
			( ( broadcast == false && remoteSystemList[ remoteSystemIndex ].playerId == playerId ) ||
			( broadcast == true && remoteSystemList[ remoteSystemIndex ].playerId != playerId ) )	)
				sendList[sendListSize++]=remoteSystemIndex;
	}

	if (sendListSize==0)
		return false;

	for (sendListIndex=0; sendListIndex < sendListSize; sendListIndex++)
	{
		// Send may split the packet and thus deallocate data.  Don't assume data is valid if we use the callerAllocationData
		bool useData = useCallerDataAllocation && callerDataAllocationUsed==false && sendListIndex+1==sendListSize;
		remoteSystemList[sendList[sendListIndex]].reliabilityLayer.Send( data, numberOfBitsToSend, priority, reliability, orderingChannel, useData==false, MTUSize, currentTime );
		if (useData)
			callerDataAllocationUsed=true;


		if (reliability==RELIABLE || reliability==RELIABLE_ORDERED || reliability==RELIABLE_SEQUENCED)
			remoteSystemList[sendList[sendListIndex]].lastReliableSend=currentTime;
	}

	// Return value only meaningful if true was passed for useCallerDataAllocation.  Means the reliability layer used that data copy, so the caller should not deallocate it
	return callerDataAllocationUsed;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ClearBufferedCommands(void)
{
	BufferedCommandStruct *bcs;
	while ((bcs=bufferedCommands.ReadLock())!=0)
	{
		if (bcs->data)
			delete [] bcs->data;

        bufferedCommands.ReadUnlock();
	}
	bufferedCommands.Clear();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ClearRequestedConnectionList(void)
{
	RequestedConnectionStruct *bcs;
	while ((bcs=requestedConnectionList.ReadLock())!=0)
	{
		if (bcs->data)
			delete [] bcs->data;

		requestedConnectionList.ReadUnlock();
	}
	requestedConnectionList.Clear();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
void __stdcall ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer )
#else
void ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer )
#endif
{
	PlayerID playerId;
//	unsigned i;
	RakPeer::RemoteSystemStruct *remoteSystem;
	playerId.binaryAddress = binaryAddress;
	playerId.port = port;

	// UNCONNECTED MESSAGE to establish a connection
	if ((unsigned char)(data)[0] == ID_OPEN_CONNECTION_REPLY && length == sizeof(unsigned char))
	{
		// Verify that we were waiting for this
		bool acceptOpenConnection;
		int actionToTake=0;
		char data[MAX_OFFLINE_DATA_LENGTH];
		unsigned short dataLength;
		RakPeer::RequestedConnectionStruct *rcsFirst, *rcs;
		rcsFirst = rakPeer->requestedConnectionList.ReadLock();
		rcs=rcsFirst;
		acceptOpenConnection=false;
		while (rcs)
		{
			if (rcs->playerId==playerId)
			{
				acceptOpenConnection=true;
				actionToTake|=(int)rcs->actionToTake;
				if (rcs->data)
				{
#ifdef _DEBUG
					assert(rcs->actionToTake==RakPeer::RequestedConnectionStruct::ADVERTISE_SYSTEM);
					assert(rcs->dataLength <= MAX_OFFLINE_DATA_LENGTH);
#endif
					memcpy(data, rcs->data, rcs->dataLength);
					dataLength=rcs->dataLength;
					delete [] rcs->data;
					rcs->data=0;
				}

				if (rcs==rcsFirst)
				{
					rakPeer->requestedConnectionList.ReadUnlock();
					rcsFirst=rakPeer->requestedConnectionList.ReadLock();
					rcs=rcsFirst;
					continue;
				}
				else
				{
					// Duplicate call - cancel it
					rcs->playerId=UNASSIGNED_PLAYER_ID;
				}
			}

			rcs=rakPeer->requestedConnectionList.ReadLock();
		}
		if (rcsFirst)
			rakPeer->requestedConnectionList.CancelReadLock(rcsFirst);

		if (acceptOpenConnection)
		{
			remoteSystem=rakPeer->AssignPlayerIDToRemoteSystemList(playerId, RakPeer::RemoteSystemStruct::UNVERIFIED_SENDER);
	//		if (remoteSystem==0)
	//			remoteSystem=rakPeer->GetRemoteSystemFromPlayerID(playerId); // Get the already connected guy
			if (remoteSystem)
			{
				unsigned int time = RakNet::GetTime();
				if (actionToTake & RakPeer::RequestedConnectionStruct::CONNECT)
				{
					remoteSystem->connectMode=RakPeer::RemoteSystemStruct::REQUESTED_CONNECTION;
					remoteSystem->weInitiatedTheConnection=true;

					RakNet::BitStream temp( sizeof(unsigned char) );
					temp.Write( (unsigned char) ID_CONNECTION_REQUEST );
					rakPeer->SendImmediate((char*)temp.GetData(), temp.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, false, time );
				}

				if ((actionToTake & RakPeer::RequestedConnectionStruct::PING) || (actionToTake & RakPeer::RequestedConnectionStruct::PING_OPEN_CONNECTIONS))
				{
					RakNet::BitStream temp( sizeof(unsigned char) + sizeof(unsigned int) );
					if ( actionToTake & RakPeer::RequestedConnectionStruct::PING_OPEN_CONNECTIONS )
						temp.Write((unsigned char)ID_UNCONNECTED_PING_OPEN_CONNECTIONS);
					else
						temp.Write((unsigned char)ID_UNCONNECTED_PING);
					temp.Write(time);

//					SocketLayer::Instance()->SendTo( connectionSocket, (const char*)bitStream.GetData(), bitStream.GetNumberOfBytesUsed(), ( char* ) host, remotePort );
					rakPeer->SendImmediate((char*)temp.GetData(), temp.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, false, time );
				}

				if (actionToTake & RakPeer::RequestedConnectionStruct::ADVERTISE_SYSTEM)
				{
					RakNet::BitStream temp;
					temp.Write((unsigned char)ID_ADVERTISE_SYSTEM);
					if (dataLength>0)
						temp.Write(data, dataLength);
					rakPeer->SendImmediate((char*)temp.GetData(), temp.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, false, time );
					remoteSystem->connectMode=RakPeer::RemoteSystemStruct::DISCONNECT_ASAP;
				}
			}
		}

		return;
	}
	// UNCONNECTED MESSAGE Broadcast ping with no data
	else if ( ( (unsigned char) data[ 0 ] == ID_UNCONNECTED_PING_OPEN_CONNECTIONS
		|| (unsigned char)(data)[0] == ID_UNCONNECTED_PING)	&& length == sizeof(unsigned char) )
	{
		if ( (unsigned char)(data)[0] == ID_UNCONNECTED_PING ||
			rakPeer->AllowIncomingConnections() ) // Open connections with players
		{
			RakNet::BitStream outBitStream;
			outBitStream.Write((unsigned char)ID_PONG); // Should be named ID_UNCONNECTED_PONG eventually
			SocketLayer::Instance()->SendTo( rakPeer->connectionSocket, (const char*)outBitStream.GetData(), outBitStream.GetNumberOfBytesUsed(), (char*)rakPeer->PlayerIDToDottedIP(playerId) , playerId.port );
		}

		return;
	}
	// UNCONNECTED MESSAGE Pong with no data
	else if ((unsigned char) data[ 0 ] == ID_PONG && length == sizeof(unsigned char) )
	{
		Packet * packet = rakPeer->packetPool.GetPointer();
		packet->data = new unsigned char[ sizeof( char )+sizeof(unsigned int) ];
		unsigned int zero=0;
		packet->data[ 0 ] = ID_PONG;
		memcpy(packet->data+sizeof( char ), (char*)&zero, sizeof(unsigned int));
		packet->length = sizeof( char );
		packet->bitSize = sizeof( char ) * 8;
		packet->playerId = playerId;
		packet->playerIndex = ( PlayerIndex ) rakPeer->GetIndexFromPlayerID( playerId );
		rakPeer->incomingQueueMutex.Lock();
		rakPeer->incomingPacketQueue.push( packet );
		rakPeer->incomingQueueMutex.Unlock();
		return;
	}

	remoteSystem = rakPeer->GetRemoteSystemFromPlayerID( playerId );
	if ( remoteSystem )
	{
		// Handle regular incoming data
		// HandleSocketReceiveFromConnectedPlayer is only safe to be called from the same thread as Update, which is this thread
		if ( remoteSystem->reliabilityLayer.HandleSocketReceiveFromConnectedPlayer( data, length ) == false )
		{
			// These kinds of packets may have been duplicated and incorrectly determined to be
			// cheat packets.  Anything else really is a cheat packet
			if ( !(
			( (unsigned char)data[0] == ID_OPEN_CONNECTION_REQUEST && length == 1 ) ||
			( (unsigned char)data[0] == ID_OPEN_CONNECTION_REPLY && length == 1 ) ||
			(((unsigned char)data[0] == ID_UNCONNECTED_PING_OPEN_CONNECTIONS || (unsigned char)(data)[0] == ID_UNCONNECTED_PING) && length == sizeof(unsigned char)+sizeof(unsigned int) ) ||
			( (unsigned char)data[0] == ID_PONG && length >= sizeof(unsigned char)+sizeof(unsigned int) ) ||
			( (unsigned char)data[0] == ID_ADVERTISE_SYSTEM && length<MAX_OFFLINE_DATA_LENGTH )
			) )
			{
				// Cheater
				Packet * packet = rakPeer->packetPool.GetPointer();
				packet->data = new unsigned char[ 1 ];
				packet->data[ 0 ] = ID_MODIFIED_PACKET;
				packet->length = sizeof( char );
				packet->bitSize = sizeof( char ) * 8;
				packet->playerId = playerId;
				packet->playerIndex = ( PlayerIndex ) rakPeer->GetIndexFromPlayerID( playerId );
				rakPeer->incomingQueueMutex.Lock();
				rakPeer->incomingPacketQueue.push( packet );
				rakPeer->incomingQueueMutex.Unlock();
			}
		}
	}
	else
	{

		// The reason for ID_OPEN_CONNECTION_REQUEST and ID_OPEN_CONNECTION_REPLY is that they are only one byte so I can be sure
		// that they are offline messages and I know to reset the connections.  This is because the smallest possible connected packet is 17 bits.
		// This is the only way I can tell for sure that a message is asking for a new connection.
		// This fixes bugs where I ignore a connection request from a connected player or handle a message that looks like a connection request but actually wasn't.
		if ((unsigned char)(data)[0] == ID_OPEN_CONNECTION_REQUEST && length == sizeof(unsigned char))
		{
			remoteSystem=rakPeer->AssignPlayerIDToRemoteSystemList(playerId, RakPeer::RemoteSystemStruct::UNVERIFIED_SENDER);
			if (remoteSystem) // If this guy is already connected remote system will be 0
			{
				char c = ID_OPEN_CONNECTION_REPLY;
				SocketLayer::Instance()->SendTo( rakPeer->connectionSocket, (char*)&c, 1, playerId.binaryAddress, playerId.port );
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::RunUpdateCycle( void )
{
	RakPeer::RemoteSystemStruct * remoteSystem;
	unsigned remoteSystemIndex;
	Packet *packet;
	int ping, lastPing;
	// int currentSentBytes,currentReceivedBytes;
//	unsigned numberOfBytesUsed;
	//PlayerID authoritativeClientPlayerId;
	int bitSize, byteSize;
	char *data;
	int errorCode;
	int gotData;
	unsigned int time;
	PlayerID playerId;
	BufferedCommandStruct *bcs;
	bool callerDataAllocationUsed;
	RakNetStatisticsStruct *rnss;

	do
	{
		// Read a packet
		gotData = SocketLayer::Instance()->RecvFrom( connectionSocket, this, &errorCode );

		if ( gotData == SOCKET_ERROR )
		{
#ifdef _WIN32
			if ( errorCode == WSAECONNRESET )
			{
				gotData=false;
				// 11/14/05 - RecvFrom now calls HandlePortUnreachable rather than PushPortRefused
				//PushPortRefused( UNASSIGNED_PLAYER_ID );
				//closesocket(peer->connectionSocket);

				//peer->connectionSocket = SocketLayer::Instance()->CreateBoundSocket(peer->myPlayerId.port, true);
			}
			else
				if ( errorCode != 0 && endThreads == false )
				{
#ifdef _DO_PRINTF
					printf( "Server RecvFrom critical failure!\n" );
#endif
					// Some kind of critical error
					// peer->isRecvfromThreadActive=false;
					endThreads = true;
					Disconnect( 0 );
					return false;
				}

#else
			if ( errorCode == -1 )
			{
				// isRecvfromThreadActive=false;
				endThreads = true;
				Disconnect( 0 );
				return false;
			}
#endif
		}

		if ( endThreads )
			return false;
	}
	while ( gotData>0 ); // Read until there is nothing left

	time=0;

	// Process all the deferred user thread Send and connect calls

	while ( ( bcs = bufferedCommands.ReadLock() ) != 0 ) // Don't immediately check mutex since it's so slow to activate it
	{
		if (bcs->command==BufferedCommandStruct::BCS_SEND)
		{
			// This will create a new connection if requested
			if (bcs->connectionMode!=RemoteSystemStruct::NO_ACTION)
			{
				remoteSystem=AssignPlayerIDToRemoteSystemList(bcs->playerId, bcs->connectionMode);
				if (!remoteSystem)
				{
					// Does this system already exist?
					remoteSystem=GetRemoteSystemFromPlayerID(bcs->playerId);
					if (remoteSystem)
						remoteSystem->connectMode=bcs->connectionMode;
				}
			}

			// GetTime is a very slow call so do it once and as late as possible
			if (time==0)
				time = RakNet::GetTime();

			callerDataAllocationUsed=SendImmediate((char*)bcs->data, bcs->numberOfBitsToSend, bcs->priority, bcs->reliability, bcs->orderingChannel, bcs->playerId, bcs->broadcast, true, time);
			if ( callerDataAllocationUsed==false )
				delete [] bcs->data;
		}
		else
		{
#ifdef _DEBUG
			assert(bcs->command==BufferedCommandStruct::BCS_CLOSE_CONNECTION);
#endif
			CloseConnectionInternalImmediate(bcs->playerId);
		}

#ifdef _DEBUG
		bcs->data=0;
#endif
		bufferedCommands.ReadUnlock();
	}

	// Process connection attempts
	RequestedConnectionStruct *rcsFirst, *rcs;
	bool condition1, condition2;
    rcsFirst = requestedConnectionList.ReadLock();
	rcs=rcsFirst;
	while (rcs)
	{
		if (time==0)
			time = RakNet::GetTime();

		if (rcs->nextRequestTime < time)
		{
			condition1=rcs->requestsMade==3;
			condition2=(bool)((rcs->playerId==UNASSIGNED_PLAYER_ID)==1);
			// If too many requests made or a hole then remove this if possible, otherwise invalidate it
			if (condition1 || condition2)
			{
				if (rcs->data)
				{
					delete [] rcs->data;
					rcs->data=0;
				}

				if (condition1 && !condition2 && rcs->actionToTake==RequestedConnectionStruct::CONNECT)
				{
					// Tell user of connection attempt failed
					packet = packetPool.GetPointer();
					packet->data = new unsigned char [ sizeof( char ) ];
					packet->data[ 0 ] = ID_CONNECTION_ATTEMPT_FAILED; // Attempted a connection and couldn't
					packet->length = sizeof( char );
					packet->bitSize = ( sizeof( char ) * 8);
					packet->playerId = myPlayerId;
					packet->playerIndex = 65535;

					incomingQueueMutex.Lock();
					( incomingPacketQueue ).push( packet );
					incomingQueueMutex.Unlock();
				}

				// Remove this if possible
				if (rcs==rcsFirst)
				{
					requestedConnectionList.ReadUnlock();
					rcsFirst = requestedConnectionList.ReadLock();
					rcs=rcsFirst;
				}
				else
				{
					// Hole in the middle
					rcs->playerId=UNASSIGNED_PLAYER_ID;
					rcs=requestedConnectionList.ReadLock();
				}

				continue;
			}

			rcs->requestsMade++;
			rcs->nextRequestTime=time+1000;
			char c = ID_OPEN_CONNECTION_REQUEST;

			SocketLayer::Instance()->SendTo( connectionSocket, (char*)&c, 1, ( char* ) PlayerIDToDottedIP(rcs->playerId), rcs->playerId.port );
		}

		rcs=requestedConnectionList.ReadLock();
	}

	if (rcsFirst)
		requestedConnectionList.CancelReadLock(rcsFirst);

	for ( remoteSystemIndex = 0; remoteSystemIndex < remoteSystemListSize; ++remoteSystemIndex )
	{
		// I'm using playerId from remoteSystemList but am not locking it because this loop is called very frequently and it doesn't
		// matter if we miss or do an extra update.  The reliability layers themselves never care which player they are associated with
		playerId = remoteSystemList[ remoteSystemIndex ].playerId;
		// Allow the playerID for this remote system list to change.  We don't care if it changes now.
	//	remoteSystemList[ remoteSystemIndex ].allowPlayerIdAssigment=true;
		if ( playerId != UNASSIGNED_PLAYER_ID )
		{
			// Found an active remote system
			remoteSystem = remoteSystemList + remoteSystemIndex;
			// Update is only safe to call from the same thread that calls HandleSocketReceiveFromConnectedPlayer,
			// which is this thread

			if (time==0)
				time = RakNet::GetTime();


			if (time > remoteSystem->lastReliableSend && time-remoteSystem->lastReliableSend > 5000 && remoteSystem->connectMode==RemoteSystemStruct::CONNECTED)
			{
				// If no reliable packets are waiting for an ack, do a one byte reliable send so that disconnections are noticed
				rnss=remoteSystem->reliabilityLayer.GetStatistics();
				if (rnss->messagesOnResendQueue==0)
				{
					unsigned char keepAlive=ID_KEEPALIVE;
					SendImmediate((char*)&keepAlive,8,LOW_PRIORITY, RELIABLE, 0, remoteSystem->playerId, false, false, time);
					remoteSystem->lastReliableSend=time+TIMEOUT_TIME;
				}
			}

			remoteSystem->reliabilityLayer.Update( connectionSocket, playerId, MTUSize, time ); // playerId only used for the internet simulator test

			// Check for failure conditions
			if ( remoteSystem->reliabilityLayer.IsDeadConnection() ||
				(remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ASAP && remoteSystem->reliabilityLayer.IsDataWaiting()==false) ||
				((remoteSystem->connectMode!=RemoteSystemStruct::CONNECTED && time > remoteSystem->connectionTime && time - remoteSystem->connectionTime > 10000))
				)
			{
				// Failed.  Inform the user?
				if (remoteSystem->connectMode==RemoteSystemStruct::CONNECTED || remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION)
				{
					// Inform the user of the connection failure.
					packet = packetPool.GetPointer();

					packet->data = new unsigned char [ sizeof( char ) ];
					if (remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION)
						packet->data[ 0 ] = ID_CONNECTION_ATTEMPT_FAILED; // Attempted a connection and couldn't
					else
						packet->data[ 0 ] = ID_CONNECTION_LOST; // DeadConnection

					packet->length = sizeof( char );
					packet->bitSize = sizeof( char ) * 8;
					packet->playerId = playerId;
					packet->playerIndex = ( PlayerIndex ) remoteSystemIndex;

					incomingQueueMutex.Lock();
					( incomingPacketQueue ).push( packet );
					incomingQueueMutex.Unlock();
				}
				// else connection shutting down, don't bother telling the user

#ifdef _DO_PRINTF
				printf("Connection dropped for player %i:%i\n", playerId.binaryAddress, playerId.port);
#endif
				CloseConnectionInternalImmediate( playerId );
				continue;
			}

			// Ping this guy if it is time to do so
			if ( remoteSystem->connectMode==RemoteSystemStruct::CONNECTED && time > remoteSystem->nextPingTime && ( remoteSystem->lowestPing == -1 ) )
			{
				remoteSystem->nextPingTime = time + 5000;
				PingInternal( playerId, true );
			}

			// Find whoever has the lowest player ID
			//if (playerId < authoritativeClientPlayerId)
			// authoritativeClientPlayerId=playerId;

			// Does the reliability layer have any packets waiting for us?
			// To be thread safe, this has to be called in the same thread as HandleSocketReceiveFromConnectedPlayer
			bitSize = remoteSystem->reliabilityLayer.Receive( &data );

			while ( bitSize > 0 )
			{
				// Fast and easy - just use the data that was returned
				byteSize = BITS_TO_BYTES( bitSize );

				// For unknown senders we only accept a few specific packets
				if (remoteSystem->connectMode==RemoteSystemStruct::UNVERIFIED_SENDER)
				{
					if ( (unsigned char)(data)[0] == ID_CONNECTION_REQUEST )
					{
						ParseConnectionRequestPacket(remoteSystem, playerId, data, byteSize);
						delete [] data;
					}
					else if ( ((unsigned char) data[0] == ID_PONG && byteSize >= sizeof(unsigned char)+sizeof(unsigned int)) ||
						((unsigned char) data[0] == ID_ADVERTISE_SYSTEM && byteSize<=MAX_OFFLINE_DATA_LENGTH))
					{
						// Push to the user
						Packet *packet = packetPool.GetPointer();
						packet->data = ( unsigned char* ) data;
						packet->length = byteSize;
						packet->bitSize = byteSize*8;
						packet->playerId = playerId;
						packet->playerIndex=65535;
						incomingQueueMutex.Lock();
						incomingPacketQueue.push( packet );
						incomingQueueMutex.Unlock();

						if (remoteSystem->connectMode!=RemoteSystemStruct::CONNECTED)
						{
							remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
						}
					}
					else if ( ( (unsigned char) data[ 0 ] == ID_UNCONNECTED_PING_OPEN_CONNECTIONS
						|| (unsigned char)(data)[0] == ID_UNCONNECTED_PING)
						&& byteSize == sizeof(unsigned char)+sizeof(unsigned int) )
					{
						RakNet::BitStream inBitStream( data, byteSize, false );
						inBitStream.IgnoreBits(8);
						unsigned int sendPingTime;
						inBitStream.Read(sendPingTime);

						if ( (unsigned char)(data)[0] == ID_UNCONNECTED_PING ||
							AllowIncomingConnections() ) // Open connections with players
						{
							RakNet::BitStream outBitStream;
							outBitStream.Write((unsigned char)ID_PONG); // Should be named ID_UNCONNECTED_PONG eventually
							outBitStream.Write(sendPingTime);
							SendImmediate( (char*)outBitStream.GetData(), outBitStream.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, UNRELIABLE, 0, playerId, false, false, time );
						}
						// else ID_UNCONNECTED_PING_OPEN_CONNECTIONS and we are full so don't send anything

						delete [] data;

						// Disconnect them after replying to their offline ping
						if (remoteSystem->connectMode!=RemoteSystemStruct::CONNECTED)
							remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
					}
					else
					{
						CloseConnectionInternalImmediate( playerId );
#ifdef _DO_PRINTF
						printf("Temporarily banning %i:%i for sending nonsense data\n", playerId.binaryAddress, playerId.port);
#endif
						delete [] data;
					}
				}
				else
				{
					// However, if we are connected we still take a connection request in case both systems are trying to connect to each other
					// at the same time
					if ( (unsigned char)(data)[0] == ID_CONNECTION_REQUEST )
					{
						if (remoteSystem->weInitiatedTheConnection==false)
							ParseConnectionRequestPacket(remoteSystem, playerId, data, byteSize);
						delete [] data;
					}
					else if ( (unsigned char) data[ 0 ] == ID_NEW_INCOMING_CONNECTION && byteSize == sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned short) )
					{
#ifdef _DEBUG
						// This assert can be ignored since it could hit from duplicate packets.
						// It's just here for internal testing since it should only happen rarely and will mostly be from bugs
//						assert(remoteSystem->connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST);
#endif
						if (remoteSystem->connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST ||
							playerId==myPlayerId) // local system connect
						{
							remoteSystem->connectMode=RemoteSystemStruct::CONNECTED;
							PingInternal( playerId, true );

							RakNet::BitStream inBitStream(data, byteSize, false);
							PlayerID bsPlayerId;

							inBitStream.IgnoreBits(8);
							inBitStream.Read(bsPlayerId.binaryAddress);
							inBitStream.Read(bsPlayerId.port);

							// Overwrite the data in the packet
							//					NewIncomingConnectionStruct newIncomingConnectionStruct;
							//					RakNet::BitStream nICS_BS( data, NewIncomingConnectionStruct_Size, false );
							//					newIncomingConnectionStruct.Deserialize( nICS_BS );
							remoteSystem->myExternalPlayerId = bsPlayerId;

							// Send this info down to the game
							packet = packetPool.GetPointer();
							packet->data = ( unsigned char* ) data;
							packet->length = byteSize;
							packet->bitSize = bitSize;
							packet->playerId = playerId;
							packet->playerIndex = ( PlayerIndex ) remoteSystemIndex;

#ifdef _DEBUG
							assert( packet->data );
#endif

							incomingQueueMutex.Lock();
							incomingPacketQueue.push( packet );
							incomingQueueMutex.Unlock();
						}
						else
							delete [] data;
					}
					else if ( (unsigned char) data[ 0 ] == ID_CONNECTED_PONG && byteSize == sizeof(unsigned char)+sizeof(unsigned int)*2 )
					{
						unsigned int sendPingTime, sendPongTime;

						// Copy into the ping times array the current time - the value returned
						// First extract the sent ping
						RakNet::BitStream inBitStream( data, byteSize, false );
						//PingStruct ps;
						//ps.Deserialize(psBS);
						inBitStream.IgnoreBits(8);
						inBitStream.Read(sendPingTime);
						inBitStream.Read(sendPongTime);

						time = RakNet::GetTime(); // Update the time value to be accurate
						ping = time - sendPingTime;
						lastPing = remoteSystem->pingTime;

						// Ignore super high spikes in the average
						if ( lastPing <= 0 || ( ( ( int ) ping < ( lastPing * 3 ) ) && ping < 1200 ) )
						{
							remoteSystem->pingTime = ( short ) ping;

							if ( remoteSystem->lowestPing == -1 || remoteSystem->lowestPing > ping )
								remoteSystem->lowestPing = ping;

							// Most packets should arrive by the ping time.
							remoteSystem->reliabilityLayer.SetLostPacketResendDelay( ping * 2 );
						}

						delete [] data;
					}
					else if ( (unsigned char)data[0] == ID_CONNECTED_PING && byteSize == sizeof(unsigned char)+sizeof(unsigned int) )
					{
						RakNet::BitStream inBitStream( data, byteSize, false );
						inBitStream.IgnoreBits(8);
						unsigned int sendPingTime;
						inBitStream.Read(sendPingTime);

						if ((unsigned char)(data)[0] == ID_CONNECTED_PING)
						{
							RakNet::BitStream outBitStream;
							outBitStream.Write((unsigned char)ID_CONNECTED_PONG);
							outBitStream.Write(sendPingTime);
							time = RakNet::GetTime();
							outBitStream.Write((unsigned int)time);
							SendImmediate( (char*)outBitStream.GetData(), outBitStream.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, UNRELIABLE, 0, playerId, false, false, time );
						}

						delete [] data;
					}
					else if ( (unsigned char) data[ 0 ] == ID_DISCONNECTION_NOTIFICATION )
					{
						packet = packetPool.GetPointer();

						packet->data = ( unsigned char* ) data;
						packet->bitSize = 8;
						packet->length = 1;

						packet->playerId = playerId;
						packet->playerIndex = ( PlayerIndex ) remoteSystemIndex;

						// We shouldn't close the connection immediately because we need to ack the ID_DISCONNECTION_NOTIFICATION
						remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
						//CloseConnectionInternal( playerId, false, true );

#ifdef _DEBUG
						assert( packet->data );
#endif
						// Relay this message to the game
						incomingQueueMutex.Lock();
						incomingPacketQueue.push( packet );
						incomingQueueMutex.Unlock();

					}
					else if ( (unsigned char)(data)[0] == ID_KEEPALIVE && byteSize == sizeof(unsigned char) )
					{
						// Do nothing
						delete [] data;
					}
					else if ( (unsigned char)(data)[0] == ID_CONNECTION_REQUEST_ACCEPTED && byteSize == sizeof(unsigned char)+sizeof(unsigned short)+sizeof(unsigned int)+sizeof(unsigned short)+sizeof(PlayerIndex) )
					{
						// Make sure this connection accept is from someone we wanted to connect to
						bool allowConnection, alreadyConnected;

						if (remoteSystem->connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST || remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION || allowConnectionResponseIPMigration)
							allowConnection=true;
						else
							allowConnection=false;
						if (remoteSystem->connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST)
							alreadyConnected=true;
						else
							alreadyConnected=false;

						if ( allowConnection )
						{
							if (alreadyConnected==false)
							{
								unsigned short remotePort;
								PlayerID externalID;
								PlayerIndex playerIndex;

								RakNet::BitStream inBitStream(data, byteSize, false);
								inBitStream.IgnoreBits(8); // ID_CONNECTION_REQUEST_ACCEPTED
								inBitStream.Read(remotePort);
								inBitStream.Read(externalID.binaryAddress);
								inBitStream.Read(externalID.port);
								inBitStream.Read(playerIndex);

								// Find a free remote system struct to use
								//						RakNet::BitStream casBitS(data, byteSize, false);
								//						ConnectionAcceptStruct cas;
								//						cas.Deserialize(casBitS);
								playerId.port = remotePort;
								remoteSystem->connectMode=RemoteSystemStruct::CONNECTED;

								// The remote system told us our external IP, so save it
								remoteSystem->myExternalPlayerId = externalID;
							}

							// Send the connection request complete to the game
							Packet *packet = packetPool.GetPointer();

							//packet->data = new unsigned char[ byteSize ];
							//memcpy( packet->data, data, byteSize );
							packet->data=(unsigned char*)data;

							// packet->data[0]=ID_CONNECTION_REQUEST_ACCEPTED;
							packet->length = byteSize;
							packet->bitSize = byteSize * 8;
							packet->playerId = playerId;
							packet->playerIndex = ( PlayerIndex ) GetIndexFromPlayerID( playerId );

#ifdef _DEBUG
							assert( packet->data );
#endif
							incomingQueueMutex.Lock();
							incomingPacketQueue.push( packet );
							incomingQueueMutex.Unlock();

							RakNet::BitStream outBitStream(sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned short));
							outBitStream.Write((unsigned char)ID_NEW_INCOMING_CONNECTION);
							outBitStream.Write(playerId.binaryAddress);
							outBitStream.Write(playerId.port);
							SendImmediate( (char*)outBitStream.GetData(), outBitStream.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, false, RakNet::GetTime() );

							if (alreadyConnected==false)
							{
								PingInternal( playerId, true );
							}
						}
						else
						{
							// Tell the remote system the connection failed
							NotifyAndFlagForDisconnect(playerId, true);
#ifdef _DO_PRINTF
							printf( "Error: Got a connection accept when we didn't request the connection.\n" );
#endif
							delete [] data;
						}
					}
					else
					{
						packet = packetPool.GetPointer();
						packet->data = ( unsigned char* ) data;
						packet->length = byteSize;
						packet->bitSize = bitSize;
						packet->playerId = playerId;
						packet->playerIndex = ( PlayerIndex ) remoteSystemIndex;

#ifdef _DEBUG
						assert( packet->data );
#endif
						incomingQueueMutex.Lock();
						incomingPacketQueue.push( packet );
						incomingQueueMutex.Unlock();
					}
				}

				// Does the reliability layer have any more packets waiting for us?
				// To be thread safe, this has to be called in the same thread as HandleSocketReceiveFromConnectedPlayer
				bitSize = remoteSystem->reliabilityLayer.Receive( &data );
			}
		}
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
unsigned __stdcall UpdateNetworkLoop( LPVOID arguments )
#else
void* UpdateNetworkLoop( void* arguments )
#endif
{
	RakPeer * rakPeer = ( RakPeer * ) arguments;

	rakPeer->isMainLoopThreadActive = true;

	while ( rakPeer->endThreads == false )
	{
		rakPeer->RunUpdateCycle();
#ifdef _WIN32
		Sleep( rakPeer->threadSleepTimer );
#else
		usleep( rakPeer->threadSleepTimer * 1000 );
#endif

	}

	rakPeer->isMainLoopThreadActive = false;

	return 0;
}

