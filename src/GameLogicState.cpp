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

#include "GameLogicState.h"

#include <ostream>

#include "GenericIO.h"

USER_SERIALIZER_IMPLEMENTATION_HELPER(GameLogicState)
{
	io.uint32(value.leftScore);
	io.uint32(value.rightScore);
	io.template generic<PlayerSide>( value.servingPlayer );
	io.uint32(value.leftSquish);
	io.uint32(value.rightSquish);
	io.uint32(value.squishWall);
	io.uint32(value.squishGround);
	io.boolean(value.isGameRunning);
	io.boolean(value.isBallValid);
}

void GameLogicState::swapSides()
{
	std::swap(leftScore, rightScore);
	std::swap(leftSquish, rightSquish);
	
	if(servingPlayer == LEFT_PLAYER)
	{
		servingPlayer = RIGHT_PLAYER;
	}
	 else if(servingPlayer == RIGHT_PLAYER)
	{
		servingPlayer = LEFT_PLAYER;
	}
}

bool GameLogicState::operator==(const GameLogicState& other) const
{
	return leftScore == other.leftScore && rightScore == other.rightScore &&
			isGameRunning == other.isGameRunning && isBallValid == other.isBallValid &&
			servingPlayer == other.servingPlayer && leftSquish == other.leftSquish &&
			rightSquish == other.rightSquish && squishWall == other.squishWall &&
			squishGround == other.squishGround;
}

std::ostream& operator<<(std::ostream& stream, const GameLogicState& state)
{
	stream << "GAME LOGIC STATE [ " << state.leftScore << " : " << state.rightScore << "  " << state.servingPlayer
			<< "  " << state.leftSquish << " " << state.rightSquish << "  " << state.squishWall 
			<< "  " << state.squishGround << "  " << state.isGameRunning << "  " << state.isBallValid << "]";
	return stream;
}
