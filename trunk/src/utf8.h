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

#pragma once

/// @brief move wchar_t string to utf8 character sequence
/// @param codepoint String of data
/// @param target Target char array which will contain utf8 sequence
inline void to_utf8(wchar_t codepoint, char* target)
{
	if(codepoint <= 0x7F)
	{
		target[0] = codepoint;
	} 
	else if (codepoint <= 0x07FF )
	{
		unsigned char high = codepoint >> 8;
		unsigned char low = codepoint & 0xFF;
		// codepoint: 00000xxx|xxyyyyyy
		// =>         110xxxxx|10yyyyyy
		target[0] = 0xC0 | (0x1F & (high << 2 | low >> 6 ) );
		target[1] = 0x80 | (0x3F & low);
	} else 
	{
		//TODO: Missing implementation
	}
}

/// @brief get utf8 character char length  
/// @param first First char of utf8 character
inline int getCodepointLength(char first)
{
	// if 0xxx xxxx
	if (!(first & 0x80))
	{
		return 1;
	}

	// if x0xx xxxx
	if (!(first & 0x40))
	{
		return 1;
	}

	// if xx0x xxxx
	if (!(first & 0x20))
	{
		return 2;
	}

	// if xxx0 xxxx
	if (!(first & 0x10))
	{
		return 3;
	}

	// if xxxx 0xxx
	if (!(first & 0x08))
	{
		return 4;
	}
	
	// for other cases, threat as 1 character
	return 1;
}
