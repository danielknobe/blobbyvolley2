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

#include <cstdint>
#include <cassert>
#include <cctype>
#include <string>
#include <array>
#include <iterator>
#include <stdexcept>
#include <algorithm>

#include "base64.h"


// helper functions and constants
constexpr const char translation_table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

// this gives the length of a constexpr string literal.
template< std::size_t N >
constexpr std::size_t length( char const (&)[N] )
{
	return N-1;
}

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
constexpr uint8_t find_in_table<0>(uint8_t entry) { return 0; }

template <uint8_t ... Ns> struct index_seq
{
};

template <uint8_t ... Ns> struct make_index_sq;

template <uint8_t I, uint8_t ... Ns>
struct make_index_sq<I, Ns...>
{
	using type = typename make_index_sq<I - 1, I - 1, Ns...>::type;
};

template <uint8_t ... Ns>
struct make_index_sq<0, Ns...>
{
	using type = index_seq<Ns...>;
};

template <uint8_t N>
using make_index_sq_t = typename make_index_sq<N>::type;

template<class T>
struct make_table_helper {
};

template<uint8_t... Ns>
struct make_table_helper<index_seq<Ns...>>
{
	static constexpr auto make() -> std::array<uint8_t, sizeof...(Ns)>
	{
		return {{find_in_table<length(translation_table)>(Ns)...}};
	}
};


// generate the decoding table
constexpr auto decoding_table = make_table_helper<typename make_index_sq<255>::type>::make();

constexpr uint8_t decode( uint8_t byte )
{
	return is_valid(byte) ? decoding_table[byte] : -1;
}

// the bitmask for slicing the three bytes into subelements
constexpr std::uint32_t bitmask = (1 << 6) - 1;

static_assert( length(translation_table) == 64, "error: need 64 characters in the translation table" );

// -------------------------------------------------------------------
//		coding functions
// -------------------------------------------------------------------

// simple encoding function for 3 bytes
void encode( const std::uint8_t*& byte_array, std::string::iterator& writer )
{
	unsigned const index0 = bitmask & (byte_array[0] >> 2);
	assert(index0 < 64);
	writer[0] = translation_table[index0];

	unsigned const index1 = bitmask & ((byte_array[0] << 4) | (byte_array[1] >> 4));
	assert(index1 < 64);
	writer[1] = translation_table[index1];

	unsigned const index2 = bitmask & ((byte_array[1] << 2) | (byte_array[2] >> 6));
	assert(index2 < 64);
	writer[2] = translation_table[index2];

	unsigned const index3 = bitmask & (byte_array[2]);
	assert(index3 < 64);
	writer[3] = translation_table[index3];

	std::advance(writer, 4);
	std::advance(byte_array, 3);
}

std::string encode( const char* begin, const char* end, int newlines)
{
	// allocate enough space inside the string
	const std::size_t length  = end - begin;
	const unsigned tail       = length % 3;
	const unsigned extra      = (tail != 0) ? 1 : 0;
	const unsigned groups     = length / 3;
	const unsigned total_data = (groups + extra) * 4;
	newlines -= newlines % 4;
	const unsigned linefeeds = newlines > 0 ? total_data / newlines : 0;

	std::string buffer(total_data + linefeeds, '\0');

	// iterate over sequence
	auto write = buffer.begin();
	auto read  = reinterpret_cast<const std::uint8_t*>(begin);
	int last_newline = 0;
	for(unsigned i = 0; i < groups; ++i)
	{
		encode(read, write);
		if( newlines > 0 && (i+1) * 4 >= static_cast<unsigned>(last_newline + newlines) )
		{
			*write = '\n';
			write++;
			last_newline = (i+1)*4;
		}
	}

	// add the partial group
	if(extra)
	{
		std::uint8_t partial_group[4] = { 0 };
		for(unsigned i = 0; i < tail; ++i)
			partial_group[i] = *(read++);
		const uint8_t* const_partial_group = partial_group;
		encode(const_partial_group, write);

		buffer[buffer.length() - 1] = '=';
		if (tail == 1) 
		{
			buffer[buffer.length() - 2] = '=';
		}
	}
	
	assert( (char*)read == end );
	assert( write == buffer.end());
	
	return buffer;
}

std::uint8_t decode_byte_one(const char* reader) {
	std::uint8_t res;
	uint8_t const value0 = decode(reader[0]);
	uint8_t const value1 = decode(reader[1]);
	if( ((value0 & (~bitmask)) != 0 ) ||
	    ((value1 & (~bitmask)) != 0 ) )
	{
		throw std::runtime_error("invalid base64 character");
	}
	res = (value0 << 2);
	res |= value1 >> 4;

	return res;
}

std::uint8_t decode_byte_two(const char* reader) {
	std::uint8_t res;
	uint8_t const value1 = decode(reader[1]);
	uint8_t const value2 = decode(reader[2]);
	if( ((value1 & (~bitmask)) != 0 ) ||
	    ((value2 & (~bitmask)) != 0 ) )
	{
		throw std::runtime_error("invalid base64 character");
	}
	res = value1 << 4;
	res |= value2 >> 2;

	return res;
}

std::uint8_t decode_byte_three(const char* reader) {
	std::uint8_t res;
	uint8_t const value2 = decode(reader[2]);
	uint8_t const value3 = decode(reader[3]);
	if( ((value2 & (~bitmask)) != 0 ) ||
	    ((value3 & (~bitmask)) != 0 ) )
	{
		throw std::runtime_error("invalid base64 character");
	}
	res = value2 << 6;
	res |= value3;

	return res;
}

std::vector<uint8_t> decode(const std::string& data )
{
	// pre-allocate buffer
	const size_t incomingDataSize = data.size();

	// count characters which do not belong to base64
	int const linefeeds = std::count_if(data.cbegin(), data.cend(), [](char ch){return std::isspace(ch); });

	// calculate character count which belongs to base64
	int const base64DataSize = incomingDataSize - linefeeds;

	// check input size
	if (base64DataSize % 4)
	{
		throw std::runtime_error("data length must be multiple of 4");
	}
	
	std::vector<uint8_t> buffer(base64DataSize / 4 * 3 );
	uint8_t* iter = buffer.data();
	auto reader = data.cbegin();

	while(reader != data.cend())
	{
		if( std::isspace(reader[0]) )
		{
			std::advance(reader, 1);
			continue;
		}

		if( is_valid(reader[1]) ) 
		{
			*iter = decode_byte_one(&reader[0]);
			std::advance(iter, 1);
		}

		if( is_valid(reader[2]) ) 
		{
			*iter = decode_byte_two(&reader[0]);
			std::advance(iter, 1);
		}

		if( is_valid(reader[3]) ) 
		{
			*iter = decode_byte_three(&reader[0]);
			std::advance(iter, 1);
		}

		std::advance(reader, 4);
	}

	buffer.resize(std::distance(buffer.data(), iter));

	return buffer;
}
