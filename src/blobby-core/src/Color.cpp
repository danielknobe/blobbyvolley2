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

/* header include */
#include "Color.h"

/* includes */

/* implementation */

Color::Color(int red, int green, int blue)
		: r(red)
		, g(green)
		, b(blue)
{

}

Color::Color(unsigned int col) : Color(col&0xff, (col>>8)&0xff, (col>>16)&0xff)
{

}

unsigned int Color::toInt() const
{
	int i = 0;
	i |= r;
	i |= g << 8;
	i |= b << 16;
	return i;
}
