/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_RPC 
 * @file 
 * @brief Internal Stuff for RPC Handling 
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

