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

#ifdef __USE_IO_COMPLETION_PORTS
#include "AsynchronousFileIO.h"
#endif

#ifdef _WIN32 
//#include <Shlwapi.h>
#include <process.h>
#else
#define closesocket close
#include <unistd.h>
#include <pthread.h>
#endif
#include <ctype.h> // toupper

#include "GetTime.h"
#include "PacketEnumerations.h"
#include "HuffmanEncodingTree.h"
#include "PacketPool.h"
#include "Rand.h"
#include "MessageHandlerInterface.h"
#include "StringCompressor.h"
#include "NetworkIDGenerator.h"

// alloca
#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

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

//#define _TEST_AES


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RakPeer()
{
	usingSecurity = false;
	memset( frequencyTable, 0, sizeof( unsigned int ) * 256 );
	rawBytesSent = rawBytesReceived = compressedBytesSent = compressedBytesReceived = 0;
	outputTree = inputTree = 0;
	connectionSocket = INVALID_SOCKET;
	MTUSize = DEFAULT_MTU_SIZE;
	trackFrequencyTable = false;
	maximumIncomingConnections = 0;
	maximumNumberOfPeers = 0;
	remoteSystemListSize=0;
	remoteSystemList = 0;
	bytesSentPerSecond = bytesReceivedPerSecond = 0;
	endThreads = true;
	isMainLoopThreadActive = false;
	// isRecvfromThreadActive=false;
	occasionalPing = false;
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
//	unsigned i;

	// Free the ban list.
	ClearBanList();

	Disconnect( 0 );

	/*
	// Clear out the lists:
	for ( i = 0; i < requestedConnectionsList.size(); i++ )
		delete requestedConnectionsList[ i ];
	requestedConnectionsList.clear();
	*/

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

	unsigned i;

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
		for ( i = 0; i < remoteSystemListSize; i++ )
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

		// Reset the frequency table that we use to save outgoing data
		memset( frequencyTable, 0, sizeof( unsigned int ) * 256 );

		// Reset the statistical data
		rawBytesSent = rawBytesReceived = compressedBytesSent = compressedBytesReceived = 0;

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

			int error;

			if ( isMainLoopThreadActive == false )
			{
				error = pthread_create( &processPacketsThreadHandle, &attr, &UpdateNetworkLoop, this );

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
// Must be called while offline
// Secures connections though a combination of SHA1, AES128, SYN Cookies, and RSA to prevent
// connection spoofing, replay attacks, data eavesdropping, packet tampering, and MitM attacks.
// There is a significant amount of processing and a slight amount of bandwidth
// overhead for this feature.
//
// If you accept connections, you must call this or else secure connections will not be enabled
// for incoming connections.
// If you are connecting to another system, you can call this with values for the
// (e and p,q) public keys before connecting to prevent MitM
//
// Parameters:
// pubKeyE, pubKeyN - A pointer to the public keys from the RSACrypt class. See the Encryption sample
// privKeyP, privKeyQ - Private keys generated from the RSACrypt class.  See the Encryption sample
// If the private keys are 0, then a new key will be generated when this function is called
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::InitializeSecurity(const char *pubKeyE, const char *pubKeyN, const char *privKeyP, const char *privKeyQ )
{
	if ( endThreads == false )
		return ;

	// Setting the client key is e,n,
	// Setting the server key is p,q
	if ( //( privKeyP && privKeyQ && ( pubKeyE || pubKeyN ) ) ||
		//( pubKeyE && pubKeyN && ( privKeyP || privKeyQ ) ) ||
		( privKeyP && privKeyQ == 0 ) ||
		( privKeyQ && privKeyP == 0 ) ||
		( pubKeyE && pubKeyN == 0 ) ||
		( pubKeyN && pubKeyE == 0 ) )
	{
		// Invalid parameters
		assert( 0 );
	}

	seedMT( RakNet::GetTime() );

	GenerateSYNCookieRandomNumber();

	usingSecurity = true;

	if ( privKeyP == 0 && privKeyQ == 0 && pubKeyE == 0 && pubKeyN == 0 )
	{
		keysLocallyGenerated = true;
		rsacrypt.generateKeys();
	}

	else
	{
		if ( pubKeyE && pubKeyN )
		{
			// Save public keys
			memcpy( ( char* ) & publicKeyE, pubKeyE, sizeof( publicKeyE ) );
			memcpy( publicKeyN, pubKeyN, sizeof( publicKeyN ) );
		}

		if ( privKeyP && privKeyQ )
		{
			BIGHALFSIZE( RSA_BIT_SIZE, p );
			BIGHALFSIZE( RSA_BIT_SIZE, q );
			memcpy( p, privKeyP, sizeof( p ) );
			memcpy( q, privKeyQ, sizeof( q ) );
			// Save private keys
			rsacrypt.setPrivateKey( p, q );
		}

		keysLocallyGenerated = false;
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description
// Must be called while offline
// Disables all security.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DisableSecurity( void )
{
	if ( endThreads == false )
		return ;

	usingSecurity = false;
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
// Sets the password incoming connections must match in the call to Connect (defaults to none)
// Pass 0 to passwordData to specify no password
//
// Parameters:
// passwordData: A data block that incoming connections must match.  This can be just a password, or can be a stream of data.
// - Specify 0 for no password data
// passwordDataLength: The length in bytes of passwordData
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetIncomingPassword( const char* passwordData, int passwordDataLength )
{
	if (passwordDataLength > MAX_OFFLINE_DATA_LENGTH)
		passwordDataLength=MAX_OFFLINE_DATA_LENGTH;

	// Set the incoming password data
	rakPeerMutexes[ incomingPasswordBitStream_Mutex ].Lock();
	incomingPasswordBitStream.Reset();

	if ( passwordData && passwordDataLength > 0 )
		incomingPasswordBitStream.Write( passwordData, passwordDataLength );

	rakPeerMutexes[ incomingPasswordBitStream_Mutex ].Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the password set by SetIncomingPassword in a BitStream
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakNet::BitStream *RakPeer::GetIncomingPassword( void )
{
	return & incomingPasswordBitStream;
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
bool RakPeer::Connect( const char* host, unsigned short remotePort, char* passwordData, int passwordDataLength )
{
	// If endThreads is true here you didn't call Initialize() first.
	if ( host == 0 || endThreads || connectionSocket == INVALID_SOCKET )
		return false;

	unsigned numberOfFreeSlots;

	numberOfFreeSlots = 0;

	/*
	for ( i = 0; i < maximumNumberOfPeers; i++ )
	{
		if ( remoteSystemList[ i ].playerId == UNASSIGNED_PLAYER_ID )
			numberOfFreeSlots++;
	}

	if ( numberOfFreeSlots < (unsigned short)(remoteSystemListSize-maximumNumberOfPeers))
		return false;
		*/

	if (passwordDataLength>MAX_OFFLINE_DATA_LENGTH)
		passwordDataLength=MAX_OFFLINE_DATA_LENGTH;
	// Set the incoming password data
	rakPeerMutexes[ outgoingPasswordBitStream_Mutex ].Lock();
	outgoingPasswordBitStream.Reset();
	if ( passwordData && passwordDataLength > 0 )
		outgoingPasswordBitStream.Write( passwordData, passwordDataLength );
	rakPeerMutexes[ outgoingPasswordBitStream_Mutex ].Unlock();

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
	unsigned i,j;
	bool anyActive;
	unsigned stopWaitingTime;
//	PlayerID playerId;
	unsigned int time;
	unsigned short systemListSize = remoteSystemListSize; // This is done for threading reasons

	for (i=0; i < messageHandlerList.size(); i++)
	{
		messageHandlerList[i]->OnDisconnect(this);
	}

	if ( blockDuration > 0 )
	{
		for ( i = 0; i < systemListSize; i++ )
		{
			NotifyAndFlagForDisconnect(remoteSystemList[i].playerId, false);
		}

		time = RakNet::GetTime();
		stopWaitingTime = time + blockDuration;
		while ( time < stopWaitingTime )
		{
			anyActive=false;
			for (j=0; j < systemListSize; j++)
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
Packet* RakPeer::Receive( void )
{
	if ( !( IsActive() ) )
		return 0;

	Packet *val;

	int offset;
	unsigned int i;
	bool propagate;

	for (i=0; i < messageHandlerList.size(); i++)
	{
		messageHandlerList[i]->OnUpdate(this);
	}

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
				return 0;
			}
			incomingQueueMutex.Unlock();
		}
		else
			return 0;

		// Do RPC calls from the user thread, not the network update thread
		if ( val->data[ 0 ] == ID_RPC )
		{
			// RPC_ASSERT
			assert(0);
			DeallocatePacket( val );
			continue;
		}

		if ( ( val->length >= sizeof(unsigned char) + sizeof( int ) ) &&
			( (unsigned char) val->data[ 0 ] == ID_TIMESTAMP ) )
		{
			offset = sizeof(unsigned char);
			ShiftIncomingTimestamp( ( char* ) val->data + offset, val->playerId );
		}

		propagate=true;
		for (i=0; i < messageHandlerList.size(); i++)
		{
			if (messageHandlerList[i]->OnReceive(this, val))
			{
				// If the message handler returns true, do no further processing
				propagate=false;
				break;
			}

			// Packets that are not propagated to the game are only processed by message handlers
			if (propagate==true && messageHandlerList[i]->PropagateToGame(val)==false)
				propagate=false;
		}

		if (propagate==false)
		{
			DeallocatePacket( val );
			continue;
		}

		break;
	}


#ifdef _DEBUG
	assert( val->data );
#endif

	return val;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Call this to deallocate a packet returned by Receive when you are done handling it.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DeallocatePacket( Packet *packet )
{
	if ( packet == 0 )
		return ;

	packetPool.ReleasePointer( packet );
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
#pragma warning( disable : 4100 ) // warning C4100: 'depreciated' : unreferenced formal parameter
void RakPeer::CloseConnection( PlayerID target, bool sendDisconnectionNotification, int depreciated )
{
	CloseConnectionInternal(target, sendDisconnectionNotification, false);
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
// Bans an IP from connecting. Banned IPs persist between connections.
//
// Parameters
// IP - Dotted IP address.  Can use * as a wildcard, such as 128.0.0.* will ban
// All IP addresses starting with 128.0.0
// milliseconds - how many ms for a temporary ban.  Use 0 for a permanent ban
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::AddToBanList( const char *IP, unsigned int milliseconds )
{
	unsigned index;
	unsigned int time = RakNet::GetTime();

	if ( IP == 0 || IP[ 0 ] == 0 || strlen( IP ) > 15 )
		return ;

	// If this guy is already in the ban list, do nothing
	index = 0;

	banListMutex.Lock();

	for ( ; index < banList.size(); index++ )
	{
		if ( strcmp( IP, banList[ index ]->IP ) == 0 )
		{
			// Already in the ban list.  Just update the time
			if (milliseconds==0)
				banList[ index ]->timeout=0; // Infinite
			else
				banList[ index ]->timeout=time+milliseconds;
			banListMutex.Unlock();
			return;
		}
	}

	banListMutex.Unlock();

	BanStruct *banStruct = new BanStruct;
	banStruct->IP = new char [ 16 ];
	if (milliseconds==0)
		banStruct->timeout=0; // Infinite
	else
		banStruct->timeout=time+milliseconds;
	strcpy( banStruct->IP, IP );
	banListMutex.Lock();
	banList.insert( banStruct );
	banListMutex.Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Allows a previously banned IP to connect.
//
// Parameters
// IP - Dotted IP address.  Can use * as a wildcard, such as 128.0.0.* will ban
// All IP addresses starting with 128.0.0
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::RemoveFromBanList( const char *IP )
{
	unsigned index;
	BanStruct *temp;

	if ( IP == 0 || IP[ 0 ] == 0 || strlen( IP ) > 15 )
		return ;

	index = 0;
	temp=0;

	banListMutex.Lock();

	for ( ; index < banList.size(); index++ )
	{
		if ( strcmp( IP, banList[ index ]->IP ) == 0 )
		{
			temp = banList[ index ];
			banList[ index ] = banList[ banList.size() - 1 ];
			banList.del( banList.size() - 1 );
			break;
		}
	}

	banListMutex.Unlock();

	if (temp)
	{
		delete [] temp->IP;
		delete temp;
	}
	
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Allows all previously banned IPs to connect.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ClearBanList( void )
{
	unsigned index;
	index = 0;
	banListMutex.Lock();

	for ( ; index < banList.size(); index++ )
	{
		delete [] banList[ index ]->IP;
		delete [] banList[ index ];
	}

	banList.clear();

	banListMutex.Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Determines if a particular IP is banned.
//
// Parameters
// IP - Complete dotted IP address
//
// Returns
// True if IP matches any IPs in the ban list, accounting for any wildcards.
// False otherwise.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::IsBanned( const char *IP )
{
	unsigned banListIndex, characterIndex;
	unsigned int time;
	BanStruct *temp;

	if ( IP == 0 || IP[ 0 ] == 0 || strlen( IP ) > 15 )
		return false;

	banListIndex = 0;

	if ( banList.size() == 0 )
		return false; // Skip the mutex if possible

	time = RakNet::GetTime();

	banListMutex.Lock();

	while ( banListIndex < banList.size() )
	{
		if (banList[ banListIndex ]->timeout>0 && banList[ banListIndex ]->timeout<time)
		{
			// Delete expired ban
			temp = banList[ banListIndex ];
			banList[ banListIndex ] = banList[ banList.size() - 1 ];
			banList.del( banList.size() - 1 );
			delete [] temp->IP;
			delete temp;
		}
		else
		{
			characterIndex = 0;

#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
			while ( true )
			{
				if ( banList[ banListIndex ]->IP[ characterIndex ] == IP[ characterIndex ] )
				{
					// Equal characters

					if ( IP[ characterIndex ] == 0 )
					{
						banListMutex.Unlock();
						// End of the string and the strings match

						return true;
					}

					characterIndex++;
				}

				else
				{
					if ( banList[ banListIndex ]->IP[ characterIndex ] == 0 || IP[ characterIndex ] == 0 )
					{
						// End of one of the strings
						break;
					}

					// Characters do not match
					if ( banList[ banListIndex ]->IP[ characterIndex ] == '*' )
					{
						banListMutex.Unlock();

						// Domain is banned.
						return true;
					}

					// Characters do not match and it is not a *
					break;
				}
			}

			banListIndex++;
		}
	}

	banListMutex.Unlock();

	// No match found.
	return false;
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



/*
	RakNet::BitStream bitStream( sizeof(unsigned char) + sizeof(unsigned int) );
	if ( onlyReplyOnAcceptingConnections )
		bitStream.Write((unsigned char)ID_UNCONNECTED_PING_OPEN_CONNECTIONS);
	else
		bitStream.Write((unsigned char)ID_UNCONNECTED_PING);
	bitStream.Write((unsigned int)RakNet::GetTime());

//	SendBuffered(&bitStream, SYSTEM_PRIORITY, UNRELIABLE, 0, playerId, false, RemoteSystemStruct::DISCONNECT_ASAP);

	SocketLayer::Instance()->SendTo( connectionSocket, (const char*)bitStream.GetData(), bitStream.GetNumberOfBytesUsed(), ( char* ) host, remotePort );
	*/
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the average of all ping times read for a specified target
//
// Parameters:
// target - whose time to read
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetAveragePing( PlayerID playerId )
{
	int sum, quantity;
	RemoteSystemStruct *remoteSystem = GetRemoteSystemFromPlayerID( playerId );

	if ( remoteSystem == 0 )
		return -1;

	for ( sum = 0, quantity = 0; quantity < PING_TIMES_ARRAY_SIZE; quantity++ )
	{
		if ( remoteSystem->pingAndClockDifferential[ quantity ].pingTime == -1 )
			break;
		else
			sum += remoteSystem->pingAndClockDifferential[ quantity ].pingTime;
	}

	if ( quantity > 0 )
		return sum / quantity;
	else
		return -1;
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

	if ( remoteSystem->pingAndClockDifferentialWriteIndex == 0 )
		return remoteSystem->pingAndClockDifferential[ PING_TIMES_ARRAY_SIZE - 1 ].pingTime;
	else
		return remoteSystem->pingAndClockDifferential[ remoteSystem->pingAndClockDifferentialWriteIndex - 1 ].pingTime;
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
// Ping the remote systems every so often.  This is off by default
// This will work anytime
//
// Parameters:
// doPing - True to start occasional pings.  False to stop them.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetOccasionalPing( bool doPing )
{
	occasionalPing = doPing;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Automatically synchronizes a block of memory between systems.
// Can be called anytime.  Calling it before a connection is initiated will cause the data to be synchronized on connection
//
// Parameters:
// uniqueIdentifier: an integer (enum) corresponding to the same variable between clients and the server.  Start the indexing at 0
// memoryBlock: Pointer to the data you want to read from or write to
// size: Size of memoryBlock in bytes
// isAuthority: True to tell all connected systems to match their data to yours.  Data changes are relayed to the authoritative
// - client which broadcasts the change
// synchronizationRules: Optional function pointer that decides whether or not to update changed memory.  It should
// - return true if the two passed memory blocks are sufficiently different to synchronize them.  This is an optimization so
// - data that changes rapidly, such as per-frame, can be made to not update every frame
// - The first parameter to synchronizationRules is the new data, the second is the internal copy of the old data
// secondaryUniqueIdentifier:  Optional and used when you have the same unique identifier and is intended for multiple instances of a class
// - that derives from NetworkObject.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
void RakPeer::SynchronizeMemory(UniqueIDType uniqueIdentifier, char *memoryBlock, unsigned short size, bool isAuthority, bool (*synchronizationRules) (char*,char*),ObjectID secondaryUniqueIdentifier)
{
automaticVariableSynchronizationMutex.Lock();
if (uniqueIdentifier >= automaticVariableSynchronizationList.size() || automaticVariableSynchronizationList[uniqueIdentifier]==0)
{
automaticVariableSynchronizationList.replace(new BasicDataStructures::List<MemoryBlock>, 0, uniqueIdentifier);
}
else
{
// If we are using a secondary identifier, make sure that is unique
#ifdef _DEBUG
assert(secondaryUniqueIdentifier!=UNASSIGNED_OBJECT_ID);
#endif
if (secondaryUniqueIdentifier==UNASSIGNED_OBJECT_ID)
{
automaticVariableSynchronizationMutex.Unlock();
return; // Cannot add to an existing list without a secondary identifier
}

for (unsigned i=0; i < automaticVariableSynchronizationList[uniqueIdentifier]->size(); i++)
{
#ifdef _DEBUG
assert ((*(automaticVariableSynchronizationList[uniqueIdentifier]))[i].secondaryID != secondaryUniqueIdentifier);
#endif
if ((*(automaticVariableSynchronizationList[uniqueIdentifier]))[i].secondaryID == secondaryUniqueIdentifier)
{
automaticVariableSynchronizationMutex.Unlock();
return; // Already used
}
}
}
automaticVariableSynchronizationMutex.Unlock();

MemoryBlock newBlock;
newBlock.original=memoryBlock;
if (isAuthority)
{
newBlock.copy = new char[size];
#ifdef _DEBUG
assert(sizeof(char)==1);
#endif
memset(newBlock.copy, 0, size);
}
else
newBlock.copy = 0; // no need to keep a copy if we are only receiving changes
newBlock.size=size;
newBlock.secondaryID=secondaryUniqueIdentifier;
newBlock.isAuthority=isAuthority;
newBlock.synchronizationRules=synchronizationRules;

automaticVariableSynchronizationMutex.Lock();
automaticVariableSynchronizationList[uniqueIdentifier]->insert(newBlock);
automaticVariableSynchronizationMutex.Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Stops synchronization of a block of memory previously defined by uniqueIdentifier and secondaryUniqueIdentifier
// by the call to SynchronizeMemory
// CALL THIS BEFORE SYNCHRONIZED MEMORY IS DEALLOCATED!
// It is not necessary to call this before disconnecting, as all synchronized states will be released then.
// Parameters:
// uniqueIdentifier: an integer (enum) corresponding to the same variable between clients and the server.  Start the indexing at 0
// secondaryUniqueIdentifier:  Optional and used when you have the same unique identifier and is intended for multiple instances of a class
// - that derives from NetworkObject.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DesynchronizeMemory(UniqueIDType uniqueIdentifier, ObjectID secondaryUniqueIdentifier)
{
automaticVariableSynchronizationMutex.Lock();
#ifdef _DEBUG
assert(uniqueIdentifier < automaticVariableSynchronizationList.size());
#endif
if (uniqueIdentifier >= automaticVariableSynchronizationList.size())
{
automaticVariableSynchronizationMutex.Unlock();
return;
}
#ifdef _DEBUG
assert(automaticVariableSynchronizationList[uniqueIdentifier]!=0);
#endif
if (automaticVariableSynchronizationList[uniqueIdentifier]==0)
{
automaticVariableSynchronizationMutex.Unlock();
return;
}

// If we don't specify a secondary identifier, then the list must only have one element
#ifdef _DEBUG
assert(!(secondaryUniqueIdentifier==UNASSIGNED_OBJECT_ID && automaticVariableSynchronizationList[uniqueIdentifier]->size()!=1));
#endif
if (secondaryUniqueIdentifier==UNASSIGNED_OBJECT_ID && automaticVariableSynchronizationList[uniqueIdentifier]->size()!=1)
{
automaticVariableSynchronizationMutex.Unlock();
return;
}

for (unsigned i=0; i < automaticVariableSynchronizationList[uniqueIdentifier]->size(); i++)
{
if ((*(automaticVariableSynchronizationList[uniqueIdentifier]))[i].secondaryID == secondaryUniqueIdentifier)
{
delete [] (*(automaticVariableSynchronizationList[uniqueIdentifier]))[i].copy;
automaticVariableSynchronizationList[uniqueIdentifier]->del(i);
if (automaticVariableSynchronizationList[uniqueIdentifier]->size()==0) // The sublist is now empty
{
delete automaticVariableSynchronizationList[uniqueIdentifier];
automaticVariableSynchronizationList[uniqueIdentifier]=0;
automaticVariableSynchronizationMutex.Unlock();
return;
}
}
}

automaticVariableSynchronizationMutex.Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Desynchronizes all synchronized memory
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DesynchronizeAllMemory(void)
{
int i;

automaticVariableSynchronizationMutex.Lock();
for (i=0; i < (int)automaticVariableSynchronizationList.size(); i++)
{
if (automaticVariableSynchronizationList[i])
{
for (unsigned j=0; j < automaticVariableSynchronizationList[i]->size(); j++)
delete [] (*(automaticVariableSynchronizationList[i]))[j].copy;
delete automaticVariableSynchronizationList[i];
}
}
automaticVariableSynchronizationList.clear();
automaticVariableSynchronizationMutex.Unlock();

synchronizedMemoryQueueMutex.Lock();
while (synchronizedMemoryPacketQueue.size())
{
packetPool.ReleasePointer(synchronizedMemoryPacketQueue.pop());
}
synchronizedMemoryQueueMutex.Unlock();
}
*/ 
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// All systems have a block of data associated with them, for user use.  This block of data can be used to easily
// specify typical system data that you want to know on connection, such as the player's name.
//
// Parameters:
// playerId: Which system you are referring to.  Pass the value returned by GetInternalID to refer to yourself
//
// Returns:
// The data passed to SetRemoteStaticData stored as a bitstream
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakNet::BitStream * RakPeer::GetRemoteStaticData( PlayerID playerId )
{
	if ( playerId == myPlayerId )
		return & localStaticData;

	RemoteSystemStruct *remoteSystem = GetRemoteSystemFromPlayerID( playerId );

	if ( remoteSystem )
		return & ( remoteSystem->staticData );
	else
		return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// All systems have a block of data associated with them, for user use.  This block of data can be used to easily
// specify typical system data that you want to know on connection, such as the player's name.
//
// Parameters:
// playerId: Whose static data to change.  Use your own playerId to change your own static data
// data: a block of data to store
// length: The length of data in bytes
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetRemoteStaticData( PlayerID playerId, const char *data, const long length )
{
	if ( playerId == myPlayerId )
	{
		localStaticData.Reset();

		if ( data && length > 0 )
			localStaticData.Write( data, length );
	}

	else
	{
		RemoteSystemStruct *remoteSystem = GetRemoteSystemFromPlayerID( playerId );

		if ( remoteSystem == 0 )
			return ;

		remoteSystem->staticData.Reset();

		if ( data && length > 0 )
			remoteSystem->staticData.Write( data, length );
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Sends your static data to the specified system.  This is automatically done on connection.
// You should call this when you change your static data.
// To send the static data of another system (such as relaying their data) you should do this normally with Send
//
// Parameters:
// target: Who to send your static data to.  Specify UNASSIGNED_PLAYER_ID to broadcast to all
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SendStaticData( PlayerID target )
{
	SendStaticDataInternal(target, false);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Length should be under 400 bytes, as a security measure against flood attacks
// Sets the data to send with an  (LAN server discovery) /(offline ping) response
// See the Ping sample project for how this is used.
// data: a block of data to store, or 0 for none
// length: The length of data in bytes, or 0 for none
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetOfflinePingResponse( const char *data, const unsigned int length )
{
	assert(length < 400);

	rakPeerMutexes[ offlinePingResponse_Mutex ].Lock();
	offlinePingResponse.Reset();

	if ( data && length > 0 )
		offlinePingResponse.Write( data, length );

	rakPeerMutexes[ offlinePingResponse_Mutex ].Unlock();
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
// Enables or disables our tracking of bytes input to and output from the network.
// This is required to get a frequency table, which is used to generate a new compression layer.
// You can call this at any time - however you SHOULD only call it when disconnected.  Otherwise you will only track
// part of the values sent over the network.
// This value persists between connect calls and defaults to false (no frequency tracking)
//
// Parameters:
// doCompile - true to track bytes.  Defaults to false
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetCompileFrequencyTable( bool doCompile )
{
	trackFrequencyTable = doCompile;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the frequency of outgoing bytes into outputFrequencyTable
// The purpose is to save to file as either a master frequency table from a sample game session for passing to
// GenerateCompressionLayer(false)
// You should only call this when disconnected.
// Requires that you first enable data frequency tracking by calling SetCompileFrequencyTable(true)
//
// Parameters:
// outputFrequencyTable (out): The frequency of each corresponding byte
//
// Returns:
// Ffalse (failure) if connected or if frequency table tracking is not enabled.  Otherwise true (success)
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::GetOutgoingFrequencyTable( unsigned int outputFrequencyTable[ 256 ] )
{
	if ( IsActive() )
		return false;

	if ( trackFrequencyTable == false )
		return false;

	memcpy( outputFrequencyTable, frequencyTable, sizeof( unsigned int ) * 256 );

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Generates the compression layer from the input frequency table.
// You should call this twice - once with inputLayer as true and once as false.
// The frequency table passed here with inputLayer=true should match the frequency table on the recipient with inputLayer=false.
// Likewise, the frequency table passed here with inputLayer=false should match the frequency table on the recipient with inputLayer=true
// Calling this function when there is an existing layer will overwrite the old layer
// You should only call this when disconnected
//
// Parameters:
// inputFrequencyTable: The frequency table returned from GetSendFrequencyTable(...)
// inputLayer - Whether inputFrequencyTable represents incoming data from other systems (true) or outgoing data from this system (false)
//
// Returns:
// False on failure (we are connected).  True otherwise
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer )
{
	if ( IsActive() )
		return false;

	DeleteCompressionLayer( inputLayer );

	if ( inputLayer )
	{
		inputTree = new HuffmanEncodingTree;
		inputTree->GenerateFromFrequencyTable( inputFrequencyTable );
	}

	else
	{
		outputTree = new HuffmanEncodingTree;
		outputTree->GenerateFromFrequencyTable( inputFrequencyTable );
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Deletes the output or input layer as specified.  This is not necessary to call and is only valuable for freeing memory
// You should only call this when disconnected
//
// Parameters:
// inputLayer - Specifies the corresponding compression layer generated by GenerateCompressionLayer.
//
// Returns:
// False on failure (we are connected).  True otherwise
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::DeleteCompressionLayer( bool inputLayer )
{
	if ( IsActive() )
		return false;

	if ( inputLayer )
	{
		if ( inputTree )
		{
			delete inputTree;
			inputTree = 0;
		}
	}

	else
	{
		if ( outputTree )
		{
			delete outputTree;
			outputTree = 0;
		}
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns:
// The compression ratio.  A low compression ratio is good.  Compression is for outgoing data
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float RakPeer::GetCompressionRatio( void ) const
{
	if ( rawBytesSent > 0 )
	{
		return ( float ) compressedBytesSent / ( float ) rawBytesSent;
	}

	else
		return 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns:
// The decompression ratio.  A high decompression ratio is good.  Decompression is for incoming data
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float RakPeer::GetDecompressionRatio( void ) const
{
	if ( rawBytesReceived > 0 )
	{
		return ( float ) compressedBytesReceived / ( float ) rawBytesReceived;
	}

	else
		return 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Attatches a message handler interface to run code automatically on message receipt in the Receive call
// 
// @param messageHandler Pointer to a message handler to attach
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::AttachMessageHandler( MessageHandlerInterface *messageHandler )
{
	if (messageHandlerList.getIndexOf(messageHandler)==MAX_UNSIGNED_LONG)
	{
		messageHandlerList.insert(messageHandler);
		messageHandler->OnAttach(this);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Detatches a message handler interface to run code automatically on message receipt
// 
// @param messageHandler Pointer to a message handler to detatch
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DetachMessageHandler( MessageHandlerInterface *messageHandler )
{
	unsigned int index;
	index = messageHandlerList.getIndexOf(messageHandler);
	if (index!=MAX_UNSIGNED_LONG)
	{
		// Unordered list so delete from end for speed
		messageHandlerList[index]=messageHandlerList[messageHandlerList.size()-1];
		messageHandlerList.del();
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the data you passed to the passwordData parameter in Connect
//
// Parameters
// passwordData (out): Should point to a block large enough to hold the password data you passed to Connect
// passwordDataLength (in, out): Maximum size of the array passwordData.  Modified to hold the number of bytes actually written
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::GetPasswordData( char *passwordData, int *passwordDataLength )
{
	int length;

	if ( incomingPasswordBitStream.GetNumberOfBytesUsed() < *passwordDataLength )
		length = incomingPasswordBitStream.GetNumberOfBytesUsed();
	else
		length = *passwordDataLength;

	memcpy( passwordData, incomingPasswordBitStream.GetData(), length );
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
/*
void RakPeer::RemoveFromRequestedConnectionsList( PlayerID playerId )
{
	int i;
	rakPeerMutexes[ RakPeer::requestedConnections_MUTEX ].Lock();

	for ( i = 0; i < ( int ) requestedConnectionsList.size(); )
	{
		if ( requestedConnectionsList[ i ]->playerId == playerId )
		{
			delete requestedConnectionsList[ i ];
			requestedConnectionsList.del( i );
			break;
		}
	}

	rakPeerMutexes[ RakPeer::requestedConnections_MUTEX ].Unlock();
}
*/

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
		const char *password = data + sizeof(unsigned char);
		int passwordLength = byteSize - sizeof(unsigned char);

		if ( incomingPasswordBitStream.GetNumberOfBytesUsed() == passwordLength &&
			memcmp( password, incomingPasswordBitStream.GetData(), passwordLength ) == 0 )
		{
			remoteSystem->connectMode=RemoteSystemStruct::HANDLING_CONNECTION_REQUEST;

			if ( usingSecurity == false )
			{
#ifdef _TEST_AES
				unsigned char AESKey[ 16 ];
				// Save the AES key
				for ( i = 0; i < 16; i++ )
					AESKey[ i ] = i;

				OnConnectionRequest( remoteSystem, AESKey, true );
#else
				// Connect this player assuming we have open slots
				OnConnectionRequest( remoteSystem, 0, false );
#endif
			}
			else
				SecuredConnectionResponse( playerId );
		}
		else
		{
			// This one we only send once since we don't care if it arrives.
			unsigned char c = ID_INVALID_PASSWORD;
			// SocketLayer::Instance()->SendTo( rakPeer->connectionSocket, ( char* ) & c, sizeof( char ), playerId.binaryAddress, playerId.port );
			SendImmediate(( char* ) & c, sizeof( char )*8, SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, false, RakNet::GetTime());
			remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
		}
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::OnConnectionRequest( RakPeer::RemoteSystemStruct *remoteSystem, unsigned char *AESKey, bool setAESKey )
{
	if ( AllowIncomingConnections() )
	{
#ifdef __USE_IO_COMPLETION_PORTS
		unsigned index;

		for ( index = 0; index < remoteSystemListSize; index++ )
			if ( remoteSystemList + index == remoteSystem )
				break;

		if ( SetupIOCompletionPortSocket( index ) == false )
		{
			// Socket error
			assert( 0 );
			return ;
		}
#endif

		RakNet::BitStream bitStream(sizeof(unsigned char)+sizeof(unsigned short)+sizeof(unsigned int)+sizeof(unsigned short)+sizeof(PlayerIndex));
		bitStream.Write((unsigned char)ID_CONNECTION_REQUEST_ACCEPTED);
#ifdef __USE_IO_COMPLETION_PORTS
		bitStream.Write((unsigned short)myPlayerId.port + ( unsigned short ) index + ( unsigned short ) 1);
#else
		bitStream.Write((unsigned short)myPlayerId.port);
#endif
		bitStream.Write(remoteSystem->playerId.binaryAddress);
		bitStream.Write(remoteSystem->playerId.port);
		bitStream.Write(( PlayerIndex ) GetIndexFromPlayerID( remoteSystem->playerId ));


/*
		ConnectionAcceptStruct ds;
		ds.typeId = ID_CONNECTION_REQUEST_ACCEPTED;

#ifdef __USE_IO_COMPLETION_PORTS
		ds.remotePort = myPlayerId.port + ( unsigned short ) index + ( unsigned short ) 1;
#else
		ds.remotePort = myPlayerId.port;
#endif

		ds.externalID = remoteSystem->playerId;
		ds.playerIndex = ( PlayerIndex ) GetIndexFromPlayerID( remoteSystem->playerId );

		RakNet::BitStream dsBitS( ConnectionAcceptStruct_Size );
		ds.Serialize( dsBitS );
		*/

		SendImmediate((char*)bitStream.GetData(), bitStream.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, remoteSystem->playerId, false, false, RakNet::GetTime());

		// Don't set secure connections immediately because we need the ack from the remote system to know ID_CONNECTION_REQUEST_ACCEPTED
		// As soon as a 16 byte packet arrives, we will turn on AES.  This works because all encrypted packets are multiples of 16 and the
		// packets I happen to be sending are less than 16 bytes
		remoteSystem->setAESKey=setAESKey;
		if ( setAESKey )
		{
			memcpy(remoteSystem->AESKey, AESKey, 16);
			remoteSystem->connectMode=RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET;
		}
	}
	else
	{
		unsigned char c = ID_NO_FREE_INCOMING_CONNECTIONS;
		//SocketLayer::Instance()->SendTo( connectionSocket, ( char* ) & c, sizeof( char ), playerId.binaryAddress, playerId.port );

		SendImmediate((char*)&c, sizeof(c)*8, SYSTEM_PRIORITY, RELIABLE, 0, remoteSystem->playerId, false, false, RakNet::GetTime());
		remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::NotifyAndFlagForDisconnect( PlayerID playerId, bool performImmediate )
{
	RakNet::BitStream temp( sizeof(unsigned char) + outgoingPasswordBitStream.GetNumberOfBytesUsed() );
	temp.Write( (unsigned char) ID_DISCONNECTION_NOTIFICATION );
	if ( outgoingPasswordBitStream.GetNumberOfBytesUsed() > 0 )
		temp.Write( ( char* ) outgoingPasswordBitStream.GetData(), outgoingPasswordBitStream.GetNumberOfBytesUsed() );
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
	unsigned i,j;
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
			remoteSystem->reliabilityLayer.SetEncryptionKey( 0 );

			for ( j = 0; j < PING_TIMES_ARRAY_SIZE; j++ )
			{
				remoteSystem->pingAndClockDifferential[ j ].pingTime = -1;
				remoteSystem->pingAndClockDifferential[ j ].clockDifferential = 0;
			}

			remoteSystem->connectMode=connectionMode;
			remoteSystem->pingAndClockDifferentialWriteIndex = 0;
			remoteSystem->lowestPing = -1;
			remoteSystem->nextPingTime = 0; // Ping immediately
			remoteSystem->weInitiatedTheConnection = false;
			remoteSystem->staticData.Reset();
			remoteSystem->connectionTime = time;
			remoteSystem->myExternalPlayerId = UNASSIGNED_PLAYER_ID;
			remoteSystem->setAESKey=false;
			remoteSystem->lastReliableSend=time;

			// Reserve this reliability layer for ourselves.
			remoteSystem->reliabilityLayer.Reset();

			return remoteSystem;
		}
	}

	return remoteSystem;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Adjust the first four bytes (treated as unsigned int) of the pointer
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ShiftIncomingTimestamp( char *data, PlayerID playerId ) const
{
#ifdef _DEBUG
	assert( IsActive() );
	assert( data );
#endif

	RakNet::BitStream timeBS(data, 4, false);
	unsigned int encodedTimestamp;
	timeBS.Read(encodedTimestamp);

	encodedTimestamp = encodedTimestamp - GetBestClockDifferential( playerId );
	timeBS.SetWriteOffset(0);
	timeBS.Write(encodedTimestamp);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Thanks to Chris Taylor (cat02e@fsu.edu) for the improved timestamping algorithm
unsigned int RakPeer::GetBestClockDifferential( PlayerID playerId ) const
{
	int counter, clockDifferential, lowestPingSoFar;
	RemoteSystemStruct *remoteSystem = GetRemoteSystemFromPlayerID( playerId );

	if ( remoteSystem == 0 )
		return 0;

	lowestPingSoFar = 65535;

	clockDifferential = 0;

	for ( counter = 0; counter < PING_TIMES_ARRAY_SIZE; counter++ )
	{
		if ( remoteSystem->pingAndClockDifferential[ counter ].pingTime == -1 )
			break;

		if ( remoteSystem->pingAndClockDifferential[ counter ].pingTime < lowestPingSoFar )
		{
			clockDifferential = remoteSystem->pingAndClockDifferential[ counter ].clockDifferential;
			lowestPingSoFar = remoteSystem->pingAndClockDifferential[ counter ].pingTime;
		}
	}

	return clockDifferential;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef __USE_IO_COMPLETION_PORTS
bool RakPeer::SetupIOCompletionPortSocket( int index )
{
	SOCKET newSocket;

	if ( remoteSystemList[ index ].reliabilityLayer.GetSocket() != INVALID_SOCKET )
		closesocket( remoteSystemList[ index ].reliabilityLayer.GetSocket() );

	newSocket = SocketLayer::Instance()->CreateBoundSocket( myPlayerId.port + index + 1, false );

	SocketLayer::Instance()->Connect( newSocket, remoteSystemList[ index ].playerId.binaryAddress, remoteSystemList[ index ].playerId.port ); // port is the port of the client

	remoteSystemList[ index ].reliabilityLayer.SetSocket( newSocket );

	// Associate our new socket with a completion port and do the first read
	return SocketLayer::Instance()->AssociateSocketWithCompletionPortAndRead( newSocket, remoteSystemList[ index ].playerId.binaryAddress, remoteSystemList[ index ].playerId.port, this );
}

#endif

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::GenerateSYNCookieRandomNumber( void )
{
	unsigned int number;
	int i;
	memcpy( oldRandomNumber, newRandomNumber, sizeof( newRandomNumber ) );

	for ( i = 0; i < sizeof( newRandomNumber ); i += sizeof( number ) )
	{
		number = randomMT();
		memcpy( newRandomNumber + i, ( char* ) & number, sizeof( number ) );
	}

	randomNumberExpirationTime = RakNet::GetTime() + SYN_COOKIE_OLD_RANDOM_NUMBER_DURATION;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SecuredConnectionResponse( PlayerID playerId )
{
	CSHA1 sha1;
	RSA_BIT_SIZE n;
	big::u32 e;
	unsigned char connectionRequestResponse[ 1 + sizeof( big::u32 ) + sizeof( RSA_BIT_SIZE ) + 20 ];
	connectionRequestResponse[ 0 ] = ID_SECURED_CONNECTION_RESPONSE;

	// Hash the SYN-Cookie
	// s2c syn-cookie = SHA1_HASH(source ip address + source port + random number)
	sha1.Reset();
	sha1.Update( ( unsigned char* ) & playerId.binaryAddress, sizeof( playerId.binaryAddress ) );
	sha1.Update( ( unsigned char* ) & playerId.port, sizeof( playerId.port ) );
	sha1.Update( ( unsigned char* ) & ( newRandomNumber ), 20 );
	sha1.Final();

	// Write the cookie
	memcpy( connectionRequestResponse + 1, sha1.GetHash(), 20 );

	// Write the public keys
	rsacrypt.getPublicKey( e, n );
#ifdef HOST_ENDIAN_IS_BIG
	// Mangle the keys on a Big-endian machine before sending
	BSWAPCPY( (unsigned char *)(connectionRequestResponse + 1 + 20),
		(unsigned char *)&e, sizeof( big::u32 ) );
	BSWAPCPY( (unsigned char *)(connectionRequestResponse + 1 + 20 + sizeof( big::u32 ) ),
		(unsigned char *)n, sizeof( RSA_BIT_SIZE ) );
#else
	memcpy( connectionRequestResponse + 1 + 20, ( char* ) & e, sizeof( big::u32 ) );
	memcpy( connectionRequestResponse + 1 + 20 + sizeof( big::u32 ), n, sizeof( RSA_BIT_SIZE ) );
#endif

	// s2c public key, syn-cookie
	//SocketLayer::Instance()->SendTo( connectionSocket, ( char* ) connectionRequestResponse, 1 + sizeof( big::u32 ) + sizeof( RSA_BIT_SIZE ) + 20, playerId.binaryAddress, playerId.port );
	// All secure connection requests are unreliable because the entire process needs to be restarted if any part fails.
	// Connection requests are resent periodically
	SendImmediate(( char* ) connectionRequestResponse, (1 + sizeof( big::u32 ) + sizeof( RSA_BIT_SIZE ) + 20) *8, SYSTEM_PRIORITY, UNRELIABLE, 0, playerId, false, false, RakNet::GetTime());
}

void RakPeer::SecuredConnectionConfirmation( RakPeer::RemoteSystemStruct * remoteSystem, char* data )
{
	int i, j;
	unsigned char randomNumber[ 20 ];
	unsigned int number;
	//bool doSend;
	Packet *packet;
	big::u32 e;
	RSA_BIT_SIZE n, message, encryptedMessage;
	big::RSACrypt<RSA_BIT_SIZE> privKeyPncrypt;

	// Make sure that we still want to connect
	if (remoteSystem->connectMode!=RemoteSystemStruct::REQUESTED_CONNECTION)
		return;

/*
	// Make sure that we still want to connect
	bool requestedConnection = false;
	
	rakPeerMutexes[ RakPeer::requestedConnections_MUTEX ].Lock();

	for ( i = 0; i < ( int ) requestedConnectionsList.size();i++ )
	{
		if ( requestedConnectionsList[ i ]->playerId == playerId )
		{
			// We did request this connection
			requestedConnection = true;
			break;
		}
	}

	rakPeerMutexes[ RakPeer::requestedConnections_MUTEX ].Unlock();

	if ( requestedConnection == false )
		return ; // Don't want to connect
		

	doSend = false;
*/

	// Copy out e and n
#ifdef HOST_ENDIAN_IS_BIG
	BSWAPCPY( (unsigned char *)&e, (unsigned char *)(data + 1 + 20), sizeof( big::u32 ) );
	BSWAPCPY( (unsigned char *)n, (unsigned char *)(data + 1 + 20 + sizeof( big::u32 )), sizeof( RSA_BIT_SIZE ) );
#else
	memcpy( ( char* ) & e, data + 1 + 20, sizeof( big::u32 ) );
	memcpy( n, data + 1 + 20 + sizeof( big::u32 ), sizeof( RSA_BIT_SIZE ) );
#endif

	// If we preset a size and it doesn't match, or the keys do not match, then tell the user
	if ( usingSecurity == true && keysLocallyGenerated == false )
	{
		if ( memcmp( ( char* ) & e, ( char* ) & publicKeyE, sizeof( big::u32 ) ) != 0 ||
			memcmp( n, publicKeyN, sizeof( RSA_BIT_SIZE ) ) != 0 )
		{
			packet = packetPool.GetPointer();
			packet->data = new unsigned char[ 1 ];
			packet->data[ 0 ] = ID_RSA_PUBLIC_KEY_MISMATCH;
			packet->length = sizeof( char );
			packet->bitSize = sizeof( char ) * 8;
			packet->playerId = remoteSystem->playerId;
			packet->playerIndex = ( PlayerIndex ) GetIndexFromPlayerID( packet->playerId );
			incomingQueueMutex.Lock();
			incomingPacketQueue.push( packet );
			incomingQueueMutex.Unlock();
			remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
			return;
		}
	}

	// Create a random number
	for ( i = 0; i < sizeof( randomNumber ); i += sizeof( number ) )
	{
		number = randomMT();
		memcpy( randomNumber + i, ( char* ) & number, sizeof( number ) );
	}

	memset( message, 0, sizeof( message ) );
	assert( sizeof( message ) >= sizeof( randomNumber ) );

#ifdef HOST_ENDIAN_IS_BIG
	// Scramble the plaintext message
	BSWAPCPY( (unsigned char *)message, randomNumber, sizeof(randomNumber) );
#else
	memcpy( message, randomNumber, sizeof( randomNumber ) );
#endif
	privKeyPncrypt.setPublicKey( e, n );
	privKeyPncrypt.encrypt( message, encryptedMessage );
#ifdef HOST_ENDIAN_IS_BIG
	// A big-endian machine needs to scramble the byte order of an outgoing (encrypted) message
	BSWAPSELF( (unsigned char *)encryptedMessage, sizeof( RSA_BIT_SIZE ) );
#endif

	/*
	rakPeerMutexes[ RakPeer::requestedConnections_MUTEX ].Lock();
	for ( i = 0; i < ( int ) requestedConnectionsList.size(); i++ )
	{
		if ( requestedConnectionsList[ i ]->playerId == playerId )
		{
			doSend = true;
			// Generate the AES key

			for ( j = 0; j < 16; j++ )
				requestedConnectionsList[ i ]->AESKey[ j ] = data[ 1 + j ] ^ randomNumber[ j ];

			requestedConnectionsList[ i ]->setAESKey = true;

			break;
		}
	}
	rakPeerMutexes[ RakPeer::requestedConnections_MUTEX ].Unlock();
	*/

	// Take the remote system's AESKey and XOR with our random number.
		for ( j = 0; j < 16; j++ )
			remoteSystem->AESKey[ j ] = data[ 1 + j ] ^ randomNumber[ j ];
	remoteSystem->setAESKey = true;

//	if ( doSend )
//	{
		char reply[ 1 + 20 + sizeof( RSA_BIT_SIZE ) ];
		// c2s RSA(random number), same syn-cookie
		reply[ 0 ] = ID_SECURED_CONNECTION_CONFIRMATION;
		memcpy( reply + 1, data + 1, 20 );  // Copy the syn-cookie
		memcpy( reply + 1 + 20, encryptedMessage, sizeof( RSA_BIT_SIZE ) ); // Copy the encoded random number

		//SocketLayer::Instance()->SendTo( connectionSocket, reply, 1 + 20 + sizeof( RSA_BIT_SIZE ), playerId.binaryAddress, playerId.port );
		// All secure connection requests are unreliable because the entire process needs to be restarted if any part fails.
		// Connection requests are resent periodically
		SendImmediate((char*)reply, (1 + 20 + sizeof( RSA_BIT_SIZE )) * 8, SYSTEM_PRIORITY, UNRELIABLE, 0, remoteSystem->playerId, false, false, RakNet::GetTime());
//	}
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
/*
RakPeer::BufferedCommandStruct *RakPeer::GetBufferedCommandStruct(void)
{
	BufferedCommandStruct *bcs;
	rakPeerMutexes[ RakPeer::bufferedCommandPool_Mutex ].Lock();
	if (bufferedCommandPool.size())
	{
		bcs = bufferedCommandPool.pop();
		rakPeerMutexes[ RakPeer::bufferedCommandPool_Mutex ].Unlock();
	}
	else
	{
		rakPeerMutexes[ RakPeer::bufferedCommandPool_Mutex ].Unlock();
		bcs = new BufferedCommandStruct;
	}
	return bcs;
}
*/
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::AllowIncomingConnections(void) const
{
	return GetNumberOfRemoteInitiatedConnections() < GetMaximumIncomingConnections();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SendStaticDataInternal( PlayerID target, bool performImmediate )
{
	RakNet::BitStream reply( sizeof(unsigned char) + localStaticData.GetNumberOfBytesUsed() );
	reply.Write( (unsigned char) ID_RECEIVED_STATIC_DATA );
	reply.Write( ( char* ) localStaticData.GetData(), localStaticData.GetNumberOfBytesUsed() );

	if (performImmediate)
	{
		if ( target == UNASSIGNED_PLAYER_ID )
			SendImmediate( (char*)reply.GetData(), reply.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, target, true, false, RakNet::GetTime() );
		else
			SendImmediate( (char*)reply.GetData(), reply.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, target, false, false, RakNet::GetTime() );
	}
	else
	{
		if ( target == UNASSIGNED_PLAYER_ID )
			Send( &reply, SYSTEM_PRIORITY, RELIABLE, 0, target, true );
		else
			Send( &reply, SYSTEM_PRIORITY, RELIABLE, 0, target, false );
	}
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
void RakPeer::CloseConnectionInternal( PlayerID target, bool sendDisconnectionNotification, bool performImmediate )
{
	unsigned i;

	if ( remoteSystemList == 0 || endThreads == true )
		return;

	if (sendDisconnectionNotification)
	{
		NotifyAndFlagForDisconnect(target, performImmediate);
	}
	else
	{
		if (performImmediate)
		{
			i = 0;
			for ( ; i < remoteSystemListSize; i++ )
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
		else
		{
			BufferedCommandStruct *bcs;
			//bcs=GetBufferedCommandStruct();
			bcs=bufferedCommands.WriteLock();
			bcs->command=BufferedCommandStruct::BCS_CLOSE_CONNECTION;
			bcs->playerId=target;
			bcs->data=0;
//			rakPeerMutexes[ RakPeer::bufferedCommandQueue_Mutex ].Lock();
//			bufferedCommandQueue.push(bcs);
//			rakPeerMutexes[ RakPeer::bufferedCommandQueue_Mutex ].Unlock();
			bufferedCommands.WriteUnlock();
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
	/*
	rakPeerMutexes[ RakPeer::bufferedCommandPool_Mutex ].Lock();
	if (bufferedCommandPool.size())
	{
		bcs = bufferedCommandPool.pop();
		rakPeerMutexes[ RakPeer::bufferedCommandPool_Mutex ].Unlock();
	}
	else
	{
		rakPeerMutexes[ RakPeer::bufferedCommandPool_Mutex ].Unlock();
		bcs = new BufferedCommandStruct;
	}
	*/

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
	/*
	rakPeerMutexes[ RakPeer::bufferedCommandQueue_Mutex ].Lock();
	bufferedCommandQueue.push(bcs);
	rakPeerMutexes[ RakPeer::bufferedCommandQueue_Mutex ].Unlock();
	*/
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::SendImmediate( char *data, int numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool useCallerDataAllocation, unsigned int currentTime )
{
	unsigned *sendList;
	unsigned sendListSize;
	bool callerDataAllocationUsed;
	unsigned remoteSystemIndex, sendListIndex; // Iterates into the list of remote systems
	unsigned numberOfBytesUsed = BITS_TO_BYTES(numberOfBitsToSend);
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
		if ( trackFrequencyTable )
		{
			unsigned i;
			// Store output frequency
			for (i=0 ; i < numberOfBytesUsed; i++ )
				frequencyTable[ (unsigned char)(data[i]) ]++;
			rawBytesSent += numberOfBytesUsed;
		}

		if ( outputTree )
		{
			RakNet::BitStream bitStreamCopy( numberOfBytesUsed );
			outputTree->EncodeArray( (unsigned char*) data, numberOfBytesUsed, &bitStreamCopy );
			compressedBytesSent += bitStreamCopy.GetNumberOfBytesUsed();
			remoteSystemList[sendList[sendListIndex]].reliabilityLayer.Send( (char*) bitStreamCopy.GetData(), bitStreamCopy.GetNumberOfBitsUsed(), priority, reliability, orderingChannel, true, MTUSize, currentTime );
		}
		else
		{
			// Send may split the packet and thus deallocate data.  Don't assume data is valid if we use the callerAllocationData
			bool useData = useCallerDataAllocation && callerDataAllocationUsed==false && sendListIndex+1==sendListSize;
			remoteSystemList[sendList[sendListIndex]].reliabilityLayer.Send( data, numberOfBitsToSend, priority, reliability, orderingChannel, useData==false, MTUSize, currentTime );
			if (useData)
				callerDataAllocationUsed=true;
		}

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
/*
#ifdef _WIN32
unsigned __stdcall RecvFromNetworkLoop(LPVOID arguments)
#else
void*  RecvFromNetworkLoop( void*  arguments )
#endif
{
RakPeer *peer = (RakPeer *)arguments;
unsigned int errorCode;

#ifdef __USE_IO_COMPLETION_PORTS
AsynchronousFileIO::Instance()->IncreaseUserCount();
#endif

peer->isRecvfromThreadActive=true;

while(peer->endThreads==false)
{
peer->isSocketLayerBlocking=true;
errorCode=SocketLayer::Instance()->RecvFrom(peer->connectionSocket, peer);
peer->isSocketLayerBlocking=false;

#ifdef _WIN32
if (errorCode==WSAECONNRESET)
{
peer->PushPortRefused(UNASSIGNED_PLAYER_ID);
//closesocket(peer->connectionSocket);
//peer->connectionSocket = SocketLayer::Instance()->CreateBoundSocket(peer->myPlayerId.port, true);
}
else if (errorCode!=0 && peer->endThreads==false)
{
#ifdef _DEBUG
printf("Server RecvFrom critical failure!\n");
#endif
// Some kind of critical error
peer->isRecvfromThreadActive=false;
peer->endThreads=true;
peer->Disconnect();
break;
}
#else
if (errorCode==-1)
{
peer->isRecvfromThreadActive=false;
peer->endThreads=true;
peer->Disconnect();
break;
}
#endif
}

#ifdef __USE_IO_COMPLETION_PORTS
AsynchronousFileIO::Instance()->DecreaseUserCount();
#endif

peer->isRecvfromThreadActive=false;

#ifdef _WIN32
//_endthreadex( 0 );
#endif
return 0;
}
*/ 
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#ifdef _WIN32
void __stdcall ProcessPortUnreachable( unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer )
#else
void ProcessPortUnreachable( unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer )
#endif
{
	// TODO - figure out how the hell to get the values for binaryAddress and port

	/*
	RakPeer::RemoteSystemStruct *remoteSystem;
	PlayerID playerId;
	playerId.binaryAddress = binaryAddress;
	playerId.port = port;
	remoteSystem = rakPeer->GetRemoteSystemFromPlayerID( playerId );
	if (remoteSystem)
		remoteSystem->reliabilityLayer.KillConnection();
		*/
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

	if (rakPeer->IsBanned( rakPeer->PlayerIDToDottedIP( playerId ) ))
		return;

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

					RakNet::BitStream temp( sizeof(unsigned char) + rakPeer->outgoingPasswordBitStream.GetNumberOfBytesUsed() );
					temp.Write( (unsigned char) ID_CONNECTION_REQUEST );
					if ( rakPeer->outgoingPasswordBitStream.GetNumberOfBytesUsed() > 0 )
						temp.Write( ( char* ) rakPeer->outgoingPasswordBitStream.GetData(), rakPeer->outgoingPasswordBitStream.GetNumberOfBytesUsed() );
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
		if (remoteSystem->connectMode==RakPeer::RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET && (length%16)==0)
			remoteSystem->reliabilityLayer.SetEncryptionKey( remoteSystem->AESKey );

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
		if (length > 512)
		{
			// Flood attack?  Unknown systems should never send more than a small amount of data
			rakPeer->AddToBanList(rakPeer->PlayerIDToDottedIP(playerId), TIMEOUT_TIME);
			return;
		}

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
	unsigned numberOfBitsUsed;
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
	while ((bcs=bufferedCommands.ReadLock())!=0) // Don't immediately check mutex since it's so slow to activate it
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
			CloseConnectionInternal(bcs->playerId, false, true);
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

					packet->data = new unsigned char [ sizeof( char ) + remoteSystem->staticData.GetNumberOfBytesUsed() ];
					if (remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION)
						packet->data[ 0 ] = ID_CONNECTION_ATTEMPT_FAILED; // Attempted a connection and couldn't
					else
						packet->data[ 0 ] = ID_CONNECTION_LOST; // DeadConnection
					memcpy( packet->data + sizeof( char ), remoteSystem->staticData.GetData(), remoteSystem->staticData.GetNumberOfBytesUsed() );

					packet->length = sizeof( char ) + remoteSystem->staticData.GetNumberOfBytesUsed();
					packet->bitSize = ( sizeof( char ) + remoteSystem->staticData.GetNumberOfBytesUsed() ) * 8;
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
				CloseConnectionInternal( playerId, false, true );
				continue;
			}

			// Did the reliability layer detect a modified packet?
			if ( remoteSystem->reliabilityLayer.IsCheater() )
			{
				packet = packetPool.GetPointer();
				packet->length = 1;
				packet->data = new unsigned char [ 1 ];
				packet->data[ 0 ] = (unsigned char) ID_MODIFIED_PACKET;
				packet->playerId = playerId;
				packet->playerIndex = ( PlayerIndex ) remoteSystemIndex;

				incomingQueueMutex.Lock();
				( incomingPacketQueue ).push( packet );
				incomingQueueMutex.Unlock();

				continue;
			}

			// Ping this guy if it is time to do so
			if ( remoteSystem->connectMode==RemoteSystemStruct::CONNECTED && time > remoteSystem->nextPingTime && ( occasionalPing || remoteSystem->lowestPing == -1 ) )
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
				// Put the input through compression if necessary
				if ( inputTree )
				{
					RakNet::BitStream dataBitStream( MAXIMUM_MTU_SIZE );
					// Since we are decompressing input, we need to copy to a bitstream, decompress, then copy back to a probably
					// larger data block.  It's slow, but the user should have known that anyway
					dataBitStream.Reset();
					dataBitStream.WriteAlignedBytes( ( unsigned char* ) data, BITS_TO_BYTES( bitSize ) );
					rawBytesReceived += dataBitStream.GetNumberOfBytesUsed();

//					numberOfBytesUsed = dataBitStream.GetNumberOfBytesUsed();
					numberOfBitsUsed = dataBitStream.GetNumberOfBitsUsed();
					//rawBytesReceived += numberOfBytesUsed;
					// Decompress the input data.

					if (numberOfBitsUsed>0)
					{
						unsigned char *dataCopy = new unsigned char[ dataBitStream.GetNumberOfBytesUsed() ];
						memcpy( dataCopy, dataBitStream.GetData(), dataBitStream.GetNumberOfBytesUsed() );
						dataBitStream.Reset();
						inputTree->DecodeArray( dataCopy, numberOfBitsUsed, &dataBitStream );
						compressedBytesReceived += dataBitStream.GetNumberOfBytesUsed();
						delete [] dataCopy;

						byteSize = dataBitStream.GetNumberOfBytesUsed();

						if ( byteSize > BITS_TO_BYTES( bitSize ) )   // Probably the case - otherwise why decompress?
						{
							delete [] data;
							data = new char [ byteSize ];
						}
						memcpy( data, dataBitStream.GetData(), byteSize );
					}
					else
						byteSize=0;
				}
				else
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
							//tempBitStream.Write( data, UnconnectedPingStruct_Size );
							rakPeerMutexes[ RakPeer::offlinePingResponse_Mutex ].Lock();
							outBitStream.Write( ( char* ) offlinePingResponse.GetData(), offlinePingResponse.GetNumberOfBytesUsed() );
							rakPeerMutexes[ RakPeer::offlinePingResponse_Mutex ].Unlock();
							//SocketLayer::Instance()->SendTo( connectionSocket, ( char* ) outBitStream.GetData(), outBitStream.GetNumberOfBytesUsed(), playerId.binaryAddress, playerId.port );
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
						CloseConnectionInternal( playerId, false, true );
#ifdef _DO_PRINTF
						printf("Temporarily banning %i:%i for sending nonsense data\n", playerId.binaryAddress, playerId.port);
#endif
						AddToBanList(PlayerIDToDottedIP(playerId), TIMEOUT_TIME);
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
							remoteSystem->connectMode==RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET ||
							playerId==myPlayerId) // local system connect
						{
							remoteSystem->connectMode=RemoteSystemStruct::CONNECTED;
							PingInternal( playerId, true );
							SendStaticDataInternal( playerId, true );

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
						lastPing = remoteSystem->pingAndClockDifferential[ remoteSystem->pingAndClockDifferentialWriteIndex ].pingTime;

						// Ignore super high spikes in the average
						if ( lastPing <= 0 || ( ( ( int ) ping < ( lastPing * 3 ) ) && ping < 1200 ) )
						{
							remoteSystem->pingAndClockDifferential[ remoteSystem->pingAndClockDifferentialWriteIndex ].pingTime = ( short ) ping;
							// Thanks to Chris Taylor (cat02e@fsu.edu) for the improved timestamping algorithm
							remoteSystem->pingAndClockDifferential[ remoteSystem->pingAndClockDifferentialWriteIndex ].clockDifferential = sendPongTime - ( time + sendPingTime ) / 2;

							if ( remoteSystem->lowestPing == -1 || remoteSystem->lowestPing > ping )
								remoteSystem->lowestPing = ping;

							// Most packets should arrive by the ping time.
							remoteSystem->reliabilityLayer.SetLostPacketResendDelay( ping * 2 );

							if ( ++( remoteSystem->pingAndClockDifferentialWriteIndex ) == PING_TIMES_ARRAY_SIZE )
								remoteSystem->pingAndClockDifferentialWriteIndex = 0;
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

						if ( remoteSystem->staticData.GetNumberOfBytesUsed() > 0 )
						{
							packet->data = new unsigned char [ sizeof( char ) + remoteSystem->staticData.GetNumberOfBytesUsed() ];
							packet->data[ 0 ] = ID_DISCONNECTION_NOTIFICATION;
							memcpy( packet->data + sizeof( char ), remoteSystem->staticData.GetData(), remoteSystem->staticData.GetNumberOfBytesUsed() );

							packet->length = sizeof( char ) + remoteSystem->staticData.GetNumberOfBytesUsed();
							packet->bitSize = sizeof( char ) * 8 + remoteSystem->staticData.GetNumberOfBitsUsed();

							delete [] data;
						}
						else
						{
							packet->data = ( unsigned char* ) data;
							packet->bitSize = 8;
							packet->length = 1;
						}

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
					else if ( (unsigned char) data[ 0 ] == ID_RPC_MAPPING )
					{
						/// RPC ASSERT
						assert( 0 );
						delete [] data;
					}
					else if ( (unsigned char) data[ 0 ] == ID_REQUEST_STATIC_DATA )
					{
						SendStaticDataInternal( playerId, true );
						delete [] data;
					}
					else if ( (unsigned char) data[ 0 ] == ID_RECEIVED_STATIC_DATA )
					{
						remoteSystem->staticData.Reset();
						remoteSystem->staticData.Write( ( char* ) data + sizeof(unsigned char), byteSize - 1 );

						// Inform game server code that we got static data
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
					else if ( (unsigned char)(data)[0] == ID_SECURED_CONNECTION_RESPONSE &&
						byteSize == 1 + sizeof( big::u32 ) + sizeof( RSA_BIT_SIZE ) + 20 )
					{
						SecuredConnectionConfirmation( remoteSystem, data );
						delete [] data;
					}
					else if ( (unsigned char)(data)[0] == ID_SECURED_CONNECTION_CONFIRMATION &&
						byteSize == 1 + 20 + sizeof( RSA_BIT_SIZE ) )
					{
						CSHA1 sha1;
						bool confirmedHash, newRandNumber;

						confirmedHash = false;

						// Hash the SYN-Cookie
						// s2c syn-cookie = SHA1_HASH(source ip address + source port + random number)
						sha1.Reset();
						sha1.Update( ( unsigned char* ) & playerId.binaryAddress, sizeof( playerId.binaryAddress ) );
						sha1.Update( ( unsigned char* ) & playerId.port, sizeof( playerId.port ) );
						sha1.Update( ( unsigned char* ) & ( newRandomNumber ), 20 );
						sha1.Final();

						newRandNumber = false;

						// Confirm if
						//syn-cookie ?= HASH(source ip address + source port + last random number)
						//syn-cookie ?= HASH(source ip address + source port + current random number)
						if ( memcmp( sha1.GetHash(), data + 1, 20 ) == 0 )
						{
							confirmedHash = true;
							newRandNumber = true;
						}
						else if ( randomNumberExpirationTime < RakNet::GetTime() )
						{
							sha1.Reset();
							sha1.Update( ( unsigned char* ) & playerId.binaryAddress, sizeof( playerId.binaryAddress ) );
							sha1.Update( ( unsigned char* ) & playerId.port, sizeof( playerId.port ) );
							sha1.Update( ( unsigned char* ) & ( oldRandomNumber ), 20 );
							sha1.Final();

							if ( memcmp( sha1.GetHash(), data + 1, 20 ) == 0 )
								confirmedHash = true;
						}
						if ( confirmedHash )
						{
							int i;
							unsigned char AESKey[ 16 ];
							RSA_BIT_SIZE message, encryptedMessage;

							// On connection accept, AES key is c2s RSA_Decrypt(random number) XOR s2c syn-cookie
							// Get the random number first
							#ifdef HOST_ENDIAN_IS_BIG
								BSWAPCPY( (unsigned char *) encryptedMessage, (unsigned char *)(data + 1 + 20), sizeof( RSA_BIT_SIZE ) );
							#else
								memcpy( encryptedMessage, data + 1 + 20, sizeof( RSA_BIT_SIZE ) );
							#endif
							rsacrypt.decrypt( encryptedMessage, message );
							#ifdef HOST_ENDIAN_IS_BIG 
								BSWAPSELF( (unsigned char *) message, sizeof( RSA_BIT_SIZE ) );
							#endif

							// Save the AES key
							for ( i = 0; i < 16; i++ )
								AESKey[ i ] = data[ 1 + i ] ^ ( ( unsigned char* ) ( message ) ) [ i ];

							// Connect this player assuming we have open slots
							OnConnectionRequest( remoteSystem, AESKey, true );

							// Invalidate the new random number
							if ( newRandomNumber )
								GenerateSYNCookieRandomNumber();
						}
						delete [] data;
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

#ifdef __USE_IO_COMPLETION_PORTS
								bool b;
								// Create a new nonblocking socket
								remoteSystem->reliabilityLayer.SetSocket( SocketLayer::Instance()->CreateBoundSocket( myPlayerId.port, false ) );

								SocketLayer::Instance()->Connect( remoteSystem->reliabilityLayer.GetSocket(), playerId.binaryAddress, playerId.port );
								// Associate our new socket with a completion port and do the first read
								b = SocketLayer::Instance()->AssociateSocketWithCompletionPortAndRead( remoteSystem->reliabilityLayer.GetSocket(), playerId.binaryAddress, playerId.port, rakPeer );
								//client->//reliabilityLayerMutex.Unlock();

								if ( b == false )   // Some damn completion port error... windows is so unreliable
								{
#ifdef _DO_PRINTF
									printf( "RakClient - AssociateSocketWithCompletionPortAndRead failed" );
#endif
									return ;
								}
#endif

								// Use the stored encryption key
								if (remoteSystem->setAESKey)
									remoteSystem->reliabilityLayer.SetEncryptionKey( remoteSystem->AESKey );
								else
									remoteSystem->reliabilityLayer.SetEncryptionKey( 0 );
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
							// We turned on encryption with SetEncryptionKey.  This pads packets to up to 16 bytes.
							// As soon as a 16 byte packet arrives on the remote system, we will turn on AES.  This works because all encrypted packets are multiples of 16 and the
							// packets I happen to be sending before this are less than 16 bytes.  Otherwise there is no way to know if a packet that arrived is
							// encrypted or not so the other side won't know to turn on encryption or not.
							SendImmediate( (char*)outBitStream.GetData(), outBitStream.GetNumberOfBitsUsed(), SYSTEM_PRIORITY, RELIABLE, 0, playerId, false, false, RakNet::GetTime() );

							if (alreadyConnected==false)
							{
								PingInternal( playerId, true );
								SendStaticDataInternal( playerId, true );
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
	// unsigned int time;

#ifdef __USE_IO_COMPLETION_PORTS

	AsynchronousFileIO::Instance()->IncreaseUserCount();
#endif

	// 11/15/05 - this is slower than Sleep()
	/*
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
	// Lets see if these timers give better performance than Sleep
	HANDLE timerHandle;
	LARGE_INTEGER dueTime;

	if ( rakPeer->threadSleepTimer == 0 )
		rakPeer->threadSleepTimer = 1;

	// 2nd parameter of false means synchronization timer instead of manual-reset timer
	timerHandle = CreateWaitableTimer( NULL, FALSE, 0 );

	assert( timerHandle );

	dueTime.QuadPart = -10000 * rakPeer->threadSleepTimer; // 10000 is 1 ms?

	BOOL success = SetWaitableTimer( timerHandle, &dueTime, rakPeer->threadSleepTimer, NULL, NULL, FALSE );

	assert( success );

#endif
#endif
	*/

	rakPeer->isMainLoopThreadActive = true;

	while ( rakPeer->endThreads == false )
	{
		/*
		time=RakNet::GetTime();

		// Dynamic threading - how long we sleep and if we update
		// depends on whether or not the user thread is updating
		if (time > rakPeer->lastUserUpdateCycle && time - rakPeer->lastUserUpdateCycle > UPDATE_THREAD_UPDATE_TIME)
		{
		// Only one thread should call RunUpdateCycle at a time.  We don't need to delay calls so
		// a mutex on the function is not necessary - only on the variable that indicates if the function is
		// running
		rakPeer->RunMutexedUpdateCycle();


		// User is not updating the network. Sleep a short time
		#ifdef _WIN32
		Sleep(rakPeer->threadSleepTimer);
		#else
		usleep(rakPeer->threadSleepTimer * 1000);
		#endif
		}
		else
		{
		// User is actively updating the network.  Only occasionally poll
		#ifdef _WIN32
		Sleep(UPDATE_THREAD_POLL_TIME);
		#else
		usleep(UPDATE_THREAD_POLL_TIME * 1000);
		#endif
		}
		*/
		rakPeer->RunUpdateCycle();
#ifdef _WIN32
		//#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
#if (0) // 08/05/05 This doesn't seem to work well at all!
		//#pragma message("-- RakNet:Using WaitForSingleObject --")

		if ( WaitForSingleObject( timerHandle, INFINITE ) != WAIT_OBJECT_0 )
		{
#ifdef _DEBUG
	
			assert( 0 );
	#ifdef _DO_PRINTF
			printf( "WaitForSingleObject failed (%d)\n", GetLastError() );
	#endif
#endif
		}

#else
		//#pragma message("-- RakNet:Using Sleep --")
		//#pragma message("-- Define _WIN32_WINNT as 0x0400 or higher to use WaitForSingleObject --")
		Sleep( rakPeer->threadSleepTimer );

#endif
#else
		usleep( rakPeer->threadSleepTimer * 1000 );
#endif

	}

	rakPeer->isMainLoopThreadActive = false;

#ifdef __USE_IO_COMPLETION_PORTS

	AsynchronousFileIO::Instance()->DecreaseUserCount();
#endif

/*
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)

	CloseHandle( timerHandle );
#endif
#endif
*/

	return 0;
}

/*
void RakPeer::RunMutexedUpdateCycle(void)
{
rakPeerMutexes[RakPeer::updateCycleIsRunning_Mutex].Lock();
if (updateCycleIsRunning==false)
{
updateCycleIsRunning=true;
rakPeerMutexes[RakPeer::updateCycleIsRunning_Mutex].Unlock();
RunUpdateCycle(); // Do one update per call to Receive
rakPeerMutexes[RakPeer::updateCycleIsRunning_Mutex].Lock();
updateCycleIsRunning=false;
rakPeerMutexes[RakPeer::updateCycleIsRunning_Mutex].Unlock();
}
else
rakPeerMutexes[RakPeer::updateCycleIsRunning_Mutex].Unlock();
}
*/
