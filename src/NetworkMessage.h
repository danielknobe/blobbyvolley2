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

#include <string>
#include <stdint.h>

#include "raknet/PacketEnumerations.h"
#include "raknet/NetworkTypes.h"
#include "raknet/BitStream.h"
#include "BlobbyDebug.h"

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
	ID_VERSION_MISMATCH,
	ID_CURRENTLY_UNUSED,	// this value is to ensure network protocol compatibility between 0.9c and 1.0
	ID_REPLAY,
	ID_CHAT_MESSAGE,
	ID_UPDATE_SCORE,
	ID_RULES_CHECKSUM,
	ID_RULES
};

// General Information:
// 	Because the client may choose their side and the server rotates
// 	everything if necessary, PlayerSide informations may not be
// 	correct on all peers. When the server sends a side information
// 	to a client, the information has to be converted into the view
// 	of the client.

// ID_INPUT_UPDATE = 63:
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
//		packet_number (unsigned char)
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
// 		 about the current point state and is used to synchronize
//		 the clocks.
// 	Structure:
// 		ID_BALL_RESET
// 		serving player (PlayerSide)
// 		left score (int)
// 		right score (int)
//		time (int)
//
// ID_BALL_GROUND_COLLISION
// 	Description:
// 		Message sent from server to all clients when the ball
// 		hits the ground. It is the only valid reason for a whistle
// 		sound during the game. The side attribute tells on which side
//		the ball hit the ground.
// 	Structure:
// 		ID_BALL_GROUND_COLLISION
//		side (PlayerSide)
//
// ID_BALL_PLAYER_COLLISION
// 	Description:
// 		Message sent from server to all clients when the ball
// 		hits a player.  It is the only valid reason for a player
// 		collision sound. The player attribute tells which player
//		hit the ball.
// 	Structure:
// 		ID_BALL_PLAYER_COLLISION
// 		intensity (float)
//		player (PlayerSide)
//
// ID_GAME_READY
// 	Description:
// 		Message sent from server to client when all clients are
// 		ready. The input is enabled after this message on the client.
// 		The attribute opponent name carrys the name of the connected
// 		opponent.
// 	Structure:
// 		ID_GAME_READY
//		gamespeed (int)
// 		opponent name (char[16])
//		opponent color (int)
//
// ID_ENTER_GAME
// 	Description:
// 		Message sent from client to server after connecting to it.
// 		The side attribute tells the server on which side the client
// 		wants to play. The name attribute reports to players name,
// 		truncated to 16 characters.
// 	Structure:
// 		ID_ENTER_GAME
// 		side (PlayerSide)
// 		name (char[16])
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
// ID_VERSION_MISMATCH
// 	Description:
// 		Sent from server to client if the version number
// 		differes from the one of the server.
// 	Structure:
// 		ID_VERSION_MISMATCH
//		server_major (int)
//		server_minor (int)
//
// ID_REPLAY
// 	Description:
// 		Sent from client to server to request a replay
// 		Sent from server to client to transmitt the replay
// 	Structure:
// 		ID_REPLAY
//		size (int)
//		data
//
// ID_UPDATE_SCORE
// 	Description:
// 		 Message sent from server to all clients when the score
// 		 was changed. It includes an informarmation about new score
// 		 and is used to synchronize the score.
// 	Structure:
// 		ID_UPDATE_SCORE
// 		left score (int)
// 		right score (int)
//		time (int)
//
// ID_RULES_CHECKSUM
// 	Description:
// 		Sent from server to client to tell rules file checksum
// 		Client should send ID_RULES after receiving ID_RULES_CHECKSUM
// 			to tell server if he needs rules file transmitting
// 	Structure:
// 		ID_RULES_CHECKSUM
//		checksum (int)
//
// ID_RULES
// 	Description:
// 		Sent from client to server to request a rules file
// 		Sent from server to client to transmit the rules file
// 		Game is starting only after transmitting a rules file
// 	Structure (from client ro server):
// 		ID_RULES
//		needRules (bool)
// 	Structure (from server ro client):
// 		ID_RULES
//		size (int)
//		data
//

class UserConfig;

struct ServerInfo : public ObjectCounter<ServerInfo>
{
	ServerInfo(RakNet::BitStream& stream, const char* ip, uint16_t port);
	ServerInfo(const UserConfig& config);
	ServerInfo(const std::string& playername);

	void writeToBitstream(RakNet::BitStream& stream);

	void setWaitingPlayer(const std::string& name);

	/// \todo maybe we should define ServerInfo a little bit more
	///			as e.g., hostname can be left uninitialised on server
	/// we combine to functionsalities here: server information and server addresses.
	int activegames;
	int gamespeed;
	uint16_t port;
	char hostname[64];
	char name[32];
	char waitingplayer[64];
	char description[192];

	static const size_t BLOBBY_SERVER_PRESENT_PACKET_SIZE;
};

bool operator == (const ServerInfo& lval, const ServerInfo& rval);

