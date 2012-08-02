/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file
* @brief SocketLayer class implementation 
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
#include "SocketLayer.h"
#include <assert.h>
#include "MTUSize.h"

#ifdef _WIN32
#include <process.h>
typedef int socklen_t;
#else
#include <string.h> // memcpy
#include <unistd.h>
#include <fcntl.h>
#endif

#include "ExtendedOverlappedPool.h"
#ifdef __USE_IO_COMPLETION_PORTS
#include "AsynchronousFileIO.h"
#endif

bool SocketLayer::socketLayerStarted = false;

SocketLayer SocketLayer::I;

#ifdef _WIN32
extern void __stdcall ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
#else
extern void ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
#endif

#ifdef _WIN32
extern void __stdcall ProcessPortUnreachable( unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer );
#else
extern void ProcessPortUnreachable( unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer );
#endif

#ifdef _DEBUG
#include <stdio.h>
#endif

#ifdef _DEBUG
void Error(const char* message)
{
#ifdef WIN32
	DWORD dwIOError = GetLastError();
	LPVOID messageBuffer;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT),  // Default language
		( LPTSTR ) &messageBuffer, 0, NULL);
	printf("%s:Error code - %d\n%s", message, dwIOError, messageBuffer);
	LocalFree(messageBuffer);
#else
	printf("%s", message);
#endif
}
#endif


SocketLayer::SocketLayer()
{
	// Check if the socketlayer is already started
	if (socketLayerStarted == false)
	{
#ifdef _WIN32
		WSADATA winsockInfo;

		// Initiate use of the Winsock DLL (Up to Verion 2.2) 
		if (WSAStartup(MAKEWORD(2, 2), &winsockInfo ) != 0)
		{
#ifdef defined(_DEBUG)
			Error("WSAStartup failed");
#endif
		}
#endif
		// The socketlayer started
		socketLayerStarted = true;
	}
}

SocketLayer::~SocketLayer()
{
	if (socketLayerStarted == true)
	{
#ifdef _WIN32
		// Terminate use of the Winsock DLL
		WSACleanup();
#endif
		// The socketlayer stopped
		socketLayerStarted = false;
	}
}

SOCKET SocketLayer::Connect(SOCKET writeSocket, unsigned int binaryAddress, unsigned short port)
{
	assert(writeSocket != INVALID_SOCKET);
	sockaddr_in connectSocketAddress;

	connectSocketAddress.sin_family = AF_INET;
	connectSocketAddress.sin_port = htons( port );
	connectSocketAddress.sin_addr.s_addr = binaryAddress;

	if ( connect( writeSocket, ( struct sockaddr * ) & connectSocketAddress, sizeof( struct sockaddr ) ) != 0 )
	{
#ifdef _DEBUG
		Error("WSAConnect failed");
#endif
	}

	return writeSocket;
}

