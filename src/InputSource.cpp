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
#include "InputSource.h"

/* includes */
#include <ostream>
#include <cassert>

#include "DuelMatch.h"
#include "GameConstants.h"

/* implementation */

/* PlayerInputAbs */

PlayerInputAbs::PlayerInputAbs() : mFlags( F_RELATIVE ), mTarget(-1)
{

}

PlayerInputAbs::PlayerInputAbs(bool l, bool r, bool j) : mFlags( F_RELATIVE ), mTarget(-1)
{
	setLeft(l);
	setRight(r);
	setJump(j);
}


// set input
void PlayerInputAbs::setLeft( bool v )
{
	if(v)
		mFlags |= F_LEFT;
	else
		mFlags &= ~F_LEFT;
}

void PlayerInputAbs::setRight( bool v )
{
	if(v)
		mFlags |= F_RIGHT;
	else
		mFlags &= ~F_RIGHT;
}

void PlayerInputAbs::setJump( bool v)
{
	if(v)
		mFlags |= F_JUMP;
	else
		mFlags &= ~F_JUMP;
}

void PlayerInputAbs::setTarget( short target, PlayerSide player )
{
	mFlags &= F_JUMP;	// reset everything but the jump flag, i.e. no left/right and no relative
	mTarget = target;

	if(player == LEFT_PLAYER )
	{
		setLeft(true);
	}
	if(player == RIGHT_PLAYER )
	{
		setRight(true);
	}
}

PlayerInput PlayerInputAbs::toPlayerInput( const DuelMatch* match ) const
{
	if( mFlags & F_RELATIVE)
		return PlayerInput( mFlags & F_LEFT, mFlags & F_RIGHT, mFlags & F_JUMP );
	else
	{
		bool left = false;
		bool right = false;

		PlayerSide side = mFlags & F_LEFT ? LEFT_PLAYER : RIGHT_PLAYER;

		// here we load the current position of the player.
		float blobpos = match->getBlobPosition(side).x;

		if (blobpos + BLOBBY_SPEED * 2 <= mTarget)
			right = true;
		else if (blobpos - BLOBBY_SPEED * 2 >= mTarget)
			left = true;
		return PlayerInput( left, right, mFlags & F_JUMP );
	}

}


/* InputSource */

InputSource::InputSource() : mInput(), mMatch(0)
{
}

PlayerInput InputSource::getInput() const
{
	return mInput.toPlayerInput( this->getMatch() );;
}

PlayerInput InputSource::updateInput()
{
	mInput = getNextInput();
	return getInput();
}

void InputSource::setInput(PlayerInput ip)
{
	mInput = PlayerInputAbs(ip.left, ip.right, ip.up);
}

PlayerInputAbs InputSource::getNextInput()
{
	return mInput;
}

const DuelMatch* InputSource::getMatch() const
{
	return mMatch;
}

void InputSource::setMatch(const DuelMatch* match)
{
	assert(mMatch == 0);
	mMatch = match;
}

std::ostream& operator<< (std::ostream& out, const PlayerInput& input)
{
	out << (input.left ? 't' : 'f') << (input.right ? 't' : 'f') << (input.up ? 't' : 'f');
	return out;
}
