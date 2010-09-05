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

#include "State.h"
#include "PhysicWorld.h"
#include "NetworkMessage.h"

#include <vector>
#include <list>

class RakClient;
class RakServer;
class DuelMatch;
class NetworkGame;

class NetworkSearchState : public State
{
public:
	NetworkSearchState();
	virtual ~NetworkSearchState();

	virtual void step();
	// onlinegames connect to the masterserver
	// LAN games send a broadcast to local network
	virtual void searchServers() = 0;

protected:
	std::vector<ServerInfo> mScannedServers;
	RakClient* mPingClient;

private:
	typedef std::list<RakClient*> ClientList;



	ClientList mQueryClients;

	int mSelectedServer;
	bool mDisplayInfo;
	bool mEnteringServer;

	std::string mEnteredServer;
	unsigned mServerBoxPosition;
};

class OnlineSearchState : public NetworkSearchState
{
public:
	OnlineSearchState();
	virtual ~OnlineSearchState() {};
	virtual void searchServers();
};

class LANSearchState : public NetworkSearchState
{
public:
	LANSearchState();
	virtual ~LANSearchState() {};
	virtual void searchServers();
};

class NetworkGameState : public State
{
public:
	NetworkGameState(const std::string& servername, Uint16 port);
	virtual ~NetworkGameState();
	virtual void step();
	
private:
	enum
	{
		CONNECTING,
		WAITING_FOR_OPPONENT,
		OPPONENT_DISCONNECTED,
		DISCONNECTED,
		CONNECTION_FAILED,
		SERVER_FULL,
		PLAYING,
		PLAYER_WON,
		PAUSING
	} mNetworkState;

	Player mLeftPlayer;
	Player mRightPlayer;
	
	Player* mLocalPlayer;
	Player* mRemotePlayer;
	
	bool mUseRemoteColor;

	InputSource* mLocalInput;
	PlayerSide mServingPlayer;

	bool mSaveReplay;
	std::string mFilename;

	RakClient* mClient;
	PlayerSide mOwnSide;
	PlayerSide mWinningPlayer;

	DuelMatch* mFakeMatch; 	// This hack is necessary to let MouseInputDevice
				// access the necessary game variables

	// Chat Vars
	std::vector<std::string> mChatlog;
	int mSelectedChatmessage;
	unsigned mChatCursorPosition;
	std::string mChattext;
};


// This class is a wrapper for NetworkGameState to run an instance
// of NetworkGame
class NetworkHostState : public State
{
public:
	NetworkHostState();
	virtual ~NetworkHostState();

	virtual void step();

private:
	NetworkGameState* mGameState;
	NetworkGame* mNetworkGame;
	RakServer* mServer;

	PlayerSide mLocalPlayerSide;
	PlayerID mLocalPlayer;
	PlayerID mRemotePlayer;
	Color mLeftColor;
	Color mRightColor;
	std::string mLocalPlayerName;
	std::string mRemotePlayerName;
};

