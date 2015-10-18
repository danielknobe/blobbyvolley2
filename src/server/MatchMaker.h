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
#include "Global.h"

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
	unsigned openGame( PlayerID creator, int speed, int rules, int points );

	void addPlayer( PlayerID id, boost::shared_ptr<NetworkPlayer> player );
	void removePlayer( PlayerID id );

	// set callback functions
	typedef std::function<void(boost::shared_ptr<NetworkPlayer>, boost::shared_ptr<NetworkPlayer>,
								PlayerSide, std::string rules, int score, float speed)> create_game_fn;
	void setCreateGame( create_game_fn func) { mCreateGame = func;};

	typedef std::function<void(const RakNet::BitStream& stream, PlayerID target)> send_fn;
	void setSendFunction( send_fn func ) { mSendPacket = func; };

	// communication
	void receiveLobbyPacket( PlayerID sender, RakNet::BitStream content );
	/// send a packet with all currently open games to \p recipient
	void sendOpenGameList( PlayerID recipient );

	// broadcast the status of a game
	void broadcastOpenGameStatus( unsigned gameID );
	void broadcastOpenGameList(); // sends the game list to all players that are not in a game


	// add settings
	void addGameSpeedOption( int speed );
	void addRuleOption( const std::string& file );
	void setAllowNewGames( bool allow );

	// info functions
	unsigned getOpenGamesCount() const;
	std::vector<unsigned> getOpenGameIDs() const;

private:
	struct OpenGame;

	/// add a new game to the gamelist
	unsigned addGame( OpenGame game );
	void joinGame(PlayerID player, unsigned gameID);
	void startGame(PlayerID host, PlayerID client);

	void removeGame( unsigned id );
	void removePlayerFromAllGames( PlayerID player );
	void removePlayerFromGame( unsigned game, PlayerID player );

	/// create a new network game from the challenges id1 and id2. If either is not valid, no game is created.
	void makeMatch( unsigned id1, unsigned id2 );

	struct OpenGame
	{
		// owner
		PlayerID creator;
		// game name
		std::string name;
		// settings
		int speed;
		int rules;
		int points;
		// connected players
		std::vector<PlayerID> connected;
	};

	struct Rule
	{
		std::string file;
		std::string name;
		std::string author;
		std::string description;
	};

	std::map<unsigned, OpenGame> mOpenGames;
	unsigned int mIDCounter = 0;

	// waiting player map
	std::map< PlayerID, boost::shared_ptr<NetworkPlayer>> mPlayerMap;

	// possible game configurations
	std::vector<unsigned int> mPossibleGameSpeeds;
	std::vector<Rule> mPossibleGameRules;
	bool mAllowNewGames = true;

	// callbacks
	create_game_fn mCreateGame;
	send_fn mSendPacket;
};
