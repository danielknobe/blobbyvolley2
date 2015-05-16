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

#include "raknet/NetworkTypes.h"
#include <map>
#include <functional>
#include "global.h"

class NetworkPlayer;
class NetworkGame;

/*! \class MatchMaker
	\brief class responsible form combining players into pairs that play a match.
	\details manages challenges between different players, as well as a list of waiting players.
*/
class MatchMaker
{
public:
	// returns a unique challenge ID
	unsigned addChallenge( PlayerID challenger, PlayerID challenged, int speed, int rules, int points );	// add a new challenge.
	/// accepts a challenge by ID. returns false if \p challenge is not a valid challenge id
	bool acceptChallenge( PlayerID player, unsigned int challenge );
	/// decline a challenge by ID. if \p challenge is no longer valid, do nothing
	void declineChallenge( PlayerID player, unsigned int challenge );


	void addPlayer( PlayerID id, boost::shared_ptr<NetworkPlayer> player );
	void removePlayer( PlayerID id );

	// set callback functions
	typedef std::function<boost::shared_ptr<NetworkGame>(boost::shared_ptr<NetworkPlayer>, boost::shared_ptr<NetworkPlayer>, PlayerSide)> create_game_fn;
	void setCreateGame( create_game_fn func) { mCreateGame = func;};

	typedef std::function<void(const RakNet::BitStream& stream, PlayerID target)> send_fn;
	void setSendFunction( send_fn func ) { mSendPacket = func; };

	// communication
	void receiveChallengePacket( PlayerID sender, RakNet::BitStream content );

private:

	/// create a new network game from the challenges id1 and id2. If either is not valid, no game is created.
	void makeMatch( unsigned id1, unsigned id2 );

	struct Challenge
	{
		PlayerID challenger;
		PlayerID challenged;
		int speed;
		int rules;
		int points;
	};

	static bool compatible(const Challenge& c1, const Challenge& c2) ;

	std::map<unsigned, Challenge> mChallenges;
	unsigned int mIDCounter = 0;

	// waiting player map
	std::map< PlayerID, boost::shared_ptr<NetworkPlayer>> mPlayerMap;

	// callbacks
	create_game_fn mCreateGame;
	send_fn mSendPacket;
};
