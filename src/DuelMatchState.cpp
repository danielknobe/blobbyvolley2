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
#include "DuelMatchState.h"

/* includes */
#include "raknet/BitStream.h"

#include "GameConstants.h"
#include "GenericIO.h"


/* implementation */
void DuelMatchState::swapSides()
{
	worldState.swapSides();
	logicState.swapSides();
	
	std::swap(playerInput[LEFT_PLAYER].left, playerInput[LEFT_PLAYER].right);
	std::swap(playerInput[RIGHT_PLAYER].left, playerInput[RIGHT_PLAYER].right);
	std::swap(playerInput[LEFT_PLAYER], playerInput[RIGHT_PLAYER]);
	
	switch (errorSide)
	{
		case LEFT_PLAYER:
			errorSide = RIGHT_PLAYER;
			break;
		case RIGHT_PLAYER:
			errorSide = LEFT_PLAYER;
			break;
	}
}

USER_SERIALIZER_IMPLEMENTATION_HELPER(DuelMatchState)
{
	io.template generic<PhysicState> (value.worldState);
	io.template generic<GameLogicState> (value.logicState);
	
	// the template keyword is needed here so the compiler knows generic is
	// a template function and does not complain about <>.
	io.template generic<PlayerInput> ( value.playerInput[LEFT_PLAYER] );
	io.template generic<PlayerInput> ( value.playerInput[RIGHT_PLAYER] );
	
	io.byte(value.errorSide);
}

bool DuelMatchState::operator==(const DuelMatchState& other) const
{
	return worldState == other.worldState  &&  logicState == other.logicState &&
		playerInput[LEFT_PLAYER] == other.playerInput[LEFT_PLAYER] && 
		playerInput[RIGHT_PLAYER] == other.playerInput[RIGHT_PLAYER] &&
		errorSide == other.errorSide;
}
