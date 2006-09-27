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
 
#include "NetworkTypes.h"

#ifndef __RPC_NODE
#define __RPC_NODE

class RakPeerInterface;

/**
 * @defgroup RAKNET_RPC Remote Procedure Call Subsystem 
 * @brief Remote Function Invocation 
 * 
 * The Remote Procedure Call Subsystem provide ability to call
 * function on a remote host knowing its name and its parameters. 
 * 
 */

/**
 * @ingroup RAKNET_RPC 
 * @note You should not use this class directly. It is used internally in the 
 * RPC Subsystem 
 * 
 * @brief Map registered procedure inside of a peer.  
 * 
 * An RPC Node corresponds to one register function. 
 */

struct RPCNode
{
	/**
	 * A unique identifier 
	 */
	char *uniqueIdentifier;
	/**
	 * A pointer to the function to be called 
	 */
	union
	{
		void ( *staticFunctionPointer ) ( RPCParameters *rpcParms );
		#ifdef __GNUC__
  		void (*memberFunctionPointer)(void* _this, RPCParameters *rpcParms);
		#else
		void (__cdecl *memberFunctionPointer)(void* _this, RPCParameters *rpcParms);
		#endif

		void *functionPointer;
	};
	/**
	* Is a member function pointer?
	*/
	bool isPointerToMember;
};

#endif

