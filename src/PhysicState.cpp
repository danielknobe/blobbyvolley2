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

/* header include */
#include "PhysicState.h"

/* includes */
#include <limits>

#include "raknet/BitStream.h"

#include "GameConstants.h"
#include "GenericIO.h"

bool PhysicState::blobbyHitGround(PlayerSide player) const
{
	return blobPosition[player].y >= GROUND_PLANE_HEIGHT;
}

USER_SERIALIZER_IMPLEMENTATION_HELPER(PhysicState)
{
	io.number( value.blobPosition[LEFT_PLAYER].x );
	io.number( value.blobPosition[LEFT_PLAYER].y );
	
	io.number( value.blobVelocity[LEFT_PLAYER].x );
	io.number( value.blobVelocity[LEFT_PLAYER].y );
	
	io.number( value.blobPosition[RIGHT_PLAYER].x );
	io.number( value.blobPosition[RIGHT_PLAYER].y );	
	
	io.number( value.blobVelocity[RIGHT_PLAYER].x );
	io.number( value.blobVelocity[RIGHT_PLAYER].y );
	
	io.number( value.ballPosition.x );
	io.number( value.ballPosition.y );	
	
	io.number( value.ballVelocity.x );
	io.number( value.ballVelocity.y );
	
	io.number( value.ballAngularVelocity );
	
	io.boolean( value.isGameRunning );
	io.boolean( value.isBallValid );
	
	// the template keyword is needed here so the compiler knows generic is
	// a template function and does not complain about <>.
	io.template generic<PlayerInput> ( value.playerInput[LEFT_PLAYER] );
	io.template generic<PlayerInput> ( value.playerInput[RIGHT_PLAYER] );
	
}

void writeCompressedToBitStream(RakNet::BitStream* stream, float value, float min, float max)
{
	assert(min <= value && value <= max);
	assert(stream);
	unsigned short only2bytes = static_cast<unsigned short>((value - min) / (max - min) * std::numeric_limits<unsigned short>::max());
	stream->Write(only2bytes);
}

void readCompressedFromBitStream(RakNet::BitStream* stream, float& value, float min, float max)
{
	unsigned short only2bytes;
	stream->Read(only2bytes);
	value = static_cast<float>(only2bytes) / static_cast<float>(std::numeric_limits<unsigned short>::max()) * (max - min) + min;
}

