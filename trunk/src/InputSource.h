#pragma once

// This struct exists for easy exchange of a single player input frame

struct PlayerInput
{
	PlayerInput()
	{
		left = false;
		right = false;
		up = false;
	}
	
	PlayerInput(bool l, bool r, bool u)
	{
		left = l;
		right = r;
		up = u;
	}

	bool left : 1;
	bool right : 1;
	bool up : 1;
};

// This class abstracts several possible input sources, like local input 
// from InputManager, input from a scripted player or input over network
// which in turn can all be recorded over a decorator
// It should be only called once per frame because some implementations
// may use this to activate a refresh routine on their actual source

class InputSource
{
public:
	virtual PlayerInput getInput() = 0;
};

