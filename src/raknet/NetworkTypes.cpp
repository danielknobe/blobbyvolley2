/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Unique Player Identifier Class implementation
 *
 * Copyright (c) 2003, Rakkarsoft LLC and Kevin Jenkins
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "NetworkTypes.h"
#include "BitStream.h"
#include <boost/lexical_cast.hpp>

int operator==( const PlayerID& left, const PlayerID& right )
{
	return left.binaryAddress == right.binaryAddress && left.port == right.port;
}

int operator!=( const PlayerID& left, const PlayerID& right )
{
	return left.binaryAddress != right.binaryAddress || left.port != right.port;
}

int operator>( const PlayerID& left, const PlayerID& right )
{
	return ( ( left.binaryAddress > right.binaryAddress ) || ( ( left.binaryAddress == right.binaryAddress ) && ( left.port > right.port ) ) );
}

int operator<( const PlayerID& left, const PlayerID& right )
{
	return ( ( left.binaryAddress < right.binaryAddress ) || ( ( left.binaryAddress == right.binaryAddress ) && ( left.port < right.port ) ) );
}

std::string PlayerID::toString() const
{
	auto tmp = binaryAddress;
	std::string ip =  boost::lexical_cast<std::string>(tmp & 0xFF);
	tmp >>= 8;
	ip =  boost::lexical_cast<std::string>(tmp & 0xFF) + "." + ip;
	tmp >>= 8;
	ip =  boost::lexical_cast<std::string>(tmp & 0xFF) + "." + ip;
	tmp >>= 8;
	ip =  boost::lexical_cast<std::string>(tmp & 0xFF) + "." + ip;

	return ip + ":" +  boost::lexical_cast<std::string>(port);
}

std::ostream& operator<<(std::ostream& stream, const PlayerID& p)
{
	return stream << p.toString();
}

RakNet::BitStream Packet::getStream() const
{
	return RakNet::BitStream((char*)data, BITS_TO_BYTES(bitSize), false);
}
