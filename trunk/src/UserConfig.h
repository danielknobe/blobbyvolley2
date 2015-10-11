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

/**
 * @file UserConfig.h
 * @brief Contains a class which manages user configurations
 */

#pragma once

#include "IUserConfigReader.h"
#include "BlobbyDebug.h"

#include <string>
#include <vector>

struct UserConfigVar : public ObjectCounter<UserConfigVar>
{
	std::string Name;
	std::string Value;
};

/*! \class UserConfig
	\brief user configuration from xml data
	\details This class manages user configurations read from/written to xml data.
			It allows saving/loading from disk and getting/setting floats, booleans,
				strings and integers by name
*/
class UserConfig: public IUserConfigReader, public ObjectCounter<UserConfig>
{
	public:
		~UserConfig();

		bool loadFile(const std::string& filename);
		bool saveFile(const std::string& filename) const;

		void setValue(const std::string& name, const std::string& value);

		float getFloat(const std::string& name, float default_value = 0.f) const;
		std::string getString(const std::string& name, const std::string& default_value = "") const;
		bool getBool(const std::string& name, bool default_value = false) const;
		int getInteger(const std::string& name, int default_value = 0) const;

		PlayerIdentity loadPlayerIdentity(PlayerSide player, bool force_human);

		void setFloat(const std::string& name, float value);
		void setString(const std::string& name, const std::string& value);
		void setBool(const std::string& name, bool value);
		void setInteger(const std::string& name, int value);
	private:

		std::vector<UserConfigVar*> mVars;
		bool mChangeFlag;
		UserConfigVar* findVarByName(const std::string& name) const;
		UserConfigVar* checkVarByName(const std::string& name) const;
		UserConfigVar* createVar(const std::string& name, const std::string& value);
};
