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
#include <boost/shared_ptr.hpp>

#include "raknet/NetworkTypes.h"
#include "Global.h"

class NetworkGame;

/*! \brief class for managing an online player
	\details This class manages an online player,
		that is, his identity (name, color, ...),
		his network address and associated game
*/
/// \todo add data to log when last packet arrived
class NetworkPlayer
{
	public:
		NetworkPlayer();
		
		NetworkPlayer(PlayerID id, const std::string& name, Color color, PlayerSide side);
		// i guess we should! not need to make a copy here
		// but this is saver as this constructor can't mess up other code.
		NetworkPlayer(PlayerID id, RakNet::BitStream stream);
	
		bool valid() const;
		const PlayerID& getID() const;
		const std::string& getName() const;
		const Color& getColor() const;
		PlayerSide getDesiredSide() const;
		const boost::shared_ptr<NetworkGame>& getGame() const;
		
	private:
		PlayerID mID;
		/* Identity */
		std::string mName;
		Color mColor;
		PlayerSide mDesiredSide;
		
		/* Game Data */
		boost::shared_ptr<NetworkGame> mGame;
		
		/* we could add more data such as stats, 
			accoutn info, etc later.
		*/
};

