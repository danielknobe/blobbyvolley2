/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Socket Layer Abstraction 
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

#ifndef __SOCKET_LAYER_H
#define __SOCKET_LAYER_H

#ifdef _WIN32
#ifdef __USE_IO_COMPLETION_PORTS
#include <winSock2.h> // DON'T FORGET TO INLCLUDE Ws2_32.lib to your project!
#else
#include <winsock.h>
#endif
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h> 
/**
* typename for communication endpoint
*/
typedef int SOCKET;
/**
* Invalid socket
*/
static const SOCKET INVALID_SOCKET = -1;
/**
* Socket error
*/
#define SOCKET_ERROR -1
#endif
#include "ClientContextStruct.h"

class RakPeer;

/**
 * the SocketLayer provide platform independent Socket implementation
 */

class SocketLayer
{

public:
	/**
	 * Default Constructor
	 */
	SocketLayer();
	/**
	 * Destructor 
	 */
	~SocketLayer();
	/**
	 * Get Singleton Instance of the Socket Layer unique object. 
	 * @return unique instance 
	 */
	static inline SocketLayer* Instance()
	{
		return & I;
	}
	
	/**
	 * Create a socket connected to a remote host 
	 * @param writeSocket The local socket 
	 * @param binaryAddress The address of the remote host 
	 * @param port the remote port 
	 * @return A new socket used for communication 
	 * @todo
	 * Check for the binary address byte order 
	 * 
	 */
	SOCKET Connect( SOCKET writeSocket, unsigned int binaryAddress, unsigned short port );
	/**
	 * Creates a socket to listen for incoming connections on the specified port
	 * @param port the port number 
	 * @param blockingSocket 
	 * @return A new socket used for accepting clients 
	 */
	SOCKET CreateBoundSocket( unsigned short port, bool blockingSocket, const char *forceHostAddress );
	const char* DomainNameToIP( const char *domainName );
#ifdef __USE_IO_COMPLETION_PORTS
	/**
	 * Associate a socket to the completion port mecanism. 
	 * @param socket the socket
	 * @param completionKey the completion port group identifier 
	 * @note Windows version only
	 */
	void AssociateSocketWithCompletionPort( SOCKET socket, ClientContextStruct* completionKey );
#endif
	/**
	 * Start an asynchronous read using the specified socket.  The
	 * callback will use the specified PlayerID (associated with this
	 * socket) and call either the client or the server callback (one or
	 * the other should be 0)
	 */
	bool AssociateSocketWithCompletionPortAndRead( SOCKET readSocket, unsigned int binaryAddress, unsigned short port, RakPeer* rakPeer );
	/**
	 * Does a writing operation on a socket.
	 * It Send a packet to a peer throught the network.
	 * The socket must be connected
	 * @param writeSocket the socket to use to do the communication 
	 * @param data a byte buffer containing the data 
	 * @param length the size of the byte buffer  
	 * return written bytes
	 */
	int Write( SOCKET writeSocket, const char* data, int length );
	/**
	 * Read data from a socket 
	 * @param s the socket 
	 * @param rakPeer 
	 * @param errorCode An error code if an error occured 
	 * @return Returns true if you successfully read data
	 * @todo check the role of RakPeer 
	 * 
	 */
	int RecvFrom( SOCKET s, RakPeer *rakPeer, int *errorCode );
	/**
	 * Retrieve all local IP address in a printable format 
	 * @param ipList An array of ip address in dot format.
	 */
	void GetMyIP( char ipList[ 10 ][ 16 ] );
	/**
	 * Send data to a peer. The socket should not be connected to a remote host. 
	 * @param s the socket 
	 * @param data the byte buffer to send 
	 * @param length The length of the @em data 
	 * @param ip The address of the remote host in dot format 
	 * @param port The port number used by the remote host 
	 * @return 0 on success. 
	 * 
	 * @todo check return value 
	 */
	int SendTo( SOCKET s, const char *data, int length, char ip[ 16 ], unsigned short port );
	/**
	 * Send data to a peer. The socket should not be connected to a remote host. 
	 * @param s the socket 
	 * @param data the byte buffer to send 
	 * @param length The length of the @em data 
	 * @param binaryAddress The peer address in binary format. 
	 * @param port The port number used by the remote host 
	 * @return 0 on success. 
	 * 
	 * @todo check return value 
	 */
	int SendTo( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port );
	








	/// @brief Get the Ip address of an domain
	/// @param name Name of the domain
	/// @return Ip address
	/// @todo This is only for IPv4. IPv6 is not available jet
	const char* nameToIP(const char* name) const;

















private:
	/**
	 * Tell whether or not the socket layer is already active 
	 */
	static int socketLayerInstanceCount;

	/**
	 * Singleton instance 
	 */
	static SocketLayer I;
};

#endif

