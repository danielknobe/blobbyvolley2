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
#include <vector>
#include <cstdint>
#include <type_traits>

// encodes a single three byte data element
// advances both by 3 or 4 positions resp.
void encode( const std::uint8_t*& bits, std::string::iterator& writer );

// encodes an arbitrary pointer range
std::string encode( const char* begin, const char* end, int newlines = -1);

template<class T>
std::string encode(const std::vector<T>& data_vec, int newlines = -1)
{
	static_assert( std::is_pod<T>::value, "T has to be a POD type");
	auto start = &*data_vec.begin();
	auto begin = reinterpret_cast<const char*>(start);
	auto end   = reinterpret_cast<const char*>(start + data_vec.size());
	return encode( begin, end, newlines );
}

// decoder
/// basic block decoder function. Does not take into account = signs.
void decode( std::uint8_t*& byte_array, std::string::const_iterator& reader );

/// decodes a base64 encoded string into a byte vector. All characters
/// that are not valid encodings are ignored (i.e. linefeeds).
std::vector<uint8_t> decode(const std::string& data );
