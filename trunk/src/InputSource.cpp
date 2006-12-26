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

#include "InputSource.h"

std::string& operator<< (std::string& out, const PlayerInput& input)
{
	char buf[4];
	buf[0] = input.left ? 't' : 'f';
	buf[1] = input.right ? 't' : 'f';
	buf[2] = input.up ? 't' : 'f';
	buf[3] = '\0';
	out.append(std::string(buf));
	return out;
}

PlayerInput::PlayerInput(const std::string& string)
{
	left = string.at(0) == 't';
	right = string.at(1) == 't';
	up = string.at(2) == 't';
}
