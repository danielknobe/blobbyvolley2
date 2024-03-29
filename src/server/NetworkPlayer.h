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
#include <memory>

#include "raknet/NetworkTypes.h"
#include "raknet/BitStream.h"
#include "Global.h"
#include "../BlobbyDebug.h"
#include "PlayerIdentity.h"

class NetworkGame;

/*! \brief class for managing an online player
	\details This class manages an online player,
		that is, his identity (name, color, ...),
		his network address and associated game
*/
/// \todo add data to log when last packet arrived
class NetworkPlayer : public ObjectCounter<NetworkPlayer>
{
	public:
		NetworkPlayer();

		NetworkPlayer(PlayerID id, const std::string& name, Color color, PlayerSide side);

		NetworkPlayer(PlayerID id, RakNet::BitStream& stream);

		NetworkPlayer(const NetworkPlayer&) = delete;
		NetworkPlayer& operator=(const NetworkPlayer&) = delete;

		bool valid() const;

		// gets network ID of this player
		const PlayerID& getID() const;
		// gets name of this player
		const std::string& getName() const;
		// gets colour of this player
		Color getColor() const;
		// gets side this player wants to play on
		PlayerSide getDesiredSide() const;
		// gets the complete player identity
		PlayerIdentity getIdentity() const;

		// get game the player currently is in
		const std::shared_ptr<NetworkGame>& getGame() const;

		void setGame(std::shared_ptr<NetworkGame>);

	private:
		/* Network ID */
		PlayerID mID;
		/* Identity */
		PlayerIdentity mIdentity;

		/* Game Data */
		std::shared_ptr<NetworkGame> mGame;

		/* we could add more data such as stats,
			account info, etc later.
		*/
};

