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
#include "NetworkMessage.h"

#include <vector>

class RakClient;
class RakServer;
class DuelMatch;
class NetworkGame;

/*! \class NetworkGameState
	\brief State for Network Game
	\details state which is responsible for presenting a network game, sending player input to the
				server, managing chat etc.
*/
class NetworkGameState : public State
{
public:
	/// create a NetworkGameState with connection to a certain server
	/// \param servername Name of server
	/// \param port Target port
	NetworkGameState(const std::string& servername, Uint16 port);
	
	virtual ~NetworkGameState();
	virtual void step();
	virtual const char* getStateName() const;
	
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
	bool mWaitingForReplay;
	std::string mFilename;
	std::string mErrorMessage;

	RakClient* mClient;
	PlayerSide mOwnSide;
	PlayerSide mWinningPlayer;

	DuelMatch* mFakeMatch; 	// This hack is necessary to let MouseInputDevice
				// access the necessary game variables

	// Chat Vars
	std::vector<std::string> mChatlog;
	std::vector<bool > mChatOrigin;
	int mSelectedChatmessage;
	unsigned mChatCursorPosition;
	std::string mChattext;
};


/*! \class NetworkHostState
	\brief state for hosting a game locally
	\details
	This class is a wrapper for NetworkGameState to run an instance
	of NetworkGame
	\todo this construction seems like a big hack ;)
*/
class NetworkHostState : public State
{
public:
	NetworkHostState();
	virtual ~NetworkHostState();

	virtual void step();
	virtual const char* getStateName() const;

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

