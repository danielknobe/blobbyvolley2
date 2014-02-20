/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

#include "State.h"
#include "NetworkMessage.h"

#include <vector>
#include <list>
#include <future>
#include <boost/scoped_ptr.hpp>

class RakClient;
class RakServer;
class DuelMatch;
class NetworkGame;

/*! \class NetworkSearchState
	\brief Base class for search states
	\details This class provides a search interface.
			The actual search process has to be implemented
			by derived class by overriding the searchServers()
			method.
*/
class NetworkSearchState : public State
{
public:
	NetworkSearchState();
	virtual ~NetworkSearchState();

	virtual void step();
	// onlinegames connect to the masterserver
	// LAN games send a broadcast to local network
	void searchServers();

	virtual const char* getStateName() const;
protected:
	std::vector<ServerInfo> mScannedServers;
	boost::scoped_ptr<RakClient> mPingClient;

private:
	virtual void doSearchServers() = 0;

	typedef std::list<RakClient*> ClientList;

	ClientList mQueryClients;
	RakClient* mDirectConnectClient;

	std::future<void> mPingJob;

	unsigned mSelectedServer;
	bool mDisplayInfo;
	bool mEnteringServer;
	bool mDisplayUpdateNotification;

	std::string mEnteredServer;
	unsigned mServerBoxPosition;
};

/*! \class OnlineSearchState
	\brief State for online server search screen.
*/
class OnlineSearchState : public NetworkSearchState
{
public:
	OnlineSearchState();
	virtual ~OnlineSearchState() {};
	virtual void doSearchServers();
	virtual const char* getStateName() const;
};


/*! \class LANSearchState
	\brief State for LAN game search screen.
*/
class LANSearchState : public NetworkSearchState
{
public:
	LANSearchState();
	virtual ~LANSearchState() {};
	virtual void doSearchServers();
	virtual const char* getStateName() const;
};

