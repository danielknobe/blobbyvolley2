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
