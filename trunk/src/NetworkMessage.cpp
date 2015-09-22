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
#include <ostream>

#include "UserConfig.h"
#include "PlayerIdentity.h"

/* implementation */
ServerInfo::ServerInfo(RakNet::BitStream& stream, const char* ip, uint16_t p)
{
	strncpy(hostname, ip, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = 0;

	port = p;

	stream.Read(activegames);
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
	port = config.getInteger("port", BLOBBY_PORT);
}

ServerInfo::ServerInfo(const std::string& playername)
{
	memset(this, 0, sizeof(ServerInfo));

	std::strncpy(hostname, "localhost", sizeof(hostname));
	port = BLOBBY_PORT;

	std::strncpy(name, std::string(playername + "'s game").c_str(), sizeof(name) - 1);
	std::strncpy(description, "client hosted game", sizeof(description) - 1);
}

void ServerInfo::writeToBitstream(RakNet::BitStream& stream)
{
	stream.Write(activegames);
	stream.Write(name, sizeof(name));
	stream.Write(waitingplayers);
	stream.Write(description, sizeof(description));
	assert( stream.GetNumberOfBytesUsed() == BLOBBY_SERVER_PRESENT_PACKET_SIZE);
}

const size_t ServerInfo::BLOBBY_SERVER_PRESENT_PACKET_SIZE = sizeof((unsigned char)ID_BLOBBY_SERVER_PRESENT)
		+ 2 * sizeof(int) 	// activegames &  waiting players
		+ 32				// name
		+ 192;			// description



RakNet::BitStream makeEnterServerPacket( const PlayerIdentity& player )
{
	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_ENTER_SERVER);

	// Send preferred side
	stream.Write( player.getPreferredSide() );

	// Send playername
	char myname[16];
	strncpy(myname, player.getName().c_str(), sizeof(myname));
	stream.Write(myname, sizeof(myname));

	// send color settings
	stream.Write(player.getStaticColor().toInt());
	return stream;
}



bool operator == (const ServerInfo& lval, const ServerInfo& rval)
{
	// check if ip and port are identical!
	/// \todo maybe we should user raknets pladerId directly?
	return lval.port == rval.port && !strncmp(lval.hostname, rval.hostname,
			sizeof(lval.hostname));
}

std::ostream& operator<<(std::ostream& stream, const ServerInfo& val)
{
	return stream << val.name << " (" << val.hostname << ":" << val.port << ")";
}

