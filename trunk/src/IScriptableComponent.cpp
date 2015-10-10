#include "IScriptableComponent.h"

#include "lua/lua.hpp"

#include "Global.h"
#include "GameConstants.h"
#include "DuelMatch.h"
#include "DuelMatchState.h"
#include "FileRead.h"

#include <iostream>

// fwd decl
int lua_print(lua_State* state);

IScriptableComponent::IScriptableComponent() :
	mState(luaL_newstate())
{
	// register this in the lua registry
	lua_pushliteral(mState, "__C++_ScriptComponent__");
	lua_pushlightuserdata(mState, (void*)this);
	lua_settable(mState, LUA_REGISTRYINDEX);

	lua_register(mState, "print", lua_print);

	// open math lib
	luaL_requiref(mState, "math", luaopen_math, 1);
	luaL_requiref(mState, "base", luaopen_base, 1);
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
	setLuaGlobal("CONST_BLOBBY_JUMP", BLOBBY_JUMP_ACCELERATION);
	setLuaGlobal("CONST_BLOBBY_BODY_RADIUS", BLOBBY_LOWER_RADIUS);
	setLuaGlobal("CONST_BLOBBY_HEAD_RADIUS", BLOBBY_UPPER_RADIUS);
	setLuaGlobal("CONST_BLOBBY_HEAD_OFFSET", BLOBBY_UPPER_SPHERE);
	setLuaGlobal("CONST_BLOBBY_BODY_OFFSET", -BLOBBY_LOWER_SPHERE);
	setLuaGlobal("CONST_BALL_HITSPEED", BALL_COLLISION_VELOCITY);
	setLuaGlobal("CONST_BLOBBY_HEIGHT", BLOBBY_HEIGHT);
	setLuaGlobal("CONST_BLOBBY_GRAVITY", -GRAVITATION);
	setLuaGlobal("CONST_BLOBBY_SPEED", BLOBBY_SPEED);
	setLuaGlobal("CONST_NET_HEIGHT", 600 - NET_SPHERE_POSITION);
	setLuaGlobal("CONST_NET_RADIUS", NET_RADIUS);
	setLuaGlobal("NO_PLAYER", NO_PLAYER);
	setLuaGlobal("LEFT_PLAYER", LEFT_PLAYER);
	setLuaGlobal("RIGHT_PLAYER", RIGHT_PLAYER);
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
	static DuelMatch* getMatch( lua_State* state )
	{
		auto sc = getScriptComponent( state );
		return sc->mGame;
	}

	static PhysicWorld* getWorld( lua_State* state )
	{
		auto sc = getScriptComponent( state );
		return &sc->mDummyWorld;
	}
};

inline DuelMatch* getMatch( lua_State* s )  { return IScriptableComponent::Access::getMatch(s); };
inline PhysicWorld* getWorld( lua_State* s )  { return IScriptableComponent::Access::getWorld(s); };

// standard lua functions
int get_ball_pos(lua_State* state)
{
	auto s = getMatch( state );
	return lua_pushvector(state, s->getBallPosition(), VectorType::POSITION);
}

int get_ball_vel(lua_State* state)
{
	auto s = getMatch( state );
	return lua_pushvector(state, s->getBallVelocity(), VectorType::VELOCITY);
}

int set_ball_data(lua_State* state)
{
	auto s = getMatch( state );
	lua_checkstack(state, 4);
	float x = lua_tonumber( state, 1);
	float y = lua_tonumber( state, 2);
	float vx = lua_tonumber( state, 3);
	float vy = lua_tonumber( state, 4);
	Vector2 p{x, 600 - y};
	Vector2 v{vx, -vy};
	DuelMatchState m = s->getState();
	m.worldState.ballPosition = p;
	m.worldState.ballVelocity = v;
	m.logicState.isGameRunning = true;
	m.logicState.isBallValid = true;
	s->setState(m);
	return 0;
}

int set_blob_data(lua_State* state)
{
	auto s = getMatch( state );
	lua_checkstack(state, 5);
	int side = lua_tointeger(state, 1);
	float x = lua_tonumber( state, 2);
	float y = lua_tonumber( state, 3);
	float vx = lua_tonumber( state, 4);
	float vy = lua_tonumber( state, 5);
	Vector2 p{x, 600 - y};
	Vector2 v{vx, -vy};
	DuelMatchState m = s->getState();
	m.worldState.blobPosition[side] = p;
	m.worldState.blobVelocity[side] = v;
	s->setState(m);
	return 0;
}

int get_blob_pos(lua_State* state)
{
	auto s = getMatch( state );
	PlayerSide side = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	assert( side == LEFT_PLAYER || side == RIGHT_PLAYER );
	return lua_pushvector(state, s->getBlobPosition(side), VectorType::POSITION);
}

