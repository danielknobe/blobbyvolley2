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

/* header include */
#include "UserConfig.h"

/* includes */
#include <iostream>
#include <map>

#include "tinyxml/tinyxml.h"

#include "Global.h"
#include "FileRead.h"
#include "FileWrite.h"


/* implementation */
std::map<std::string, boost::shared_ptr<IUserConfigReader> > userConfigCache;

boost::shared_ptr<IUserConfigReader> IUserConfigReader::createUserConfigReader(const std::string& file)
{
	// if we have this userconfig already cached, just return from cache
	std::map<std::string, boost::shared_ptr<IUserConfigReader> >:: iterator cfg_cached = userConfigCache.find(file);
	if( cfg_cached != userConfigCache.end() )
	{
		return cfg_cached->second;
	}
	
	// otherwise, load user config...
	UserConfig* uc = new UserConfig();
	uc->loadFile(file);
	boost::shared_ptr<IUserConfigReader> config(uc);
	
	// ... and add to cache
	userConfigCache[file] = config;
	
	return config;
}



bool UserConfig::loadFile(const std::string& filename)
{
	boost::shared_ptr<TiXmlDocument> configDoc = FileRead::readXMLDocument(filename);

	if (configDoc->Error())
	{
		std::cerr << "Warning: Parse error in " << filename;
		std::cerr << "!" << std::endl;
	}

	TiXmlElement* userConfigElem =
		configDoc->FirstChildElement("userconfig");
	if (userConfigElem == NULL)
		return false;
	for (TiXmlElement* varElem =
		userConfigElem->FirstChildElement("var");
		varElem != NULL;
		varElem = varElem->NextSiblingElement("var"))
	{
		std::string name, value;
		const char* c;
		c = varElem->Attribute("name");
		if (c)
			name = c;
		c = varElem->Attribute("value");
		if (c)
			value = c;
		createVar(name, value);
	}
	
	return true;
}

bool UserConfig::saveFile(const std::string& filename) const
{
	// this trows an exception if the file could not be opened for writing
	FileWrite file(filename);

	const std::string xmlHeader =
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n<userconfig>\n";
	
	const std::string xmlFooter = "</userconfig>\n\n";
	
	file.write(xmlHeader);

	for (unsigned int i = 0; i < mVars.size(); ++i)
	{
		char writeBuffer[256];
		int charsWritten = snprintf(writeBuffer, 256,
			"\t<var name=\"%s\" value=\"%s\"/>\n",
			mVars[i]->Name.c_str(), mVars[i]->Value.c_str());
		
		file.write(writeBuffer, charsWritten);
	}

	file.write(xmlFooter);
	file.close();
	
	// we have to make sure that we don't cache any outdated user configs
	std::map<std::string, boost::shared_ptr<IUserConfigReader> >:: iterator cfg_cached = userConfigCache.find(filename);
	if( cfg_cached != userConfigCache.end() )
	{
		userConfigCache.erase(cfg_cached);
	}
	
	return true;
}

float UserConfig::getFloat(const std::string& name) const
{
	return atof(getValue(name).c_str());
}

std::string UserConfig::getString(const std::string& name) const
{
	return getValue(name);
}

bool UserConfig::getBool(const std::string& name) const
{
	return (getValue(name) == "true") ? true : false;
}

int UserConfig::getInteger(const std::string& name) const
{
	return atoi(getValue(name).c_str());
}

void UserConfig::setFloat(const std::string& name, float var)
{
	char writeBuffer[256];
	snprintf(writeBuffer, 256, "%f", var);
	setValue(name, writeBuffer);
}

void UserConfig::setString(const std::string& name, const std::string& var)
{
	setValue(name, var);
}

void UserConfig::setBool(const std::string& name, bool var)
{
	setValue(name, var ? "true" : "false");
}

void UserConfig::setInteger(const std::string& name, int var)
{
	char writeBuffer[256];
	snprintf(writeBuffer, 256, "%d", var);
	setValue(name, writeBuffer);
}

UserConfigVar* UserConfig::createVar(const std::string& name,
		const std::string& defaultValue)
{
	if (findVarByName(name)) return NULL;
	UserConfigVar *var = new UserConfigVar;
	var->Name = name;
	var->DefaultValue = var->Value = defaultValue;
	mVars.push_back(var);
	return var;
}

void UserConfig::setValue(const std::string& name, const std::string& value)
{
	UserConfigVar *var = findVarByName(name);
	if (!var)
	{
		std::cerr << "Warning: impossible to set value of " <<
			"unknown configuration variable " << name <<
			"\n Creating new variable" << std::endl;
		var = createVar(name, value);
		mChangeFlag = true;
		return;
	}

	if (var->Value != value) mChangeFlag = true;
	var->Value = value;
}

std::string UserConfig::getValue(const std::string& name) const
{
	UserConfigVar *var = findVarByName(name);
	if (!var)
	{
		std::cerr << "Warning: impossible to get value of " <<
			"unknown configuration variable " << name << std::endl;
		return "";
	}
	return var->Value;
}

UserConfig::~UserConfig()
{
	for (unsigned int i = 0; i < mVars.size(); ++i)
		delete mVars[i];
}

UserConfigVar* UserConfig::findVarByName(const std::string& name) const
{
	for (unsigned int i = 0; i < mVars.size(); ++i)
		if (mVars[i]->Name == name) return mVars[i];
	return NULL;
}

