/*=============================================================================
Blobby Volley 2
Copyright (C) 2023 Daniel Knobe (daniel-knobe@web.de)
Copyright (C) 2023 Erik Schultheis (erik-schultheis@freenet.de)

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

#include <mutex>
#include <memory>
#include "raknet/PacketPriority.h"
#include "raknet/NetworkTypes.h"
#include "raknet/RakServer.h"

/*! \brief Wraps a RakServer, so that it can only be accessed while a mutex is locked.
 *  \details Any interaction with the underlying RakServer has to go through the `access`
 *  function, which ensures that the mutex is locked, and then calls an arbitrary callable
 *  with the actual server.
 *
 *  As `Send`ing a packet is by far the most common operation, we provide a specialized convenience
 *  function.
 */
class ThreadSafeRakServer {
public:
	ThreadSafeRakServer() : mServer(new RakServer()) {};

	template<class F>
	auto access(F&& f) -> decltype(f(std::declval<RakServer&>())) {
		std::lock_guard<std::mutex> lock(mMutex);
		return f(*mServer);
	}

	void Send(const RakNet::BitStream& stream, PacketPriority priority, PacketReliability reliability, PlayerID target,
			  bool broadcast=false) {
		access([&](RakServer& server){ server.Send(&stream, priority, reliability, 0, target, broadcast); });
	}

private:
	std::mutex mMutex;
	std::unique_ptr<RakServer> mServer;
};