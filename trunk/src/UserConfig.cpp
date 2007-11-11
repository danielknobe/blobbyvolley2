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

#include <iostream>
#include <physfs.h>
#include "tinyxml/tinyxml.h"
#include "UserConfig.h"
#include "Global.h"

bool UserConfig::loadFile(const std::string& filename)
{
	PHYSFS_file* fileHandle = PHYSFS_openRead(filename.c_str());
	if (!fileHandle)
	{
		throw FileLoadException(filename);
	}

	int fileLength = PHYSFS_fileLength(fileHandle);
	char* fileBuffer = new char[fileLength + 1];
	PHYSFS_read(fileHandle, fileBuffer, 1, fileLength);
	fileBuffer[fileLength] = 0;
	TiXmlDocument configDoc;
	configDoc.Parse(fileBuffer);
	delete[] fileBuffer;
	PHYSFS_close(fileHandle);

	if (configDoc.Error())
	{
		std::cerr << "Warning: Parse error in " << filename;
		std::cerr << "!" << std::endl;
	}

	TiXmlElement* userConfigElem =
		configDoc.FirstChildElement("userconfig");
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

bool UserConfig::saveFile(const std::string& filename)
{
	PHYSFS_file* fileHandle = PHYSFS_openWrite(filename.c_str());
	if (!fileHandle)
		return false;

	const char xmlHeader[] =
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n<userconfig>\n";
	const char xmlFooter[] = "</userconfig>\n\n";

	PHYSFS_write(fileHandle, xmlHeader, 1, sizeof(xmlHeader) - 1);
	for (unsigned int i = 0; i < mVars.size(); ++i)
	{
		char writeBuffer[256];
		int charsWritten = snprintf(writeBuffer, 256,
			"\t<var name=\"%s\" value=\"%s\"/>\n",
			mVars[i]->Name.c_str(), mVars[i]->Value.c_str());
		PHYSFS_write (fileHandle, writeBuffer, 1, charsWritten);
	}

	PHYSFS_write(fileHandle, xmlFooter, 1, sizeof(xmlFooter) - 1);
	PHYSFS_close(fileHandle);
	return true;
}

float UserConfig::getFloat(const std::string& name)
{
	return atof(getValue(name).c_str());
}

std::string UserConfig::getString(const std::string& name)
{
	return getValue(name);
}

bool UserConfig::getBool(const std::string& name)
{
	return (getValue(name) == "true") ? true : false;
}

int UserConfig::getInteger(const std::string& name)
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
			"\n Creating new varaible" << std::endl;
		var = createVar(name, value);
		mChangeFlag = true;
		return;
	}

	if (var->Value != value) mChangeFlag = true;
	var->Value = value;
}

std::string UserConfig::getValue(const std::string& name)
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

UserConfigVar* UserConfig::findVarByName(const std::string& name)
{
	for (unsigned int i = 0; i < mVars.size(); ++i)
		if (mVars[i]->Name == name) return mVars[i];
	return NULL;
}

