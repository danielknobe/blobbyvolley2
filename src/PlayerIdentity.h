/*=============================================================================
Blobby Volley 2
Copyright (C) 2008 Jonathan Sieber (jonathan_sieber@yahoo.de)

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

#include "Global.h"

class PlayerIdentity
{
	public:
		PlayerIdentity() {};
		PlayerIdentity(std::string name);
		PlayerIdentity(const std::string& name, Color color, bool osci);
		~PlayerIdentity();

		const std::string& getName() const;
		Color getStaticColor() const;
		bool getOscillating() const;
		
		void setName(const std::string& nname);
		void setStaticColor(Color ncol);
		void setOscillating(bool oc);
	
	private:
		std::string mName;		
		Color mStaticColor;
		bool mOscillating;
};
