#pragma once

#include <PacketEnumerations.h>

enum MessageType
{
	ID_GENERIC_MESSAGE = ID_RESERVED_9 + 1,
	ID_INPUT_UPDATE,
	ID_PHYSIC_UPDATE,
	ID_WIN_NOTIFICATION,
	ID_BALL_RESET,
	ID_BALL_GROUND_COLLISION,
	ID_BALL_PLAYER_COLLISION,
	ID_GAME_READY,
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
// 		left points (char)
// 		right points (char)
//
// ID_BALL_GROUND_COLLISION
// 	Description:
// 		Message sent from server to all clients when the ball
// 		hits the ground. It is the only valid reason for a whistle
// 		sound during the game. The side attribute tells on which side
// 		the ball hit the ground.
// 	Structure:
// 		ID_BALL_GROUND_COLLISION
//		side (PlayerSide)
//
// ID_BALL_PLAYER_COLLISION
// 	Description:
// 		Message sent from server to all clients when the ball
// 		hits a player.  It is the only valid reason for a player
// 		collision sound. The player attribute tells which player
// 		hit the ball
// 	Structure:
// 		ID_BALL_PLAYER_COLLISION
// 		player (PlayerSide)
//
// ID_GAME_READY
// 	Description:
// 		Message sent from server to client when all clients are
// 		ready. The input is enabled after this message on the client.
// 	Structure:
// 		ID_GAME_READY

