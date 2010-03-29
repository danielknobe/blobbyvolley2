/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#pragma once

#include <boost/shared_ptr.hpp>
#include "raknet/RakPeer.h"
#include <iostream>

class Packet;

typedef boost::shared_ptr<Packet> packet_ptr;

struct deleter{
	RakPeer* peer;
	void operator()(Packet* p){
		peer->DeallocatePacket(p);
	}
};

inline packet_ptr receivePacket(RakPeer* peer){
	deleter del;
	del.peer = peer;
	Packet* pptr = peer->Receive();
	if(pptr){
		return packet_ptr(pptr, del);
	}else{
		return packet_ptr();
	}	
}
