#pragma once

#include "Global.h"
#include "UserConfig.h"

class DuelMatch;
class InputSource;

class State
{
private:
	
protected:
	State();
	static State* mCurrentState;
	
public:
	virtual ~State() {}
	virtual void step() = 0;
	
	static State* getCurrentState();

};

class MainMenuState : public State
{
private:
	int mStartButton;
	int mExitButton;
public:
	MainMenuState();
	virtual ~MainMenuState();
	
	virtual void step();
};

class OptionMenuState : public State
{
private:
	
public:
		
};

class WinState : public State
{
private:
	
public:
	WinState(int winningPlayer);	
	virtual ~WinState() {}
	virtual void step();	
};

class LocalGameState : public State
{
private:
	InputSource* mLeftInput;
	InputSource* mRightInput;
	
	Color mLeftColor;
	Color mRightColor;
	bool mLeftOscillate;
	bool mRightOscillate;
	
	DuelMatch* mMatch;

public:
	LocalGameState(GameMode mode);
	virtual ~LocalGameState();
	virtual void step();
	
};

