/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2022 Daniel Knobe (daniel-knobe@web.de)
Copyright (C) 2022 Erik Schultheis (erik-schultheis@freenet.de)

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

#include <cstdint>

/*! \class Color
	\brief represents RGB Colours
	\details This class represents colors as RGB with one byte for each channel.
*/
struct Color
{
	public:
		Color(int red, int green, int blue);

		/// \sa toInt()
		explicit Color(unsigned int col);

		Color() = default;

		bool operator == (Color rval) const
		{
			return r == rval.r && g == rval.g && b == rval.b;
		}

		bool operator != (Color rval) const
		{
			return !(*this == rval);
		}

		unsigned int toInt() const;

		std::uint8_t r = 0;
		std::uint8_t g = 0;
		std::uint8_t b = 0;

};
