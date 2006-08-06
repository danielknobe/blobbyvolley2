#pragma once

#include "Global.h"
#include "InputSource.h"

#include <iostream>

// ScriptedInputSource provides an implementation of InputSource, which uses
// Lua scripts to get its input. The given script is automatically initialised
// and provided with an interface to the game.

// The Lua API is designed very closely to the script API of the original
// blobby volley. The main difference is that instead of regularly pausing
// an endless loop, the lua scripts must provide the function 'step', which
// is called once a frame.

// The old API is a bit ugly at the moment, especially because it is
// unaware of the multiple return value feature of Lua.
// Some functions may be later moved to a 'deprecated' API and be
// replaced with nicer functions. But for now, easy porting
// of the old scripts is the main target, so we begin with rebuilding
// the old script interface.

// The following functions are provided to Lua scripts:
// (note: this documentation is guessed from the original scripts
// and may be incomplete or wrong)
//
// side() : Tells the side of the controlled player
//
// touches() : Number of own ball touches since last ball exchange
//      Note: This could also contain the opponents ball touches
//
// balldown() : (unsure) Tells whether the ball is valid and can be touched
//
// touching() : (???) I absolutely do not know what this does yet.
//              It could have something to do with touching the walls.
//
// launched() : Tells whether the player is jumping at the moment
//
// debug(number) : (???) This could have something to do with reporting
//                 computation results for debugging purposes. If this
//                 is the case, we do not need this, because lua provides
//                 better debugging facilities 

// Note: These mathematical functions could also be provided by the 
//       standard Lua mathematical library.
// abs(number) : Returns the unsigned value of the given number
//
// random(number) : (Unsure) This returns a random number in a range which
//               one limit equal to the parameter. I am quite unsure about
//               this function. The return value could be either an integer
//               or a float, but this unimportant because Lua treats all
//               numbers as doubles by default.

// stop() : Releases the side steering keys
//
// stopjump() : Releases the jump key
//
// jump() : This presses the jump key, until stopjump() is called
//
// moveto(int) : Press appropriate keys to move to the specified y position
//
// left() : press left move key
//
// right () : press right move key
//      Note : The last two functions are commented out in the original
//             scripts in favor of moveto(). But some nifty newly created
//             scripts could have a need for low-level access to the controls,
//             so they are kept for now.


// ballx() : x component of the ball position
//
// bally() : y component of the ball position
//
// bspeedx() : x component of the ball velocity
//
// bspeedy() : y component of the ball velocity
//
// posx() : x component of own position
//
// posy() : y component of own position
//
// estimate() : (Unsure) y component of estimated ball impact point

class lua_State;

class ScriptedInputSource : public InputSource
{
public:
	// The constructor automatically loads and initializes the script
	// with the given filename. The side parameter tells the script
	// which side is it on.
	ScriptedInputSource(const std::string& filename, PlayerSide side);
	~ScriptedInputSource();
	
	virtual PlayerInput getInput();
	
private:
	static int side(lua_State* state);
	static int touches(lua_State* state);
	static int balldown(lua_State* state);
	static int touching(lua_State* state);
	static int launched(lua_State* state);
	static int debug(lua_State* state);
	static int abs(lua_State* state);
	static int random(lua_State* state);
	static int stop(lua_State* state);
	static int stopjump(lua_State* state);
	static int jump(lua_State* state);
	static int left(lua_State* state);
	static int right(lua_State* state);
	static int moveto(lua_State* state);
	static int ballx(lua_State* state);
	static int bally(lua_State* state);
	static int bspeedx(lua_State* state);
	static int bspeedy(lua_State* state);
	static int posx(lua_State* state);
	static int posy(lua_State* state);
	static int estimate(lua_State* state);

	lua_State* mState;
};
