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

#include "MatchMaker.h"

#include <boost/make_shared.hpp>
#include <iostream>

#include "global.h"
#include "GenericIO.h"
#include "NetworkMessage.h"
#include "NetworkGame.h"
#include "NetworkPlayer.h"

unsigned MatchMaker::addChallenge(PlayerID challenger, PlayerID challenged, int speed, int rules, int points)
{
	Challenge new_ch{challenger, challenged, speed, rules, points};
	unsigned id = ++mIDCounter;
	mChallenges[id] = new_ch;

	/// \todo prevent duplicates!

	// check if this request would fulfill another challenge
	for( const auto& c : mChallenges )
	{
		// find out if we have a (different) match that is compatible
		if( c.first != id && compatible(new_ch, c.second) )
		{
			std::cout << "MAKE MATCH\n";
			makeMatch( id, c.first );
			return id;
		}
	}

	std::cout << "MatchMaker status " << mChallenges.size() << " challenges, " << mPlayerMap.size() << " players\n";

	return id;
}

bool MatchMaker::acceptChallenge( PlayerID player, unsigned int challenge )
{
	// we accept the challenge by simply duplicating it
	auto it = mChallenges.find( challenge );
	if( it == mChallenges.end() )
		return false;

	auto ch_data = it->second;
	// we found it. duplicate!
	addChallenge(player, ch_data.challenger, ch_data.speed, ch_data.rules, ch_data.points);
	return true;
}

void MatchMaker::declineChallenge( PlayerID player, unsigned int challenge )
{
	auto it = mChallenges.find( challenge );
	if( it == mChallenges.end() )
		return;
	mChallenges.erase(it);
}

bool MatchMaker::compatible(const Challenge& c1, const Challenge& c2)
{
	// check player compatibility
	bool direct_match = c1.challenger == c2.challenged && c1.challenged == c2.challenger;
	bool indirect_match = (c1.challenged == c2.challenger && c2.challenger == UNASSIGNED_PLAYER_ID) ||
							(c2.challenged == c1.challenger && c1.challenger == UNASSIGNED_PLAYER_ID);

	if(!(direct_match || indirect_match) )
		return false;

	// now check game setup. later, add wildcard support
	bool speed_compatible = c1.speed == c2.speed;
	bool rules_compatible = c1.rules == c2.rules;
	bool points_compatible = c1.points == c2.points;

	return speed_compatible && rules_compatible && points_compatible;
}

// create a new NetworkGame
void MatchMaker::makeMatch( unsigned id1, unsigned id2 )
{
	// find corresponding challenges
	auto ch1 = mChallenges.find(id1);
	auto ch2 = mChallenges.find(id2);

	// challenges have to be valid.
	/// \todo safer checks than assert
	assert( ch1 != mChallenges.end() );
	assert( ch2 != mChallenges.end() );
	assert( compatible(ch1->second, ch2->second) );

	// get the players
	auto pl1 = ch1->second.challenger;
	auto pl2 = ch2->second.challenger;

	// check player validity
	assert( mPlayerMap.find( pl1 ) != mPlayerMap.end() );
	assert( mPlayerMap.find( pl2 ) != mPlayerMap.end() );

	// match with NetworkPlayers
	auto leftPlayer = mPlayerMap[pl1];
	auto rightPlayer = mPlayerMap[pl2];

	PlayerSide switchSide = NO_PLAYER;

	// put first player on his desired side in game
	// check all 4 combinations and set switchSide accordingly
	if( leftPlayer->getDesiredSide() == RIGHT_PLAYER && rightPlayer->getDesiredSide() == LEFT_PLAYER )
		std::swap(leftPlayer, rightPlayer);
	if( leftPlayer->getDesiredSide() == RIGHT_PLAYER && rightPlayer->getDesiredSide() == RIGHT_PLAYER )
		switchSide = LEFT_PLAYER;
	if( leftPlayer->getDesiredSide() == LEFT_PLAYER && rightPlayer->getDesiredSide() == LEFT_PLAYER )
		switchSide = RIGHT_PLAYER;

	/// \todo find compatible options regarding speed, rules and points in case of wildcards

	auto newgame = mCreateGame(leftPlayer, rightPlayer, switchSide);
	leftPlayer->setGame( newgame );
	rightPlayer->setGame( newgame );

	// remove players as waiting players
	removePlayer(pl1);
	removePlayer(pl2);
}


// - - - - - - - - - - - - - - - - - -
// 			player management
// - - - - - - - - - - - - - - - - - -

void MatchMaker::addPlayer( PlayerID id, boost::shared_ptr<NetworkPlayer> player )
{
	assert( mPlayerMap.find(id) == mPlayerMap.end() );
	mPlayerMap[id] = player;
}

void MatchMaker::removePlayer( PlayerID id )
{
	// removing an id that is not in mPlayerMap is a valid use case.
	// It happens when a player enters a game [removed from waiting players] and then disconnects [removed again]
	mPlayerMap.erase( id );

	// remove challenges of that player
	for( auto it = mChallenges.begin(); it != mChallenges.end(); ++it)
	{
		if( it->second.challenger == id )
			it = mChallenges.erase(it);
	}

	std::cout << "MatchMaker status " << mChallenges.size() << " challenges, " << mPlayerMap.size() << " players\n";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MatchMaker::receiveChallengePacket( PlayerID first, RakNet::BitStream stream )
{
	// which player is wanted as opponent
	PlayerID second = UNASSIGNED_PLAYER_ID;

	auto player = mPlayerMap.find(first);
	if( player == mPlayerMap.end() )
	{
		// seems like we did not receive a ENTER_SERVER Packet before.
		std::cerr << "a player tried to enter a game, but has no player info\n";
		return;
	}

	auto firstPlayer = player->second;

	auto reader = createGenericReader(&stream);
	unsigned char type, speed, rules, points;
	reader->byte(type);
	reader->byte(type);

	if( type == 1 )	// challenge
	{
		reader->generic<PlayerID>(second);
		reader->byte(speed);
		reader->byte(rules);
		reader->byte(points);

		// debug log challenge
		std::cerr << first.toString() << " challenged " << second.toString() << "\n";

		auto id = addChallenge(first, second, speed, rules, points);
		RakNet::BitStream stream;
		auto writer = createGenericWriter(&stream);
		writer->byte( (unsigned char)ID_CHALLENGE );
		writer->byte(type);
		writer->generic<PlayerID>(first);
		writer->byte(speed);
		writer->byte(rules);
		writer->byte(points);
		writer->uint32( id );

		mSendPacket(stream, second);
	}
	 else if( type == 2 ) // accept
	{
		unsigned int id;
		reader->uint32(id);

		// we need to check if that match is actually valid
		if(acceptChallenge(first, id))
		{
			std::cout << first.toString() << " accepted challenge " << id << "\n";
		}
		 else
		{
			std::cout << first.toString() << " accepted challenge " << id << ", which was no longer valid\n";
		}
	}
}
