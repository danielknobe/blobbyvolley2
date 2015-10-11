/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
This program is free software; you can redistribute it and/or modify
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#include <cstdint>
#include <cassert>
#include <string>
#include <array>
#include <iterator>
#include <iostream>

#include "base64.h"


// helper functions and constants
constexpr const char translation_table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvcxyz"
	"0123456789+/";

// this gives the length of a constexpr string literal.
template< std::size_t N >
constexpr std::size_t length( char const (&)[N] )
{
	return N-1;
};

// checks whether a given character belongs to a valid base64 block
constexpr bool is_valid( char c )
{
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <='z') || ('0' <= c && c <='9') || c == '+' || c == '/';
}

// find table entry for a byte
template<uint8_t N>
constexpr uint8_t find_in_table(uint8_t entry)
{
	return is_valid(entry) ? translation_table[N] == entry ? N : find_in_table<N-1>(entry) : -1;
}
template<>
constexpr uint8_t find_in_table<0>(uint8_t entry) { return 0; };

template<uint8_t...Is>
struct index_seq
{
};

template<uint8_t N>
struct make_index_sq
{
	template<typename>	struct append;

	template<uint8_t... ints>
	struct append<index_seq<ints...>>
	{
		using type = index_seq<ints..., N-1>;
	};

	using type = typename append<typename make_index_sq<N-1>::type>::type;
};

template<>
struct make_index_sq<1u>
{
	using type = index_seq<0u>;
};

template<class T>
struct make_table_helper {};

template<uint8_t... seq>
struct make_table_helper<index_seq<seq...>>
{
	static constexpr auto make() -> std::array<uint8_t, sizeof...(seq)>
	{
		return {{find_in_table<length(translation_table)>(seq)...}};
	}
};




// generate the decoding table
constexpr auto decoding_table = make_table_helper<typename make_index_sq<255>::type>::make();

constexpr uint8_t decode( uint8_t byte )
{
    return is_valid(byte) ? decoding_table[byte] : -1;
}

// the bitmask for slicing the three bytes into subelements
constexpr std::uint64_t bitmask = (1 << 6) - 1;

static_assert( length(translation_table) == 64, "error: need 64 characters in the translation table" );

// -------------------------------------------------------------------
//		coding functions
// -------------------------------------------------------------------

// simple encoding function for 3 bytes
void encode( const std::uint8_t*& byte_array, std::string::iterator& writer )
{
	const uint64_t* bits = reinterpret_cast<const uint64_t*>(byte_array);
	for(int i = 0; i < 4; ++i)
	{
		unsigned index = bitmask & (*bits >> (6*i));
		assert(index < 64);
		*writer = translation_table[index];
		++writer;
	}
	std::advance(byte_array, 3);
}

std::string encode( const char* begin, const char* end, int newlines)
{
	// allocate enough space inside the string
	const std::size_t length = end - begin;
	const unsigned tail 	 = length % 3;
	const unsigned extra     = (tail != 0) ? 1 : 0;
	const unsigned groups 	 = length / 3;
	const unsigned total_data= (groups + extra) * 4 + tail;
	newlines -= newlines % 4;
	const unsigned linefeeds = newlines > 0 ? total_data / newlines : 0;

	std::string buffer(total_data + linefeeds, '=');

	// iterate over sequence
	auto write = buffer.begin();
	auto read  = reinterpret_cast<const std::uint8_t*>(begin);
	int last_newline = 0;
	for(unsigned i = 0; i < groups; ++i)
	{
		encode(read, write);
		if( newlines > 0 && (i+1) * 4 >= last_newline + newlines )
		{
			*write = '\n';
			write++;
			last_newline = (i+1)*4;
		}
	}

	// add the partial group
	if(extra)
	{
		uint64_t copy = 0;
		uint8_t* bits = reinterpret_cast<uint8_t*>(&copy);
		for(unsigned i = 0; i < tail; ++i)
			bits[i] = *(read++);

		const uint8_t* cp = bits;
		encode(cp, write);
	}

	assert( (char*)read == end );
	assert( write == buffer.end() - tail );

	return buffer;
}

void decode( std::uint8_t*& byte_array, std::string::const_iterator& reader )
{
	uint64_t* bits = reinterpret_cast<uint64_t*>(byte_array);
	*bits &= 0;
	for(int i = 0; i < 4; ++i)
	{
		uint8_t value = decode(*(reader++));
		assert( (value & (~bitmask)) == 0 ); // check that it is valid
		*bits |= value << (6*i);
	}
	std::advance(byte_array, 3);
}


std::vector<uint8_t> decode(const std::string& data )
{
    // pre-allocate buffer
    std::vector<uint8_t> buffer( data.size() / 4 * 3 + 4 );
    uint8_t* iter = buffer.data();
    auto reader = data.cbegin();

	// read block by block
	while(reader != data.cend())
	{
		// decode the valid group
		if( is_valid(*reader) )
		{
            decode( iter, reader );
		// correct fill bytes
		} else if( *reader == '=' )
		{
			--iter;
			++reader;
		}
		// ignore line feeds
		else ++reader;
	}

	buffer.resize( std::distance(buffer.data(), iter) + 1 );
	return buffer;
}
