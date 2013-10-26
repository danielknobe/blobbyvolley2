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
#include "NetworkMessage.h"

/* includes */
#include <cstring>

#include "UserConfig.h"
#include "SpeedController.h"

/* implementation */
ServerInfo::ServerInfo(RakNet::BitStream& stream, const char* ip, uint16_t p)
{
	strncpy(hostname, ip, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = 0;

	port = p;

	stream.Read(activegames);
	stream.Read(gamespeed);
	stream.Read(name, sizeof(name));
	stream.Read(waitingplayers);
	stream.Read(description, sizeof(description));
}

ServerInfo::ServerInfo(const IUserConfigReader& config)
{
	/// \todo we only need a config reader here!

	// default values
	std::string n = "Blobby Volley 2 Server";
	std::string d = "no description available";

	memset(this, 0, sizeof(ServerInfo));
	std::string tmp;
	tmp = config.getString("name", n);
	strncpy(name, tmp.c_str(), sizeof(name) - 1);
	tmp = config.getString("description", d);
	strncpy(description, tmp.c_str(), sizeof(description) - 1);
	gamespeed = config.getInteger("speed", 75);
	/// \todo maybe we should check if that's a reasonable value, too.
	if (gamespeed < 20 || gamespeed > 200)
		gamespeed = 75;

	port = config.getInteger("port", BLOBBY_PORT);
}

ServerInfo::ServerInfo(const std::string& playername)
{
	memset(this, 0, sizeof(ServerInfo));

	std::strncpy(hostname, "localhost", sizeof(hostname));
	port = BLOBBY_PORT;

	std::strncpy(name, std::string(playername + "'s game").c_str(), sizeof(name) - 1);
	std::strncpy(description, "client hosted game", sizeof(description) - 1);
	gamespeed = (int)SpeedController::getMainInstance()->getGameSpeed();
}

void ServerInfo::writeToBitstream(RakNet::BitStream& stream)
{
	stream.Write(activegames);
	stream.Write(gamespeed);
	stream.Write(name, sizeof(name));
	stream.Write(waitingplayers);
	stream.Write(description, sizeof(description));
}

const size_t ServerInfo::BLOBBY_SERVER_PRESENT_PACKET_SIZE = sizeof((unsigned char)ID_BLOBBY_SERVER_PRESENT)
		+ 2 * sizeof(int) 	// activegames & gamespeed
		+ 32				// name
		+ sizeof(int)		// waiting players
		+ 192;				// description



bool operator == (const ServerInfo& lval, const ServerInfo& rval)
{
	// check if ip and port are identical!
	/// \todo maybe we should user raknets pladerId directly?
	return lval.port == rval.port && !strncmp(lval.hostname, rval.hostname,
			sizeof(lval.hostname));
}


