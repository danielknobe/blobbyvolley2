#include "ScriptedInputSource.h"
#include "DuelMatch.h"

extern "C"
{
#if defined(WIN32)
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
#else
#if defined(HAVE_LUA51_LUA_H)
	#include <lua5.1/lua.h>
#elif defined(HAVE_LUA50_LUA_H)
	#include <lua50/lua.h>
#elif defined(HAVE_LUA_LUA_H)
	#include <lua/lua.h>
#elif defined(HAVE_LUA_H)
	#include <lua.h>
#else
	#error Missing lua.h
#endif

#if defined(HAVE_LUA51_LAUXLIB_H)
	#include <lua5.1/lauxlib.h>
#elif defined(HAVE_LUA50_LAUXLIB_H)
	#include <lua50/lauxlib.h>
#elif defined(HAVE_LUA_LAUXLIB_H)
	#include <lua/lauxlib.h>
#elif defined(HAVE_LAUXLIB_H)
	#include <lauxlib.h>
#else
	#error Missing lauxlib.h
#endif
#if defined(HAVE_LUA51_LUALIB_H)
	#include <lua5.1/lualib.h>
#elif defined(HAVE_LUA50_LUALIB_H)
	#include <lua50/lualib.h>
#elif defined(HAVE_LUA_LUALIB_H)
	#include <lua/lualib.h>
#elif defined(HAVE_LUALIB_H)
	#include <lualib.h>
#else
	#error Missing lualib.h
#endif
#endif
}

#include <physfs.h>
#include <cmath>

struct ReaderInfo
{
	PHYSFS_file* handle;
	char readBuffer[2048];
};

static const char* chunkReader(lua_State* state, void* data, size_t *size)
{
	ReaderInfo* info = (ReaderInfo*) data;
	int bytesRead = PHYSFS_read(info->handle, info->readBuffer, 1, 2048);
	*size = bytesRead;
	if (bytesRead == 0)
	{
		return 0;
	}
	else
	{
		return info->readBuffer;
	}
}

ScriptedInputSource::ScriptedInputSource(const std::string& filename,
						PlayerSide playerside)
{
	mState = lua_open();
	lua_pushnumber(mState, playerside);
	lua_setglobal(mState, "blobby_side");
	luaopen_math(mState);
	lua_register(mState, "touches", touches);
	lua_register(mState, "launched", launched);
	lua_register(mState, "debug", debug);
	lua_register(mState, "jump", jump);
	lua_register(mState, "moveto", moveto);
	lua_register(mState, "left", left);
	lua_register(mState, "right", right);
	lua_register(mState, "ballx", ballx);
	lua_register(mState, "bally", bally);
	lua_register(mState, "bspeedx", bspeedx);
	lua_register(mState, "bspeedy", bspeedy);
	lua_register(mState, "posx", posx);
	lua_register(mState, "posy", posy);
	lua_register(mState, "oppx", oppx);
	lua_register(mState, "oppy", oppy);
	lua_register(mState, "estimate", estimate);
	lua_register(mState, "estimx", estimx);
	lua_register(mState, "estimy", estimy);

	ReaderInfo info;
	info.handle = PHYSFS_openRead(filename.c_str());
	if (!info.handle)
	{
		throw FileLoadException(filename);
	}
	int error;
	error = lua_load(mState, chunkReader, &info, filename.c_str());
	PHYSFS_close(info.handle);
	if (error == 0)
		error = lua_pcall(mState, 0, 6, 0);
        if (error)
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
		ScriptException except;
		except.luaerror = lua_tostring(mState, -1);
		lua_pop(mState, 1);
		lua_close(mState);
		throw except;
	}
	lua_pop(mState, lua_gettop(mState));
}

ScriptedInputSource::~ScriptedInputSource()
{
	lua_close(mState);
}

