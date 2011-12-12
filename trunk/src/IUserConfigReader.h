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
#include <boost/shared_ptr.hpp>

class IUserConfigReader
{
public:
	IUserConfigReader() {};
	static boost::shared_ptr<IUserConfigReader> createUserConfigReader(const std::string& file);
	virtual ~IUserConfigReader() {};
	
	virtual std::string getValue(const std::string& name) const = 0;
	virtual float getFloat(const std::string& name) const = 0;
	virtual std::string getString(const std::string& name) const = 0;
	virtual bool getBool(const std::string& name) const = 0;
	virtual int getInteger(const std::string& name) const = 0;
};
