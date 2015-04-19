#include "IScriptableComponent.h"

#include "lua/lua.hpp"

#include "Global.h"
#include "GameConstants.h"
#include "DuelMatchState.h"
#include "FileRead.h"

#include <iostream>

// fwd decl
int lua_print(lua_State* state);

IScriptableComponent::IScriptableComponent() :
	mState(luaL_newstate()),
	mGameState( new DuelMatchState() )
{
	// register this in the lua registry
	lua_pushliteral(mState, "__C++_ScriptComponent__");
	lua_pushlightuserdata(mState, (void*)this);
	lua_settable(mState, LUA_REGISTRYINDEX);

	lua_register(mState, "print", lua_print);

	// open math lib
	luaL_requiref(mState, "math", luaopen_math, 1);
}

IScriptableComponent::~IScriptableComponent()
{
	lua_close(mState);
}

void IScriptableComponent::openScript(std::string file)
{
	int error = FileRead::readLuaScript(file, mState);
	if (error == 0)
		error = lua_pcall(mState, 0, 0, 0);

	if (error)
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
		ScriptException except;
		except.luaerror = lua_tostring(mState, -1);
		BOOST_THROW_EXCEPTION(except);
	}
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

void IScriptableComponent::callLuaFunction(int arg_count)
{
	if (lua_pcall(mState, arg_count, 0, 0))
	{
		std::cerr << "Lua Error: " << lua_tostring(mState, -1);
		std::cerr << std::endl;
		lua_pop(mState, 1);
	}
}

void IScriptableComponent::setGameConstants()
{
	// set game constants
	setLuaGlobal("CONST_FIELD_WIDTH", RIGHT_PLANE);
	setLuaGlobal("CONST_GROUND_HEIGHT", 600 - GROUND_PLANE_HEIGHT_MAX);
	setLuaGlobal("CONST_BALL_GRAVITY", -BALL_GRAVITATION);
	setLuaGlobal("CONST_BALL_RADIUS", BALL_RADIUS);
	setLuaGlobal("CONST_BLOBBY_JUMP", -BLOBBY_JUMP_ACCELERATION);
	setLuaGlobal("CONST_BLOBBY_BODY_RADIUS", BLOBBY_LOWER_RADIUS);
	setLuaGlobal("CONST_BLOBBY_HEAD_RADIUS", BLOBBY_UPPER_RADIUS);
	setLuaGlobal("CONST_BLOBBY_HEIGHT", BLOBBY_HEIGHT);
	setLuaGlobal("CONST_BLOBBY_GRAVITY", -GRAVITATION);
	setLuaGlobal("CONST_NET_HEIGHT", 600 - NET_SPHERE_POSITION);
	setLuaGlobal("CONST_NET_RADIUS", NET_RADIUS);
	setLuaGlobal("NO_PLAYER", NO_PLAYER);
	setLuaGlobal("LEFT_PLAYER", LEFT_PLAYER);
	setLuaGlobal("RIGHT_PLAYER", RIGHT_PLAYER);
}

void IScriptableComponent::updateGameState(const DuelMatchState& state)
{
	*mGameState = state;
}

// helpers
inline IScriptableComponent* getScriptComponent(lua_State* state)
{
	lua_pushliteral(state, "__C++_ScriptComponent__");
	lua_gettable(state, LUA_REGISTRYINDEX);
	void* result = lua_touserdata(state, -1);
	lua_pop(state, 1);
	return (IScriptableComponent*)result;
}

enum class VectorType
{
	POSITION,
	VELOCITY
};

int lua_pushvector(lua_State* state, const Vector2& v, VectorType type)
{
	if( type == VectorType::VELOCITY )
	{
		lua_pushnumber( state, v.x );
		lua_pushnumber( state, -v.y );
	}
	 else if ( type == VectorType::POSITION )
	{
		lua_pushnumber( state, v.x );
		lua_pushnumber( state, 600 - v.y );
	}
	return 2;
}

static inline int lua_toint(lua_State* state, int index)
{
	double value = lua_tonumber(state, index);
	return int(value + (value > 0 ? 0.5 : -0.5));
}

// Access struct to get to the privates of IScriptableComponent
struct IScriptableComponent::Access
{
	static DuelMatchState* getMatch( lua_State* state )
	{
		auto sc = getScriptComponent( state );
		return sc->mGameState.get();
	}
};

inline DuelMatchState* getMatch( lua_State* s )  { return IScriptableComponent::Access::getMatch(s); };

// standard lua functions
int get_ball_pos(lua_State* state)
{
	auto s = getMatch( state );
	return lua_pushvector(state, s->worldState.ballPosition, VectorType::POSITION);
}

int get_ball_vel(lua_State* state)
{
	auto s = getMatch( state );
	return lua_pushvector(state, s->worldState.ballVelocity, VectorType::VELOCITY);
}

int get_blob_pos(lua_State* state)
{
	auto s = getMatch( state );
	PlayerSide side = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	assert( side == LEFT_PLAYER || side == RIGHT_PLAYER );
	return lua_pushvector(state, s->worldState.blobPosition[side], VectorType::POSITION);
}

int get_blob_vel(lua_State* state)
{
	auto s = getMatch( state );
	PlayerSide side = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	assert( side == LEFT_PLAYER || side == RIGHT_PLAYER );
	return lua_pushvector(state, s->worldState.blobVelocity[side], VectorType::VELOCITY);
}

int get_score( lua_State* state )
{
	auto s = getMatch( state );
	PlayerSide side = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	assert( side == LEFT_PLAYER || side == RIGHT_PLAYER );
	if( side == LEFT_PLAYER )
		lua_pushinteger(state, s->logicState.leftScore);
	else
		lua_pushinteger(state, s->logicState.rightScore);
	return 1;
}

int get_touches( lua_State* state )
{
	auto s = getMatch( state );
	PlayerSide side = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	assert( side == LEFT_PLAYER || side == RIGHT_PLAYER );
	lua_pushinteger(state, s->logicState.hitCount[side]);
	return 1;
}


int lua_print(lua_State* state)
{
	int count = lua_gettop(state);
	for( int i = 1; i <= count; ++i)
	{
		lua_pushvalue(state, i);
		const char* str = lua_tostring(state, -1);
		std::cout << (i != 1 ? ", " : "");
		if(str)
		 std::cout << str;
		else
		std::cout << "[" << lua_typename(state, lua_type(state, -1)) << "]";
	}
	std::cout << "\n";
	lua_pop(state, lua_gettop(state));
	return 0;
}

void IScriptableComponent::setGameFunctions()
{
	lua_register(mState, "get_ball_pos", get_ball_pos);
	lua_register(mState, "get_ball_vel", get_ball_vel);
	lua_register(mState, "get_blob_pos", get_blob_pos);
	lua_register(mState, "get_blob_vel", get_blob_vel);
	lua_register(mState, "get_score", get_score);
	lua_register(mState, "get_touches", get_touches);
}