int get_blob_vel(lua_State* state)
{
	auto s = getMatch( state );
	PlayerSide side = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	assert( side == LEFT_PLAYER || side == RIGHT_PLAYER );
	return lua_pushvector(state, s->getBlobVelocity(side), VectorType::VELOCITY);
}

int get_score( lua_State* state )
{
	auto s = getMatch( state );
	PlayerSide side = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	assert( side == LEFT_PLAYER || side == RIGHT_PLAYER );
	lua_pushinteger(state, s->getScore(side));
	return 1;
}

int get_touches( lua_State* state )
{
	auto s = getMatch( state );
	PlayerSide side = (PlayerSide)lua_toint(state, -1);
	lua_pop(state, 1);
	assert( side == LEFT_PLAYER || side == RIGHT_PLAYER );
	lua_pushinteger(state, s->getTouches(side));
	return 1;
}

int get_ball_valid( lua_State* state )
{
	auto s = getMatch( state );
	lua_pushboolean(state, !s->getBallDown());
	return 1;
}

int get_game_running( lua_State* state )
{
	auto s = getMatch( state );
	lua_pushboolean(state, s->getBallActive());
	return 1;
}

int get_serving_player( lua_State* state )
{
	auto s = getMatch( state );
	lua_pushinteger(state, s->getServingPlayer());
	return 1;
}

int simulate_steps( lua_State* state )
{
	/// \todo should we gather and return all events that happen to the ball on the way?
	PhysicWorld* world = getWorld( state );
	// get the initial ball settings
	lua_checkstack(state, 5);
	int steps = lua_tointeger( state, 1);
	float x = lua_tonumber( state, 2);
	float y = lua_tonumber( state, 3);
	float vx = lua_tonumber( state, 4);
	float vy = lua_tonumber( state, 5);
	lua_pop( state, 5);

	Vector2 v{vx, -vy};
	world->setBallPosition( Vector2{x, 600 - y} );
	world->setBallVelocity( Vector2{vx, -vy});
	for(int i = 0; i < steps; ++i)
	{
		// set ball valid to false to ignore blobby bounces
		world->step(PlayerInput(), PlayerInput(), false, true);
	}

	int ret = lua_pushvector(state, world->getBallPosition(), VectorType::POSITION);
	ret += lua_pushvector(state, world->getBallVelocity(), VectorType::VELOCITY);
	return ret;
}

int simulate_until(lua_State* state)
{
	/// \todo should we gather and return all events that happen to the ball on the way?
	PhysicWorld* world = getWorld( state );
	// get the initial ball settings
	lua_checkstack(state, 6);
	float x = lua_tonumber( state, 1);
	float y = lua_tonumber( state, 2);
	float vx = lua_tonumber( state, 3);
	float vy = lua_tonumber( state, 4);
	std::string axis = lua_tostring( state, 5 );
	const float coordinate = lua_tonumber( state, 6 );
	lua_pop( state, 6 );

	const float ival = axis == "x" ? x : y;
	if(axis != "x" && axis != "y")
	{
		lua_pushstring(state, "invalid condition specified: choose either 'x' or 'y'");
		lua_error(state);
	}
	const bool init = ival < coordinate;

	// setup the world
	Vector2 v{vx, -vy};
	world->setBallPosition( Vector2{x, 600 - y} );
	world->setBallVelocity( Vector2{vx, -vy});

	int steps = 0;
	while(coordinate != ival && steps < 75 * 5)
	{
		steps++;
		// set ball valid to false to ignore blobby bounces
		world->step(PlayerInput(), PlayerInput(), false, true);
		// check for the condition
		auto pos = world->getBallPosition();
		float v = axis == "x" ? pos.x : 600 - pos.y;
		if( (v < coordinate) != init )
			break;
	}
	// indicate failure
	if(steps == 75 * 5)
		steps = -1;

	lua_pushinteger(state, steps);
	int ret = 1;
	ret += lua_pushvector(state, world->getBallPosition(), VectorType::POSITION);
	ret += lua_pushvector(state, world->getBallVelocity(), VectorType::VELOCITY);
	return ret;
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
	lua_register(mState, "is_ball_valid", get_ball_valid);
	lua_register(mState, "is_game_running", get_game_running);
	lua_register(mState, "get_serving_player", get_serving_player);
	lua_register(mState, "simulate", simulate_steps);
	lua_register(mState, "simulate_until", simulate_until);

	#ifndef NDEBUG
	// only enable this function in debug builds.
	lua_register(mState, "set_ball_data", set_ball_data);
	lua_register(mState, "set_blob_data", set_blob_data);
	#endif
}
