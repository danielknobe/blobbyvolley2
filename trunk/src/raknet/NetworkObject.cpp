/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file NetworkObject.cpp
* @brief DEPRECIATED! Use NetworkIDGenerator instead!
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
#include "NetworkObject.h"
#include "RakServerInterface.h"
#include "RakClientInterface.h"
#include "DistributedNetworkObjectManager.h"


//////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////

unsigned short NetworkObject::GetID( void )
{
	return NetworkIDGenerator::GetNetworkID();
};

//-------------------------------------------------------------------------------------

bool NetworkObject::IsObjectIDAuthority(void) const
{
	return DistributedNetworkObjectManager::Instance()->GetRakServerInterface()!=0;
}

//-------------------------------------------------------------------------------------

bool NetworkObject::IsObjectIDAuthorityActive(void) const
{
	return DistributedNetworkObjectManager::Instance()->GetRakServerInterface()->IsActive();
}

//-------------------------------------------------------------------------------------

bool NetworkObject::IsObjectIDRecipient(void) const
{
	return DistributedNetworkObjectManager::Instance()->GetRakClientInterface()!=0;
}

//-------------------------------------------------------------------------------------

bool NetworkObject::IsObjectIDRecipientActive(void) const
{
	return DistributedNetworkObjectManager::Instance()->GetRakClientInterface()->IsConnected();
}

//-------------------------------------------------------------------------------------

void NetworkObject::SetID( unsigned short id )
{
	NetworkIDGenerator::SetNetworkID(id);
}

//-------------------------------------------------------------------------------------

unsigned short NetworkObject::GetStaticItemID( void )
{
	return NetworkIDGenerator::GetStaticNetworkID();
}

//-------------------------------------------------------------------------------------

void NetworkObject::SetStaticItemID( unsigned short i )
{
	NetworkIDGenerator::SetStaticNetworkID(i);
}


//////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////
