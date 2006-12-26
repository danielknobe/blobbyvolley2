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

#include <iostream>
#include <vector>

struct UserConfigVar
{
	std::string Name;
	std::string Value;
	std::string DefaultValue;	  
};

class UserConfig
{
public:
	~UserConfig();
	
	bool loadFile(const std::string& filename);
	bool saveFile(const std::string& filename);

	void setValue(const std::string& name, const std::string& value);
	std::string getValue(const std::string& name);
	UserConfigVar* createVar(const std::string& name, 
			const std::string& defaultValue);

	float getFloat(const std::string& name);
	std::string getString(const std::string& name);
	bool getBool(const std::string& name);
	int getInteger(const std::string& name);

	void setFloat(const std::string& name, float value);
	void setString(const std::string& name, const std::string& value);
	void setBool(const std::string& name, bool value);
	void setInteger(const std::string& name, int value);
	
private:
	std::vector<UserConfigVar*> mVars;
	bool mChangeFlag;
	UserConfigVar *findVarByName(const std::string& Name);
};
