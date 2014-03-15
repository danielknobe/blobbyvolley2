#include "IScriptableComponent.h"

#include "lua/lua.hpp"

#include "Global.h"
#include "GameConstants.h"

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

void IScriptableComponent::setGameConstants()
{
	// set game constants
	setLuaGlobal("CONST_FIELD_WIDTH", RIGHT_PLANE);
	setLuaGlobal("CONST_GROUND_HEIGHT", 600 - GROUND_PLANE_HEIGHT_MAX);
	setLuaGlobal("CONST_BALL_GRAVITY", -BALL_GRAVITATION);
	setLuaGlobal("CONST_BALL_RADIUS", BALL_RADIUS);
	setLuaGlobal("CONST_BLOBBY_JUMP", BLOBBY_JUMP_ACCELERATION);
	setLuaGlobal("CONST_BLOBBY_BODY_RADIUS", BLOBBY_LOWER_RADIUS);
	setLuaGlobal("CONST_BLOBBY_HEAD_RADIUS", BLOBBY_UPPER_RADIUS);
	setLuaGlobal("CONST_BLOBBY_HEIGHT", BLOBBY_HEIGHT);
	setLuaGlobal("CONST_BLOBBY_GRAVITY", GRAVITATION);
	setLuaGlobal("CONST_NET_HEIGHT", 600 - NET_SPHERE_POSITION);
	setLuaGlobal("CONST_NET_RADIUS", NET_RADIUS);
	setLuaGlobal("NO_PLAYER", NO_PLAYER);
	setLuaGlobal("LEFT_PLAYER", LEFT_PLAYER);
	setLuaGlobal("RIGHT_PLAYER", RIGHT_PLAYER);
}
