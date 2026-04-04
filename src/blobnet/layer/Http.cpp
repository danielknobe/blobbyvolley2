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
#if !(defined _WIN32)
#include <unistd.h>
#endif

#include <iostream>
#include <sstream>
#include <string>
#include <utility>


#if (defined _WIN32)
	#define closeSocket(s) closesocket(s)
#else
	#define closeSocket(s) close(s)
#endif


namespace BlobNet
{

namespace Layer
{

/* implementation */
Http::Http(std::string hostname, const int& port)
: mHostname(std::move(hostname))
, mPort(port)
{
}

Http::~Http() = default;


void Http::request(const std::string& path, std::stringstream& response, const HttpRequestHeader& requestHeader)
{
	// Create TCP/IP Socket
	SOCKET inOutSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(inOutSocket == INVALID_SOCKET)
	{
		throw Exception::HttpException("Can't create HTTP-Socket.");
	}

	try {
		// Connect to the host
		char ipAddress[16];
		if (mSocketLayer.nameToIpStrings(mHostname.c_str(), ipAddress, sizeof(ipAddress), 1) == 0)
		{
			throw Exception::HttpException("Can't resolve IP Address.");
		}

		if(mSocketLayer.Connect(inOutSocket, inet_addr(ipAddress), mPort) == -1)
		{
			throw Exception::HttpException("Can't connect to host.");
		}

		// Message for a simple request
		std::string request = "GET /" + path + " HTTP/1.1\r\nHost: " + mHostname + "\r\n";
		request += requestHeader.getHeaderString();
		request += "\r\n"; // End of header

		// Write the whole message
		int bytesSend = 0;
		do
		{
			bytesSend += mSocketLayer.Write(inOutSocket, request.c_str(), request.size());
			if(bytesSend == -1)
			{
				throw Exception::HttpException("Can't send the request to host.");
			}
		} while(bytesSend < request.size());
	}
	catch (const Exception::HttpException& e)
	{
		closeSocket(inOutSocket);
		throw e;
	}

	// Read the header of the response and extract content size
	std::stringstream header;

	readHeader(inOutSocket, header);
	int contentSize = getContentSize(header);

	// Read the body
	readBody(inOutSocket, response, contentSize);

	closeSocket(inOutSocket);
}

void Http::readHeader(int inOutSocket, std::stringstream& response)
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
	throw Exception::HttpException("Can't read response.");
}

void Http::readBody(int inOutSocket, std::stringstream& response, int contentSize)
{
	// Read body
	for(char c; (contentSize != 0) && (recv(inOutSocket, &c, 1, 0) > 0); response << c, contentSize--)
	{
	}

	if (contentSize != 0) 
	{
		throw Exception::HttpException("Can't read all bytes of response declared in headers content-length field.");
	}
}

int Http::getContentSize(std::stringstream& response)
{
	// Parser variables
	std::string token;
	State state = something;

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
			return std::stoi(token);
		default:
			break;
		}
	}
	throw Exception::HttpException("Can't get contentsize of http response.");
}

}
}