#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
SOCKET SocketLayer::CreateBoundSocket( unsigned short port, bool blockingSocket, const char *forceHostAddress )
{
	SOCKET listenSocket;
	sockaddr_in listenerSocketAddress;
	int ret;

#ifdef __USE_IO_COMPLETION_PORTS

	if ( blockingSocket == false )
		listenSocket = WSASocket( AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
	else
#endif

		listenSocket = socket( AF_INET, SOCK_DGRAM, 0 );

	if ( listenSocket == INVALID_SOCKET )
	{
#ifdef _DEBUG
		Error("socket(...) failed");
#endif

		return INVALID_SOCKET;
	}

	int sock_opt = 1;

	if ( setsockopt( listenSocket, SOL_SOCKET, SO_REUSEADDR, ( char * ) & sock_opt, sizeof ( sock_opt ) ) == -1 )
	{
#ifdef _DEBUG
		Error("setsockopt(SO_REUSEADDR) failed");
#endif
	}

	//Set non-blocking
#ifdef _WIN32
	unsigned long nonblocking = 1;

	if ( ioctlsocket( listenSocket, FIONBIO, &nonblocking ) != 0 )
	{
		assert( 0 );
		return INVALID_SOCKET;
	}

#else
	if ( fcntl( listenSocket, F_SETFL, O_NONBLOCK ) != 0 )
	{
		assert( 0 );
		return INVALID_SOCKET;
	}

#endif

	// Set broadcast capable
	if ( setsockopt( listenSocket, SOL_SOCKET, SO_BROADCAST, ( char * ) & sock_opt, sizeof( sock_opt ) ) == -1 )
	{
#ifdef _DEBUG
		Error("setsockopt(SO_BROADCAST) failed");
#endif

	}

	// Listen on our designated Port#
	listenerSocketAddress.sin_port = htons( port );

	// Fill in the rest of the address structure
	listenerSocketAddress.sin_family = AF_INET;

	if (forceHostAddress)
	{
		listenerSocketAddress.sin_addr.s_addr = inet_addr( forceHostAddress );
	}
	else
	{
		listenerSocketAddress.sin_addr.s_addr = INADDR_ANY;
	}	

	// bind our name to the socket
	ret = bind( listenSocket, ( struct sockaddr * ) & listenerSocketAddress, sizeof( struct sockaddr ) );

	if ( ret == SOCKET_ERROR )
	{
#ifdef _DEBUG
		Error("bind(...) failed");
#endif

		return INVALID_SOCKET;
	}

	return listenSocket;
}

const char* SocketLayer::DomainNameToIP( const char *domainName )
{

	struct hostent * phe = gethostbyname( domainName );

	if ( phe == 0 || phe->h_addr_list[ 0 ] == 0 )
	{
		//cerr << "Yow! Bad host lookup." << endl;
		return 0;
	}

	struct in_addr addr;

	memcpy( &addr, phe->h_addr_list[ 0 ], sizeof( struct in_addr ) );

	return inet_ntoa( addr );
}

void SocketLayer::Write( SOCKET writeSocket, const char* data, int length )
{
#ifdef _DEBUG
	assert( writeSocket != INVALID_SOCKET );
#endif
#ifdef __USE_IO_COMPLETION_PORTS

	ExtendedOverlappedStruct* eos = ExtendedOverlappedPool::Instance()->GetPointer();
	memset( &( eos->overlapped ), 0, sizeof( OVERLAPPED ) );
	memcpy( eos->data, data, length );
	eos->length = length;

	//AsynchronousFileIO::Instance()->PostWriteCompletion(ccs);
	WriteAsynch( ( HANDLE ) writeSocket, eos );
#else

	send( writeSocket, data, length, 0 );
#endif
}

// Start an asynchronous read using the specified socket.
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
bool SocketLayer::AssociateSocketWithCompletionPortAndRead( SOCKET readSocket, unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer )
{
#ifdef __USE_IO_COMPLETION_PORTS
	assert( readSocket != INVALID_SOCKET );

	ClientContextStruct* ccs = new ClientContextStruct;
	ccs->handle = ( HANDLE ) readSocket;

	ExtendedOverlappedStruct* eos = ExtendedOverlappedPool::Instance()->GetPointer();
	memset( &( eos->overlapped ), 0, sizeof( OVERLAPPED ) );
	eos->binaryAddress = binaryAddress;
	eos->port = port;
	eos->rakPeer = rakPeer;
	eos->length = MAXIMUM_MTU_SIZE;

	bool b = AsynchronousFileIO::Instance()->AssociateSocketWithCompletionPort( readSocket, ( DWORD ) ccs );

	if ( !b )
	{
		ExtendedOverlappedPool::Instance()->ReleasePointer( eos );
		delete ccs;
		return false;
	}

	BOOL success = ReadAsynch( ( HANDLE ) readSocket, eos );

	if ( success == FALSE )
		return false;

#endif

	return true;
}

int SocketLayer::RecvFrom( SOCKET s, RakPeer *rakPeer, int *errorCode )
{
	int len;
	char data[ MAXIMUM_MTU_SIZE ];
	sockaddr_in sa;
	unsigned short portnum;

	socklen_t len2 = sizeof( struct sockaddr_in );
	sa.sin_family = AF_INET;

#ifdef _DEBUG

	portnum = 0;
	data[ 0 ] = 0;
	len = 0;
	sa.sin_addr.s_addr = 0;
#endif

	if ( s == INVALID_SOCKET )
	{
		*errorCode = SOCKET_ERROR;
		return SOCKET_ERROR;
	}

	len = recvfrom( s, data, MAXIMUM_MTU_SIZE, 0, ( sockaddr* ) & sa, ( socklen_t* ) & len2 );

	// if (len>0)
	//  printf("Got packet on port %i\n",ntohs(sa.sin_port));

	if ( len == 0 )
	{
#ifdef _DEBUG
		printf( "Error: recvfrom returned 0 on a connectionless blocking call\non port %i.  This is a bug with Zone Alarm.  Please turn off Zone Alarm.\n", ntohs( sa.sin_port ) );
		assert( 0 );
#endif

		*errorCode = SOCKET_ERROR;
		return SOCKET_ERROR;
	}

	if ( len != SOCKET_ERROR )
	{
		portnum = ntohs( sa.sin_port );
		//strcpy(ip, inet_ntoa(sa.sin_addr));
		//if (strcmp(ip, "0.0.0.0")==0)
		// strcpy(ip, "127.0.0.1");
		ProcessNetworkPacket( sa.sin_addr.s_addr, portnum, data, len, rakPeer );

		return 1;
	}
	else
	{
		*errorCode = 0;

#if defined(_WIN32) && defined(_DEBUG)

		DWORD dwIOError = WSAGetLastError();

		if ( dwIOError == WSAEWOULDBLOCK )
		{
			return SOCKET_ERROR;
		}
		if ( dwIOError == WSAECONNRESET )
		{
			ProcessPortUnreachable(sa.sin_addr.s_addr, portnum, rakPeer);
			// *errorCode = dwIOError;
			return SOCKET_ERROR;
		}
		else
		{
			if ( dwIOError != WSAEINTR )
			{
				Error("recvfrom failed");
			}
		}
#endif
	}

	return 0; // no data
}

int SocketLayer::SendTo( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port )
{
	if ( s == INVALID_SOCKET )
	{
		return -1;
	}

	int len;
	sockaddr_in sa;
	sa.sin_port = htons( port );
	sa.sin_addr.s_addr = binaryAddress;
	sa.sin_family = AF_INET;

	do
	{

		len = sendto( s, data, length, 0, ( const sockaddr* ) & sa, sizeof( struct sockaddr_in ) );
	}

	while ( len == 0 );

	if ( len != SOCKET_ERROR )
		return 0;


#if defined(_WIN32)

	DWORD dwIOError = WSAGetLastError();

	if ( dwIOError == WSAECONNRESET )
	{
#if defined(_DEBUG)
		printf( "A previous send operation resulted in an ICMP Port Unreachable message.\n" );
#endif

	}
	else
	{
#if defined(_DEBUG)
		Error("recvfrom failed");
#endif

	}

	return dwIOError;
#else
	return 1;
#endif
}

int SocketLayer::SendTo( SOCKET s, const char *data, int length, char ip[ 16 ], unsigned short port )
{
	unsigned int binaryAddress;
	binaryAddress = inet_addr( ip );
	return SendTo( s, data, length, binaryAddress, port );
}

void SocketLayer::GetMyIP( char ipList[ 10 ][ 16 ] )
{
	// Longest possible Hostname
	char hostname[80];

	// Get the hostname of the local maschine
	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
	{
#ifdef _DEBUG
		Error("gethostname failed");
#endif
		return ;
	}

	//cout << "Host name is " << hostname << "." << endl;

	struct hostent *phe = gethostbyname( hostname );

	if ( phe == 0 )
	{
#ifdef _DEBUG
		Error("gethostbyname failed");
#endif

		return ;
	}

	for ( int i = 0; phe->h_addr_list[ i ] != 0 && i < 10; ++i )
	{

		struct in_addr addr;

		memcpy( &addr, phe->h_addr_list[ i ], sizeof( struct in_addr ) );
		//cout << "Address " << i << ": " << inet_ntoa(addr) << endl;
		strcpy( ipList[ i ], inet_ntoa( addr ) );
	}
}

