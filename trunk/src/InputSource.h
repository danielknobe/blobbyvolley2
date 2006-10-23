#pragma once

#include <iostream>

// This struct exists for easy exchange of a single player input frame

struct PlayerInput
{
	PlayerInput()
	{
		left = false;
		right = false;
		up = false;
	}
	
	PlayerInput(const std::string& string);
	
	PlayerInput(bool l, bool r, bool u)
	{
		left = l;
		right = r;
		up = u;
	}
	
	void swap()
	{
		bool tmp = left;
		left = right;
		right = tmp;	
	}

	bool left;
	bool right;
	bool up;
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
	virtual ~InputSource()
	{
	}
};

// This class serves as a dummy input source.
// It can optionally be set from outside if low level input access
// is required at a higher level

class DummyInputSource : public InputSource
{
public:
	virtual PlayerInput getInput()
	{
		return mInput;
	}
	virtual ~DummyInputSource()
	{
	}
	
	void setInput(PlayerInput input)
	{
		mInput = input;
	}
	
private:
	PlayerInput mInput;
};

// This operator converts a PlayerInput structure in a packed string
// suitable for saving

std::string& operator<< (std::string& out, const PlayerInput& input);
