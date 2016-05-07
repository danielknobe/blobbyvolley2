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
#include "../blobnet/Logger.hpp"

#include "SocketLayer.h"
#include <cassert>
#include "MTUSize.h"

#ifdef _WIN32
#include <process.h>
typedef int socklen_t;
#else
#include <cstring> // memcpy
#include <fcntl.h>
#endif

int SocketLayer::socketLayerInstanceCount = 0;

SocketLayer SocketLayer::I;

#ifdef _WIN32
extern void __stdcall ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
#else
extern void ProcessNetworkPacket( unsigned int binaryAddress, unsigned short port, const char *data, int length, RakPeer *rakPeer );
#endif

#ifdef _DEBUG
#include <cstdio>
#endif

SocketLayer::SocketLayer()
{
	// Check if the socketlayer is already started
	if (socketLayerInstanceCount == 0)
	{
#ifdef _WIN32
		WSADATA winsockInfo;

		// Initiate use of the Winsock DLL (Up to Verion 2.2)
		if (WSAStartup(MAKEWORD(2, 2), &winsockInfo ) != 0)
		{
			LOG("SocketLayer", "WSAStartup failed")
		}
#endif
	}

	// increase usecount
	socketLayerInstanceCount++;
}

SocketLayer::~SocketLayer()
{
	if (socketLayerInstanceCount == 1)
	{
#ifdef _WIN32
		// Terminate use of the Winsock DLL
		WSACleanup();
#endif
	}

	// decrease usecount
	socketLayerInstanceCount--;
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
		LOG("SocketLayer", "WSAConnect failed")
	}

	return writeSocket;
}

#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
SOCKET SocketLayer::CreateBoundSocket(unsigned short port, bool blockingSocket, const char *forceHostAddress)
{
	SOCKET listenSocket;
	sockaddr_in listenerSocketAddress;
	int ret;

	listenSocket = socket( AF_INET, SOCK_DGRAM, 0 );

	if ( listenSocket == INVALID_SOCKET )
	{
		LOG("SocketLayer", "socket(...) failed")

		return INVALID_SOCKET;
	}

	int sock_opt = 1;

	if ( setsockopt( listenSocket, SOL_SOCKET, SO_REUSEADDR, ( char * ) & sock_opt, sizeof ( sock_opt ) ) == -1 )
	{
		LOG("SocketLayer", "setsockopt(SO_REUSEADDR) failed")
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
		LOG("SocketLayer", "setsockopt(SO_BROADCAST) failed")

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
		LOG("SocketLayer", "bind(...) failed")

		return INVALID_SOCKET;
	}

	return listenSocket;
}

const char* SocketLayer::DomainNameToIP(const char *domainName)
{
	int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res);
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

int SocketLayer::Write( SOCKET writeSocket, const char* data, int length )
{
#ifdef _DEBUG
	assert( writeSocket != INVALID_SOCKET );
#endif
	return send( writeSocket, data, length, 0 );
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
		LOG("SocketLayer", "Recvfrom returned 0 on a connectionless blocking call\non port %i. This is a bug with Zone Alarm. Please turn off Zone Alarm.\n" << ntohs( sa.sin_port ) )
#ifdef _DEBUG
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
			// ProcessPortUnreachable(sa.sin_addr.s_addr, portnum, rakPeer);
			// this function no longer exists, as the old implementation did nothing
			// *errorCode = dwIOError;
			return SOCKET_ERROR;
		}
		else
		{
			if ( dwIOError != WSAEINTR )
			{
				LOG("SocketLayer", "recvfrom failed")
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
//		LOG("A previous send operation resulted in an ICMP Port Unreachable message.\n" )
	}
	else
	{
		LOG("SocketLayer", "recvfrom failed")

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
		LOG("SocketLayer", "gethostname failed")

		return ;
	}

	LOG("SocketLayer", "Host name is " << hostname )

	struct hostent *phe = gethostbyname( hostname );

	if ( phe == 0 )
	{
		LOG("SocketLayer", "gethostbyname failed")

		return ;
	}

	for ( int i = 0; phe->h_addr_list[ i ] != 0 && i < 10; ++i )
	{

		struct in_addr addr;

		memcpy( &addr, phe->h_addr_list[ i ], sizeof( struct in_addr ) );
		//cout << "Address " << i << ": " << inet_ntoa(addr) << endl;
		strcpy( ipList[ i ], inet_ntoa( addr ) );
		LOG("SocketLayer", "My IP addresses "<< ipList[ i ])
	}
}


char* const SocketLayer::nameToIP(char const * const name, char* const buffer, int const bufferSize)
{
	struct addrinfo *res = NULL;
	int ErrorCode = 0;
	struct addrinfo hints;
	
	// Prepare hints, atm get only ipv4 address
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;

	// Fill the list with data
	int errorCode = getaddrinfo(name, NULL, &hints, &res);

	// Check if data is available
	// Copy and format address if possible
	if ((errorCode != 0) || 
	    (res == NULL) || 
	    (res->ai_addr == NULL) ||
	    (this->ipToString(res->ai_addr, buffer, bufferSize) == NULL))
	{
		LOG("SocketLayer", "Ip of host can't be resolved")
		if (res != NULL)
		{
			freeaddrinfo(res);
		}
		return NULL;
	}

	// Cleanup
	freeaddrinfo(res);

	return buffer;
}


char const * SocketLayer::ipToString(struct sockaddr const * const socketaddress, char * const buffer, int const bufferSize)
{
	switch(socketaddress->sa_family)
	{
	case AF_INET:
		return inet_ntop(AF_INET, &(((struct sockaddr_in *)socketaddress)->sin_addr), buffer, bufferSize);
	case AF_INET6:
		return inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)socketaddress)->sin6_addr), buffer, bufferSize);
	default:
		return NULL;
	}

	return NULL;
}

