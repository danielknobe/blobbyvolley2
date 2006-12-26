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

#include "NetworkMessage.h"
#include "UserConfig.h"

#include <cstring>

ServerInfo::ServerInfo(RakNet::BitStream& stream, const char* ip)
{
	strncpy(hostname, ip, sizeof(hostname));
	hostname[sizeof(hostname)] = 0;

	stream.Read(activegames);
	stream.Read(name, sizeof(name));
	stream.Read(waitingplayer, sizeof(waitingplayer));
	stream.Read(description, sizeof(description));
}

ServerInfo::ServerInfo(UserConfig& config)
{
	memset(this, 0, sizeof(ServerInfo));
	std::string tmp;
	tmp = config.getString("name");
	strncpy(name, tmp.c_str(), sizeof(name) - 1);
	tmp = config.getString("description");
	strncpy(description, tmp.c_str(), sizeof(description) - 1);
}

ServerInfo::ServerInfo(const std::string& playername)
{
	memset(this, 0, sizeof(ServerInfo));
	strncpy(name, std::string(playername + "'s game").c_str(), sizeof(name) - 1);
	strncpy(description, "client hosted game", sizeof(description) - 1);
}

void ServerInfo::writeToBitstream(RakNet::BitStream& stream)
{
	stream.Write(activegames);
	stream.Write(name, sizeof(name));
	stream.Write(waitingplayer, sizeof(waitingplayer));
	stream.Write(description, sizeof(description));
}

bool operator == (const ServerInfo& lval, const ServerInfo& rval)
{
	return !strncmp(lval.hostname, rval.hostname,
			sizeof(lval.hostname));
}

