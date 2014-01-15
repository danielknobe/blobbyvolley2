#include "IScriptableComponent.h"
#include "lua/lua.hpp"

IScriptableComponent::IScriptableComponent() : mState(luaL_newstate())
{

}

IScriptableComponent::~IScriptableComponent()
{
	lua_close(mState);
}

void IScriptableComponent::setLuaGlobal(const char* name, double value)
{
	lua_pushnumber(mState, value);
	lua_setglobal(mState, name);
}

bool IScriptableComponent::getLuaFunction(const char* fname) const
{
	lua_getglobal(mState, fname);
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
		return false;
	}

	return true;
}
