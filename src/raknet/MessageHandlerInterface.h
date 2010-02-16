/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Message handler interface
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

#ifndef __MESSAGE_HANDLER_INTERFACE_H
#define __MESSAGE_HANDLER_INTERFACE_H

class RakPeerInterface;
struct Packet;
#include "NetworkTypes.h"

// MessageHandlers should derive from MessageHandlerInterface and be attached to RakPeer using the function AttachMessageHandler
// On a user call to Receive, OnReceive is called for every MessageHandlerInterface, which can then take action based on the packet
// passed to it.  This is used to transparently add game-independent functional modules, similar to browser plugins
class MessageHandlerInterface
{
public:
	/**
	* OnUpdate is called everytime a packet is checked for .
	*
	* @param peer - the instance of RakPeer that is calling Receive
	* 
	*/
	virtual void OnAttach(RakPeerInterface *peer);

	/**
	* OnUpdate is called everytime a packet is checked for .
	*
	* @param peer - the instance of RakPeer that is calling Receive
	* 
	*/
	virtual void OnUpdate(RakPeerInterface *peer)=0;

	/**
	* OnReceive is called for every packet.
	*
	* @param peer - the instance of RakPeer that is calling Receive
	* @param packet - the packet that is being returned to the user
	* 
	* @return true to absorb the packet, false to allow the packet to propagate to another handler, or to the game
	*/
	virtual bool OnReceive(RakPeerInterface *peer, Packet *packet)=0;

	/**
	* Called when RakPeer is shutdown
	*
	* @param playerId - the instance of RakPeer that is calling Receive
	* 
	*/
	virtual void OnDisconnect(RakPeerInterface *peer)=0;

	/**
	* PropagateToGame tells RakPeer if a particular packet should be sent to the game or not
	* If you create a custom packet ID just for this handler you would not want to propagate it to the game, for example
	*
	* @param id - The first byte of the packet in question
	* 
	* @return true (default) to allow a packet to propagate to the game.  False to only let the packet be sent to message handlers
	*/
	virtual bool PropagateToGame(Packet *packet) const;
};

#endif

