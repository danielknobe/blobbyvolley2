/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file
* @brief Implementation of RAkNetworkFactory class
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
#include "RakNetworkFactory.h"
#include "RakServerInterface.h"
#include "RakClientInterface.h"
#include "RakServer.h"
#include "RakClient.h"
#include "RakPeerInterface.h"
#include "RakPeer.h"


// Returns a new instance of the network client.
RakClientInterface* RakNetworkFactory::GetRakClientInterface( void )
{
	return new RakClient;
}

// Returns a new instance of the network server.
RakServerInterface* RakNetworkFactory::GetRakServerInterface( void )
{
	return new RakServer;
}


// Returns a new instance of the network peer.
RakPeerInterface* RakNetworkFactory::GetRakPeerInterface( void )
{
	return new RakPeer;
}

// Destroys an instance of the network client;
void RakNetworkFactory::DestroyRakClientInterface( RakClientInterface* i )
{
	delete ( RakClient* ) i;
}

// Destroys an instance of the network server;
void RakNetworkFactory::DestroyRakServerInterface( RakServerInterface* i )
{
	delete ( RakServer* ) i;
}

void RakNetworkFactory::DestroyRakPeerInterface( RakPeerInterface* i )
{
	delete ( RakPeer* ) i;
}
