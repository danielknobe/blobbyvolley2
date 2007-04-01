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

#include <iostream>
#include <cstring>

#include "raknet/PacketEnumerations.h"
#include "raknet/NetworkTypes.h"

enum MessageType
{
	ID_GENERIC_MESSAGE = ID_RESERVED9 + 1,
	ID_INPUT_UPDATE,
	ID_PHYSIC_UPDATE,
	ID_WIN_NOTIFICATION,
	ID_OPPONENT_DISCONNECTED,
	ID_BALL_RESET,
	ID_BALL_GROUND_COLLISION,
	ID_BALL_PLAYER_COLLISION,
	ID_GAME_READY,
	ID_ENTER_GAME,
	ID_PAUSE,
	ID_UNPAUSE,
	ID_BLOBBY_SERVER_PRESENT,
	ID_OLD_CLIENT,
	ID_UNKNOWN_CLIENT,

};

// General Information:
// 	Because the client may choose their side and the server rotates
// 	everything if necessary, PlayerSide informations may not be
// 	correct on all peers. When the server sends a side information
// 	to a client, the information has to be converted into the view
// 	of the client.

// ID_INPUT_UPDATE:
// 	Description:
// 		This packet is sent from client to server every frame.
// 		It contains the current input state as three booleans.
// 	Structure:
// 		ID_INPUT_UPDATE
// 		ID_TIMESTAMP
// 		timestamp (int)
// 		left keypress (bool)
// 		right keypress (bool)
// 		up keypress (bool)
//
// ID_PHYSIC_UPDATE:
// 	Description:
// 		The server sends this information of the current physics state
// 		to all clients every frame. Local physic states will always
// 		be overwritten.
// 	Structure:
// 		ID_PHYSIC_UPDATE
// 		ID_TIMESTAMP
// 		timestamp (int)
// 		Physic data (analysed by PhysicWorld)
//
// ID_WIN_NOTIFICATION
// 	Description:
// 		Message sent from server to all clients when a player
// 		won the game. The appended enum tells the client which
// 		player won.
// 	Structure:
// 		ID_WIN_NOTIFICATION
// 		winning player (PlayerSide)
//
// ID_BALL_RESET
// 	Description:
// 		 Message sent from server to all clients when the ball
// 		 is reset to the starting position. It includes an information
// 		 about the current point state.
// 	Structure:
// 		ID_BALL_RESET
// 		serving player (PlayerSide)
// 		left score (int)
// 		right score (int)
//
// ID_BALL_GROUND_COLLISION
// 	Description:
// 		Message sent from server to all clients when the ball
// 		hits the ground. It is the only valid reason for a whistle
// 		sound during the game.
// 	Structure:
// 		ID_BALL_GROUND_COLLISION
//
// ID_BALL_PLAYER_COLLISION
// 	Description:
// 		Message sent from server to all clients when the ball
// 		hits a player.  It is the only valid reason for a player
// 		collision sound.
// 	Structure:
// 		ID_BALL_PLAYER_COLLISION
// 		intensity (float)
//
// ID_GAME_READY
// 	Description:
// 		Message sent from server to client when all clients are
// 		ready. The input is enabled after this message on the client.
// 		The attribute opponent name carrys the name of the connected
// 		opponent.
//		GameSpeed attribute tells what is the speed of the game, which
//		is set by the first player.
// 	Structure:
// 		ID_GAME_READY
// 		opponentname(char[16])
//		gameSpeed (float)
//
// ID_ENTER_GAME
// 	Description:
// 		Message sent from client to server after connecting to it.
// 		The side attribute tells the server on which side the client
// 		wants to play. The name attribute reports to players name,
// 		truncated to 16 characters. GameSpeed attribute sets the speed
//		of the game. If you are the second player, this value isn't
//		used on server in any way.
// 	Structure:
// 		ID_ENTER_GAME
// 		side (PlayerSide)
// 		name (char[16])
//		gameSpeed (float)
//
// ID_PAUSE
// 	Description:
// 		Sent from client to server, this message can be seen as a request
// 		to pause the game. From server to client it is an acknowledgement
// 		of the pause and request demand to display an appropriate dialog.
// 	Structure:
// 		ID_PAUSE
//
// ID_UNPAUSE
// 	Description:
// 		As ID_PAUSE, this packets is an acknowledgement if sent from a client
// 		and a request if sent from the server.
// 	Structure:
// 		ID_UNPAUSE
//
// ID_OPPONENTED_DISCONNECTED
// 	Description:
// 		Sent from server to client when an opponent left the game
// 	Structure:
// 		ID_OPPONENT_DISCONNECTED
//
// ID_BLOBBY_SERVER_PRESENT
// 	Description:
// 		Sent from client to probe a server and from server to client
// 		as answer to the same packet.
// 		Sent with version number since alpha 7 in the first case.
// 	Structure:
// 		ID_BLOBBY_SERVER_PRESENT
// 		major (int)
// 		minor (int)
//
// ID_OLD_CLIENT
// 	Description:
// 		Sent from server to client if the version number
// 		is to old or simply missing
// 	Structure:
// 		ID_OLD_CLIENT
//
// ID_UNKNOWN_CLIENT
// 	Description:
// 		Sent from server to client if the version number
// 		is unknown, typically because the client is more
// 		recent than the server
// 	Structure:
// 		ID_UNKNOWN_CLIENT
//

class UserConfig;

struct ServerInfo
{
	ServerInfo(RakNet::BitStream& stream, const char* ip);
	ServerInfo(UserConfig& config);
	ServerInfo(const std::string& playername);
	ServerInfo() {}

	void writeToBitstream(RakNet::BitStream& stream);
	int activegames;
	char hostname[64];
	char name[32];
	char waitingplayer[64];
	char description[192];
};

bool operator == (const ServerInfo& lval, const ServerInfo& rval);

