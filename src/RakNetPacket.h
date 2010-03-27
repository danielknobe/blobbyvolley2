#include <boost/shared_ptr.hpp>
#include "raknet/RakPeer.h"
#include <iostream>
#pragma once

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