/* implementation */
void PhysicState::writeToStream(RakNet::BitStream* stream) const
{
	// if the blobbys are standing on the ground, we need not send
	// y position and velocity
	stream->Write(blobbyHitGround(LEFT_PLAYER));
	stream->Write(blobbyHitGround(RIGHT_PLAYER));
	
	if(!blobbyHitGround(LEFT_PLAYER))
	{
		writeCompressedToBitStream(stream, blobPosition[LEFT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, blobVelocity[LEFT_PLAYER].y, -30, 30);
	}
	
	if(!blobbyHitGround(RIGHT_PLAYER))
	{
		writeCompressedToBitStream(stream, blobPosition[RIGHT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, blobVelocity[RIGHT_PLAYER].y, -30, 30);
	}
	
	writeCompressedToBitStream(stream, blobPosition[LEFT_PLAYER].x, LEFT_PLANE, NET_POSITION_X);
	writeCompressedToBitStream(stream, blobPosition[RIGHT_PLAYER].x, NET_POSITION_X, RIGHT_PLANE);
	
	writeCompressedToBitStream(stream, ballPosition.x, LEFT_PLANE, RIGHT_PLANE);
	writeCompressedToBitStream(stream, ballPosition.y, -500, GROUND_PLANE_HEIGHT_MAX);

	writeCompressedToBitStream(stream, ballVelocity.x, -30, 30);
	writeCompressedToBitStream(stream, ballVelocity.y, -30, 30);

	stream->Write(playerInput[LEFT_PLAYER].left);
	stream->Write(playerInput[LEFT_PLAYER].right);
	stream->Write(playerInput[LEFT_PLAYER].up);
	stream->Write(playerInput[RIGHT_PLAYER].left);
	stream->Write(playerInput[RIGHT_PLAYER].right);
	stream->Write(playerInput[RIGHT_PLAYER].up);
}

void PhysicState::readFromStream(RakNet::BitStream* stream)
{
	bool leftGround;
	bool rightGround;
	stream->Read(leftGround);
	stream->Read(rightGround);
	
	if(leftGround)
	{
		blobPosition[LEFT_PLAYER].y = GROUND_PLANE_HEIGHT;
		blobVelocity[LEFT_PLAYER].y = 0;
	}
	else
	{
		readCompressedFromBitStream(stream, blobPosition[LEFT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		readCompressedFromBitStream(stream, blobVelocity[LEFT_PLAYER].y, -30, 30);
	}
	
	if(rightGround)
	{
		blobPosition[RIGHT_PLAYER].y = GROUND_PLANE_HEIGHT;
		blobVelocity[RIGHT_PLAYER].y = 0;
	}
	else
	{
		readCompressedFromBitStream(stream, blobPosition[RIGHT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		readCompressedFromBitStream(stream, blobVelocity[RIGHT_PLAYER].y, -30, 30);
	}
	
	readCompressedFromBitStream(stream, blobPosition[LEFT_PLAYER].x, LEFT_PLANE, NET_POSITION_X);
	readCompressedFromBitStream(stream, blobPosition[RIGHT_PLAYER].x, NET_POSITION_X, RIGHT_PLANE);
	
	readCompressedFromBitStream(stream, ballPosition.x, LEFT_PLANE, RIGHT_PLANE);
	// maybe these values is a bit too pessimistic...
	// but we have 65535 values, hence it should be precise enough
	readCompressedFromBitStream(stream, ballPosition.y, -500, GROUND_PLANE_HEIGHT_MAX);

	readCompressedFromBitStream(stream, ballVelocity.x, -30, 30);
	readCompressedFromBitStream(stream, ballVelocity.y, -30, 30);
	
	// if ball velocity not zero, we must assume that the game is active
	// i'm not sure if this would be set correctly otherwise...
	// we must use this check with 0.1f because of precision loss when velocities are transmitted
	// wo prevent setting a false value when the ball is at the parabels top, we check also if the 
	// y - position is the starting y position
	/// \todo maybe we should simply send a bit which contains this information? 
	if( std::abs(ballVelocity.x) > 0.1f || std::abs(ballVelocity.y) > 0.1f || std::abs(ballPosition.y - STANDARD_BALL_HEIGHT) > 0.1f) {
		isGameRunning = true;
	} else {
		isGameRunning = false;
	}

	stream->Read(playerInput[LEFT_PLAYER].left);
	stream->Read(playerInput[LEFT_PLAYER].right);
	stream->Read(playerInput[LEFT_PLAYER].up);
	stream->Read(playerInput[RIGHT_PLAYER].left);
	stream->Read(playerInput[RIGHT_PLAYER].right);
	stream->Read(playerInput[RIGHT_PLAYER].up);

}

void PhysicState::swapSides()
{
	blobPosition[LEFT_PLAYER].x = RIGHT_PLANE - blobPosition[LEFT_PLAYER].x;
	blobPosition[RIGHT_PLAYER].x = RIGHT_PLANE - blobPosition[RIGHT_PLAYER].x;
	std::swap(blobPosition[LEFT_PLAYER], blobPosition[RIGHT_PLAYER]);
	
	ballPosition.x = RIGHT_PLANE - ballPosition.x;
	ballVelocity.x = -ballVelocity.x;
	ballAngularVelocity = -ballAngularVelocity;
	
	std::swap(playerInput[LEFT_PLAYER].left, playerInput[LEFT_PLAYER].right);
	std::swap(playerInput[RIGHT_PLAYER].left, playerInput[RIGHT_PLAYER].right);
	std::swap(playerInput[LEFT_PLAYER], playerInput[RIGHT_PLAYER]);
}

bool PhysicState::operator==(const PhysicState& other) const
{
	return
		blobPosition[LEFT_PLAYER] == other.blobPosition[LEFT_PLAYER] && 
		blobPosition[RIGHT_PLAYER] == other.blobPosition[RIGHT_PLAYER] &&
		blobVelocity[LEFT_PLAYER] == other.blobVelocity[LEFT_PLAYER] && 
		blobVelocity[RIGHT_PLAYER] == other.blobVelocity[RIGHT_PLAYER] &&
		ballPosition == other.ballPosition &&
		ballVelocity == other.ballVelocity &&
		ballAngularVelocity == other.ballAngularVelocity &&
		isGameRunning == other.isGameRunning &&
		isBallValid == other.isBallValid &&
		playerInput[LEFT_PLAYER] == other.playerInput[LEFT_PLAYER] && 
		playerInput[RIGHT_PLAYER] == other.playerInput[RIGHT_PLAYER];
}
