#include "ScriptedInputSource.h"
#include "DuelMatch.h"

extern "C"
{
#include <lua.h>
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
	
	lua_register(mState, "side", side);
	lua_register(mState, "touches", touches);
	lua_register(mState, "balldown", balldown);
	lua_register(mState, "touching", touching);
	lua_register(mState, "launched", launched);
	lua_register(mState, "debug", debug);
	lua_register(mState, "abs", abs);
	lua_register(mState, "random", random);
	lua_register(mState, "stop", stop);
	lua_register(mState, "stopjump", stopjump);
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
	lua_register(mState, "estimate", estimate);

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
		lua_pop(mState, 1);
		lua_close(mState);
		throw ScriptException();
	}
	
	}

ScriptedInputSource::~ScriptedInputSource()
{
	lua_close(mState);
}

PlayerInput ScriptedInputSource::getInput()
{
	lua_getglobal(mState, "step");
	if (!lua_isfunction(mState, -1))
	{
		lua_pop(mState, 1);
		std::cerr << "Lua Error: Could not find function step()";
		std::cerr << std::endl;
		return PlayerInput();
	}
	int error = lua_pcall(mState, 0, 0, 0);
	if (error)
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
		lua_pop(mState, 1);
	}
	lua_getglobal(mState, "blobby_moveleft");
	lua_getglobal(mState, "blobby_moveright");
	lua_getglobal(mState, "blobby_moveup");
	PlayerInput currentInput = PlayerInput(lua_toboolean(mState, -3),
		lua_toboolean(mState, -2), lua_toboolean(mState, -1));
	lua_pop(mState, 3);
	return currentInput;
}

int ScriptedInputSource::side(lua_State* state)
{
	lua_getglobal(state, "blobby_side");
	return 1;
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

int ScriptedInputSource::balldown(lua_State* state)
{
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushboolean(state, false);
		return 1;
	}
	lua_pushboolean(state, match->getBallDown());
	return 1;
}

int ScriptedInputSource::touching(lua_State* state)
{
// TODO: Implement this correctly
	lua_pushboolean(state, false);
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
	lua_getglobal(state, "blobby_side");
	PlayerSide player = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, -1);
	lua_pushboolean(state, match->getBlobJump(player));
	return 1;
}

int ScriptedInputSource::debug(lua_State* state)
{
// TODO: implement this correctly or leave it out
	lua_pop(state, 1);
}

int ScriptedInputSource::abs(lua_State* state)
{
	double num = lua_tonumber(state, -1);
	lua_pop(state, 1);
	lua_pushnumber(state, fabs(num));
	return 1;
}

int ScriptedInputSource::random(lua_State* state)
{
// TODO: check if this the correct behaviour.
//       Anyway, the current implementation isn't very optimal.
	int num = lround(lua_tonumber(state, -1));
	lua_pop(state, 1);
	lua_pushnumber(state, rand() % num);
	return 1;
}

int ScriptedInputSource::stop(lua_State* state)
{
	lua_pushboolean(state, false);
	lua_pushboolean(state, false);
	lua_setglobal(state, "blobby_moveleft");
	lua_setglobal(state, "blobby_moveright");
	return 0;
}

int ScriptedInputSource::stopjump(lua_State* state)
{
	lua_pushboolean(state, false);
	lua_setglobal(state, "blobby_moveup");
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
	lua_setglobal(state, "blobby_moveleft");
	return 0;
}

int ScriptedInputSource::right(lua_State* state)
{
	lua_pushboolean(state, true);
	lua_setglobal(state, "blobby_moveright");
	return 0;
}

int ScriptedInputSource::moveto(lua_State* state)
{
	float target = lua_tonumber(state, -1);
	lua_getglobal(state, "blobby_side");
	PlayerSide player = (PlayerSide)lua_tonumber(state, -1);
	lua_pop(state, 2);
	DuelMatch* match = DuelMatch::getMainGame();
	if (match != 0)
	{
		float position = match->getBlobPosition(player).x;
		if (position > target)
			left(state);
		if (position < target)
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
	lua_pushnumber(state, match->getBallPosition().x);
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
	lua_pushnumber(state, match->getBallPosition().y);
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
	lua_pushnumber(state, match->getBallVelocity().x);
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
	lua_pushnumber(state, match->getBallVelocity().y);
	return 1;
}

int ScriptedInputSource::posx(lua_State* state)
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
	lua_pushnumber(state, match->getBlobPosition(player).x);
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
	lua_pushnumber(state, match->getBlobPosition(player).y);
	return 1;
}

int ScriptedInputSource::estimate(lua_State* state)
{
// TODO: implement this extremly important function
	DuelMatch* match = DuelMatch::getMainGame();
	if (match == 0)
	{
		lua_pushnumber(state, 400.0);
		return 1;
	}
	lua_pushnumber(state, match->getBallEstimation());
	return 1;
}

