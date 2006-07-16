#pragma once

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
	UserConfig& mGameConfig;
	
	PhysicWorld mPhysicWorld;
	
	InputSource* mLeftInput;
	InputSource* mRightInput;
	
	int mLeftScore;
	int mRightScore;
	int mServingPlayer;
	
	int mLeftHitcount;
	int mRightHitcount;
	
	int mSquish;
public:
	LocalGameState(UserConfig& gameConfig);
	virtual ~LocalGameState();
	virtual void step();
	
};

