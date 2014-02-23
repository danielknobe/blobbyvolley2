
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

#include <string>
#include <iosfwd>
#include "BlobbyDebug.h"
#include "Global.h"

class DuelMatch;

namespace RakNet
{
	class BitStream;
}

/*! \struct PlayerInput
	\brief struct for easy exchange of a single player input frame
*/
struct PlayerInput
{
	// constructors
	PlayerInput() : left(false), right(false), up(false)
	{
	}

	PlayerInput(bool l, bool r, bool u) : left(l), right(r), up(u)
	{
	}

	// set or get complete input as bits in a byte

	void setAll( unsigned char all );
	unsigned char getAll() const;

	bool operator==(const PlayerInput& other) const;

	// data
	bool left;
	bool right;
	bool up;
};


class PlayerInputAbs
{
	public:
		PlayerInputAbs();
		PlayerInputAbs(RakNet::BitStream& stream);
		PlayerInputAbs(bool l, bool r, bool j);


		// set input
		void setLeft( bool v );
		void setRight( bool v );
		void setJump( bool v);

		void setTarget( short target, PlayerSide player );

		void swapSides();

		// we need some way of getting information about the game, for which we use the match
		// currently.
		PlayerInput toPlayerInput( const DuelMatch* match ) const;

		// send via network
		void writeTo(RakNet::BitStream& stream);


	private:
		enum Flags
		{
			F_LEFT = 1,
			F_RIGHT = 2,
			F_JUMP = 4,
			F_RELATIVE = 8
		};
		unsigned char mFlags;
		short mTarget;
};

// This operator converts a PlayerInput structure in a packed string
// suitable for saving

std::ostream& operator<< (std::ostream& out, const PlayerInput& input);
