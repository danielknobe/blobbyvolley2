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
#include "Http.hpp"

/* includes */
#include <unistd.h>

/* implementation */
BlobNet::Layer::Http::Http(const std::string& host, const int& port)
{
	mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	mSocketLayer.Connect(mSocket, inet_addr(mSocketLayer.nameToIP(host.c_str())), port);
}

BlobNet::Layer::Http::~Http()
{
	close(mSocket);
}

void BlobNet::Layer::Http::request(const std::string& path)
{
	mSocketLayer.Write(mSocket, path.c_str(), path.size());
}

