/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#pragma once

#include "PhysicWorld.h"
#include "Vector.h"

// This class represents a single game between two players
// It applys the rules itself and provides an interface for querying
// different parameters. For this purpose it is designed as something
// similar to a singleton, but it can be instantiated
// multiple times on a server or be completely unavailable

class InputSource;

class DuelMatch
{
public:
	// This constructor takes the input sources used to get player input
	// The parameter output tells DuelMatch if it should report its
	// results to the user through RenderManager and SoundManager.
	// A deacivation of the output is useful on dedicated servers

	// Allthough DuelMatch is only ment to implement the ruleset
	// and combine input and physics, it would be unpractical to
	// export the attributes necessary for output.

	// If global is true, the instance registered as the main
	// game and can be accessed from everywhere. There can only
	// be one global game at a time, otherwise an assertion fails.

	DuelMatch(InputSource* linput, InputSource* rinput,
				bool output, bool global);

	~DuelMatch();

	// Allthough DuelMatch can be instantiated multiple times, a
	// singleton may be registered for the purpose of scripted or
	// interactive input. Note this can return 0.
	static DuelMatch* getMainGame();

	// This steps through one frame
	void step();

	// This reports the index of the winning player and -1 if the
	// game is still running
	PlayerSide winningPlayer();

	// This methods report the current game state and a useful for
	// the input manager, which needs information about the blob
	// positions and for lua export, which makes them accessable
	// for scripted input sources

	int getScore(PlayerSide player);
	PlayerSide getServingPlayer();

	int getHitcount(PlayerSide player);

	Vector2 getBallPosition();
	Vector2 getBallVelocity();
	Vector2 getBlobPosition(PlayerSide player);

	bool getBallDown();
	bool getBallActive();

	// Estimates Ball impact point
	float getBallEstimation();

	Vector2 getBallTimeEstimation(int steps);

	// This functions returns true if the player launched
	// and is jumping at the moment
	bool getBlobJump(PlayerSide player);

	// Set a new state received from server over a RakNet BitStream
	void setState(RakNet::BitStream* stream);

	//Input stuff for recording and playing replays
	const PlayerInput* getPlayersInput();
	void setPlayersInput(PlayerInput* input);

	void setPlayerName(std::string name);
	void setOpponentName(std::string name);
	std::string getPlayerName();
	std::string getOpponentName();
	void setServingPlayer(PlayerSide side);

private:
	static DuelMatch* mMainGame;
	bool mGlobal;

	PhysicWorld mPhysicWorld;

	InputSource* mLeftInput;
	InputSource* mRightInput;

	int mLeftScore;
	int mRightScore;
	PlayerSide mServingPlayer;

	int mLeftHitcount;
	int mRightHitcount;

	int mSquishLeft;
	int mSquishRight;

	bool mBallDown;

	bool mOutput;
	int mWinningPlayer;
	int mScoreToWin;

	std::string mPlayerName;
	std::string mOpponentName;
};
