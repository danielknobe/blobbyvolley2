/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_RPC 
 * @file 
 * @brief Internal Stuff for RPC Handling 
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
#ifndef __RPC_MAP
#define __RPC_MAP

#include "RPCNode.h"
#include "ArrayList.h"
#include "NetworkTypes.h"

/**
 * @ingroup RAKNET_RPC 
 * @note You should not use this class directly. It is used internally in the 
 * RPC Subsystem 
 * 
 * @brief Maps index to RPC node
 * 
 * An RPC Node corresponds to one register function. 
 */
struct  RPCMap
{
public:
	RPCMap();
	~RPCMap();
	void Clear(void);
    RPCNode *GetNodeFromIndex(RPCIndex index);
	RPCNode *GetNodeFromFunctionName(char *uniqueIdentifier);
	RPCIndex GetIndexFromFunctionName(char *uniqueIdentifier);
	void AddIdentifierWithFunction(char *uniqueIdentifier, void *functionPointer, bool isPointerToMember);
	void AddIdentifierAtIndex(char *uniqueIdentifier, RPCIndex insertionIndex);
	void RemoveNode(char *uniqueIdentifier);
protected:
	BasicDataStructures::List<RPCNode *> rpcSet;
};

#endif