PlayerInput ScriptedInputSource::getInput()
{
	lua_pushboolean(mState, false);
	lua_pushboolean(mState, false);
	lua_pushboolean(mState, false);
	lua_setglobal(mState, "blobby_moveleft");
	lua_setglobal(mState, "blobby_moveright");
	lua_setglobal(mState, "blobby_moveup");

	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		return PlayerInput();
	}
	
	PlayerSide player = getSide(mState);
	int error;
	if (!match->getBallActive() && player == match->getServingPlayer())
	{
		lua_getglobal(mState, "OnServe");
		if (!lua_isfunction(mState, -1))
		{
			lua_pop(mState, 1);
			std::cerr << "Lua Error: Could not find function ";
			std::cerr << "OnServe()" << std::endl;
			return PlayerInput();
		}
		lua_pushboolean(mState, !match->getBallDown());
		error = lua_pcall(mState, 1, 0, 0);
	}
	else if (!match->getBallActive() && player != match->getServingPlayer())
	{
		lua_getglobal(mState, "OnOpponentServe");
		if (!lua_isfunction(mState, -1))
		{
			lua_pop(mState, 1);
			std::cerr << "Lua Error: Could not find function ";
			std::cerr << "OnOpponentServe()" << std::endl;
			return PlayerInput();
		}
		error = lua_pcall(mState, 0, 0, 0);
	}
	else
	{
		lua_getglobal(mState, "OnGame");
		if (!lua_isfunction(mState, -1))
		{
			lua_pop(mState, 1);
			std::cerr << "Lua Error: Could not find function ";
			std::cerr << "OnGame()" << std::endl;
			return PlayerInput();
		}
		error = lua_pcall(mState, 0, 0, 0);
	}
	if (error)
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
		lua_pop(mState, 1);
	}

	lua_getglobal(mState, "blobby_moveleft");
	lua_getglobal(mState, "blobby_moveright");
	lua_getglobal(mState, "blobby_moveup");
	bool wantLeft = lua_toboolean(mState, -3);
	bool wantRight = lua_toboolean(mState, -2);
	bool wantUp = lua_toboolean(mState, -1);
	lua_pop(mState, 3);
	PlayerInput currentInput = PlayerInput(wantLeft, wantRight, wantUp);

	int stacksize = lua_gettop(mState);
	if (stacksize > 0)
	{
		std::cerr << "Warning: Stack messed up!" << std::endl;
		std::cerr << "Element on stack is a ";
		std::cerr << lua_typename(mState, -1) << std::endl;
		lua_pop(mState, stacksize);
	}
	
	return currentInput;
}

PlayerSide ScriptedInputSource::getSide(lua_State* state)
{
	lua_getglobal(state, "blobby_side");
	PlayerSide player = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	return player;
}

int ScriptedInputSource::touches(lua_State* state)
{
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 0);
		return 1;
	}
	lua_getglobal(state, "blobby_side");
	PlayerSide player = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, -1);
	lua_pushnumber(state, match->getHitcount(player));
	return 1;
}

int ScriptedInputSource::launched(lua_State* state)
{
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushboolean(state, false);
		return 1;
	}
	lua_pushboolean(state, match->getBlobJump(getSide(state)));
	return 1;
}

int ScriptedInputSource::debug(lua_State* state)
{
	float number = lua_tonumber(state, -1);
	lua_pop(state, 1);
	std::cerr << "Lua Debug: " << number << std::endl;
	return 0;
}

int ScriptedInputSource::jump(lua_State* state)
{
	lua_pushboolean(state, true);
	lua_setglobal(state, "blobby_moveup");
	return 0;
}

int ScriptedInputSource::left(lua_State* state)
{
	lua_pushboolean(state, true);
	if (getSide(state) == LEFT_PLAYER)
		lua_setglobal(state, "blobby_moveleft");
	else if (getSide(state) == RIGHT_PLAYER)
		lua_setglobal(state, "blobby_moveright");
	else
		lua_pop(state, 1);
	return 0;
}

