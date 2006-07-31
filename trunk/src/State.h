#pragma once

#include "Global.h"
#include "UserConfig.h"
#include "PhysicWorld.h"

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
	PhysicWorld mPhysicWorld;
	
	InputSource* mLeftInput;
	InputSource* mRightInput;
	
	int mLeftScore;
	int mRightScore;
	int mServingPlayer;
	
	int mLeftHitcount;
	int mRightHitcount;
	
	int mSquishLeft;
	int mSquishRight;
	Color mLeftColor;
	Color mRightColor;
	bool mLeftOscillate;
	bool mRightOscillate;

public:
	LocalGameState();
	virtual ~LocalGameState();
	virtual void step();
	
};

