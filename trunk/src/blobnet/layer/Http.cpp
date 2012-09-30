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

#include <iostream>
#include <sstream>
#include <typeinfo>
#include <string>
#include <cstdlib>

/* implementation */
BlobNet::Layer::Http::Http(const std::string& hostname, const int& port)
: mHostname(hostname)
, mPort(port)
{
}

BlobNet::Layer::Http::~Http()
{
}


void BlobNet::Layer::Http::request(const std::string& path, std::stringstream& response)
{
	// Create TCP/IP Socket
	SOCKET inOutSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(inOutSocket == INVALID_SOCKET)
	{
		throw BlobNet::Exception::HttpException("Can't create HTTP-Socket.");
	}

	// Connect to the host
	if(mSocketLayer.Connect(inOutSocket, inet_addr(mSocketLayer.nameToIP(mHostname.c_str())), mPort) == -1)
	{
		throw BlobNet::Exception::HttpException("Can't connect to host.");
	};

	// Message for a simple request
	std::string request = "GET /" + path + " HTTP/1.1\r\nHost: " + mHostname + "\r\n\r\n";

	// Write the whole message
	int bytesSend = 0;
	do
	{
		bytesSend += mSocketLayer.Write(inOutSocket, request.c_str(), request.size());
		if(bytesSend == -1)
		{
			throw BlobNet::Exception::HttpException("Can't send the request to host.");
		}
	} while(bytesSend < request.size());

	// Read the header of the response and extract content size
	std::stringstream header;

	readHeader(inOutSocket, header);
	int contentSize = getContentSize(header);

	// Read the body	
	readBody(inOutSocket, response, contentSize);

	close(inOutSocket);
}

void BlobNet::Layer::Http::readHeader(int inOutSocket, std::stringstream& response)
{
	// Parser variables
	State state = something;
	bool done = false;

	// Read header
	for(char c; (!done) && (recv(inOutSocket, &c, 1, 0) > 0); response << c)
	{
		switch(c)
		{
		case '\r':
			if(state == something)
			{
				state = cr;
				break;
			}
			if(state == crlf)
			{
				state = crlfcr;
				break;
			}								
			state = error;
			done = true;
			break;

		case '\n':
			if(state == cr)
			{
				state = crlf;
				break;
			}
			if(state == crlfcr)
			{
				state = headerDone;
				return;
			}
			state = error;
			done = true;
			break;

		default:
			state = something;
			break;
		}
	}
	throw BlobNet::Exception::HttpException("Can't read response.");
}

void BlobNet::Layer::Http::readBody(int inOutSocket, std::stringstream& response, int contentSize)
{
	// Parser variables
	int counter = 0;
	State state = something;

	// Read body
	for(char c; recv(inOutSocket, &c, 1, 0) > 0; response << c)
	{
		counter += 1;
		if(counter == contentSize)
		{
			return;
		}
	}
	throw BlobNet::Exception::HttpException("Can't read response.");
}

int BlobNet::Layer::Http::getContentSize(std::stringstream& response)
{
	// Parser variables
	std::string token;
	State state = something;

	// Data variables
	int contentSize = -1;

	// Parser
	while(response >> token)
	{
		switch(state)
		{
		case something:
			if(token == "Content-Length:")
			{
				state = contentlength;
			}
			break;
		case contentlength:
			state = headerDone;
			return atoi(token.c_str());
		}
	}
	throw BlobNet::Exception::HttpException("Can't get contentsize of http response.");
}

