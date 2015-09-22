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

#include "GameState.h"
#include "NetworkMessage.h"
#include "PlayerIdentity.h"

#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

class RakClient;
class RakServer;
class DuelMatch;
class InputSource;
class NetworkGame;
class PlayerIdentity;
class DedicatedServer;

/*! \class NetworkGameState
	\brief State for Network Game
	\details state which is responsible for presenting a network game, sending player input to the
				server, managing chat etc.
*/
class NetworkGameState : public GameState
{
public:
	/// create a NetworkGameState with connection to a certain server
	/// \param client A client which has an established connection to the server we want to start the game on.
	NetworkGameState(boost::shared_ptr<RakClient> client, int rule_checksum, int score_to_win);

	virtual ~NetworkGameState();
	virtual void step_impl();
	virtual const char* getStateName() const;

private:
	enum
	{
		WAITING_FOR_OPPONENT,
		OPPONENT_DISCONNECTED,
		DISCONNECTED,
		SERVER_FULL,
		PLAYING,
		PLAYER_WON,
		PAUSING
	} mNetworkState;

	// these are pointers to mLeftPlayer or mRightPlayer respectively, so we don't need a smart pointer here
	PlayerIdentity* mLocalPlayer;
	PlayerIdentity* mRemotePlayer;

	bool mUseRemoteColor;

	boost::scoped_ptr<InputSource> mLocalInput;

	bool mWaitingForReplay;

	boost::shared_ptr<RakClient> mClient;
	PlayerSide mOwnSide;
	PlayerSide mWinningPlayer;

	// Chat Vars
	std::vector<std::string> mChatlog;
	std::vector<bool > mChatOrigin;
	unsigned mSelectedChatmessage;
	unsigned mChatCursorPosition;
	std::string mChattext;
};
