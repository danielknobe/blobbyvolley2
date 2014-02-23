
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
	PlayerInput()
	{
		left = false;
		right = false;
		up = false;
	}

	PlayerInput(const std::string& string);

	PlayerInput(bool l, bool r, bool u)
	{
		left = l;
		right = r;
		up = u;
	}

	void set(bool l, bool r, bool u)
	{
		left = l;
		right = r;
		up = u;
	}

	void set( unsigned char all )
	{
		left = all & 4;
		right = all & 2;
		up =  all & 1;
	}

	void swap()
	{
		bool tmp = left;
		left = right;
		right = tmp;
	}

	bool operator==(const PlayerInput& other) const
	{
		return left == other.left && right == other.right && up == other.up;
	}

	bool operator!=(const PlayerInput& other) const
	{
		return !(*this == other);
	}

	unsigned char getAll() const
	{
		unsigned char c = 0;
		c = (left ? 4 : 0) + (right ? 2 : 0) + (up ? 1 : 0);
		return c;
	}

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