int ScriptedInputSource::right(lua_State* state)
{
	lua_pushboolean(state, true);
	if (getSide(state) == LEFT_PLAYER)
		lua_setglobal(state, "blobby_moveright");
	else if (getSide(state) == RIGHT_PLAYER)
		lua_setglobal(state, "blobby_moveleft");
	else
		lua_pop(state, 1);
	return 0;
}

int ScriptedInputSource::moveto(lua_State* state)
{
	float target = lua_tonumber(state, -1);
	lua_pop(state, 1);
	DuelMatch* match = DuelMatch::getMainGame();
	if (match != 0)
	{
		PlayerSide player = getSide(state);
		float position = match->getBlobPosition(player).x;
		if (player == RIGHT_PLAYER)
		{
//			target = 800.0 - target;
			position = 800.0 - position;
		}
		if (position > target + 2)
			left(state);
		if (position < target - 2)
			right(state);
	}	
	return 0;
}

int ScriptedInputSource::ballx(lua_State* state)
{
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 400.0);
		return 1;
	}
	float pos = match->getBallPosition().x;
	if (getSide(state) == RIGHT_PLAYER)
		pos = 800.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::bally(lua_State* state)
{
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 150.0);
		return 1;
	}
	float pos = match->getBallPosition().y;
	pos = 600.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::bspeedx(lua_State* state)
{
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 0.0);
		return 1;
	}
	float vel = match->getBallVelocity().x;
	if (getSide(state) == RIGHT_PLAYER)
		vel = -vel;
	lua_pushnumber(state, vel);
	return 1;
}

int ScriptedInputSource::bspeedy(lua_State* state)
{
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 0.0);
		return 1;
	}
	float vel = match->getBallVelocity().y;
	vel = 600.0 - vel;
	lua_pushnumber(state, vel);
	return 1;
}

int ScriptedInputSource::posx(lua_State* state)
{
	PlayerSide player = getSide(state);
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 0.0);
		return 1;
	}
	float pos = match->getBlobPosition(player).x;
	if (player == RIGHT_PLAYER)
		pos = 800.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::posy(lua_State* state)
{
	lua_getglobal(state, "blobby_side");
	PlayerSide player = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 1);
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 0.0);
		return 1;
	}
	float pos = match->getBlobPosition(player).y;
	pos = 600.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::oppx(lua_State* state)
{
	PlayerSide player = getSide(state);
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 0.0);
		return 1;
	}
	PlayerSide invPlayer = 
		player == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER;
	lua_pushnumber(state, match->getBlobPosition(invPlayer).x);
	return 1;
}

int ScriptedInputSource::oppy(lua_State* state)
{
	PlayerSide player = getSide(state);
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 0.0);
		return 1;
	}
	PlayerSide invPlayer =
		player == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER;
	float pos = match->getBlobPosition(invPlayer).y;
	pos = 600.0 - pos;
	lua_pushnumber(state, pos);
	return 1;
}

int ScriptedInputSource::estimate(lua_State* state)
{
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 400.0);
		return 1;
	}
	float estim = match->getBallEstimation();
	if (getSide(state) == RIGHT_PLAYER)
		estim = 800.0 - estim;
	lua_pushnumber(state, estim);
	return 1;
}

int ScriptedInputSource::estimx(lua_State* state)
{
	int num = lround(lua_tonumber(state, -1));
	lua_pop(state, 1);
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 0.0);
		return 1;
	}
	float estim = match->getBallTimeEstimation(num).x;
	if (getSide(state) == RIGHT_PLAYER)
		estim = 800.0 - estim;
	lua_pushnumber(state, estim);
	return 1;
}

int ScriptedInputSource::estimy(lua_State* state)
{
	int num = lround(lua_tonumber(state, -1));
	lua_pop(state, 1);
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 0.0);
		return 1;
	}
	float estim = match->getBallTimeEstimation(num).y;
	estim = 600.0 - estim;
	lua_pushnumber(state, estim);
	return 1;
}
