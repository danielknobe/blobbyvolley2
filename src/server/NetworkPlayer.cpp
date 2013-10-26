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
#include "NetworkPlayer.h"

/* includes */

/* implementation */

// initialise NetworkPlayer. Set NetworkID to 0.0.0.0:0, so we are sure no player
// will ever have this.
NetworkPlayer::NetworkPlayer() : mID(), mIdentity()
{
	mID.binaryAddress = 0;
	mID.port = 0;
}

NetworkPlayer::NetworkPlayer(PlayerID id, const std::string& name, Color color, PlayerSide side)
:mID(id), mIdentity(name, color, false, side)
{

}

NetworkPlayer::NetworkPlayer(PlayerID id, RakNet::BitStream stream) : mID(id)
{
	int playerSide;
	stream.Read(playerSide);

	// Read the Playername
	char charName[16];
	stream.Read(charName, sizeof(charName));

	// ensures that charName is null terminated
	charName[sizeof(charName)-1] = '\0';

	// read colour data
	int color;
	stream.Read(color);

	mIdentity = PlayerIdentity(charName, color, false, (PlayerSide)playerSide);
}

bool NetworkPlayer::valid() const
{
	return mID.port != 0;
}

const PlayerID& NetworkPlayer::getID() const
{
	return mID;
}

const std::string& NetworkPlayer::getName() const
{
	return mIdentity.getName();
}
Color NetworkPlayer::getColor() const
{
	return mIdentity.getStaticColor();
}

PlayerSide NetworkPlayer::getDesiredSide() const
{
	return mIdentity.getPreferredSide();
}

const boost::shared_ptr<NetworkGame>& NetworkPlayer::getGame() const
{
	return mGame;
}

void NetworkPlayer::setGame(boost::shared_ptr<NetworkGame> game)
{
	mGame = game;
}
