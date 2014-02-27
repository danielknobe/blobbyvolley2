/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

/* header include */
#include "DuelMatch.h"

/* includes */
#include <cassert>

#include <boost/make_shared.hpp>

#include "DuelMatchState.h"
#include "MatchEvents.h"
#include "PhysicWorld.h"
#include "GenericIO.h"
#include "GameConstants.h"
#include "InputSource.h"

/* implementation */

DuelMatch::DuelMatch(bool remote, std::string rules) :
		// we send a pointer to an unconstructed object here!
		mLogic(createGameLogic(rules, this)),
		mPaused(false),
		events(0),
		external_events(0),
		mRemote(remote)
{
	mPhysicWorld.reset( new PhysicWorld() );

	setInputSources(boost::make_shared<InputSource>(), boost::make_shared<InputSource>());
}

void DuelMatch::setPlayers( PlayerIdentity lplayer, PlayerIdentity rplayer)
{
	mPlayers[LEFT_PLAYER] = lplayer;
	mPlayers[RIGHT_PLAYER] = rplayer;
}

void DuelMatch::setInputSources(boost::shared_ptr<InputSource> linput, boost::shared_ptr<InputSource> rinput )
{
	if(linput)
		mInputSources[LEFT_PLAYER] = linput;

	if(rinput)
		mInputSources[RIGHT_PLAYER] = rinput;

	mInputSources[LEFT_PLAYER]->setMatch(this);
	mInputSources[RIGHT_PLAYER]->setMatch(this);
}

void DuelMatch::reset()
{
	mPhysicWorld.reset(new PhysicWorld());
	mLogic = mLogic->clone();
}

DuelMatch::~DuelMatch()
{
}

void DuelMatch::setRules(std::string rulesFile)
{
	mLogic = createGameLogic(rulesFile, this);
}


void DuelMatch::step()
{
	events = external_events;

	// in pause mode, step does nothing
	if(mPaused)
		return;

	mTransformedInput[LEFT_PLAYER] = mInputSources[LEFT_PLAYER]->updateInput();
	mTransformedInput[RIGHT_PLAYER] = mInputSources[RIGHT_PLAYER]->updateInput();

	if(!mRemote)
	{
		mTransformedInput[LEFT_PLAYER] = mLogic->transformInput( mTransformedInput[LEFT_PLAYER], LEFT_PLAYER );
		mTransformedInput[RIGHT_PLAYER] = mLogic->transformInput( mTransformedInput[RIGHT_PLAYER], RIGHT_PLAYER );
	}

	// do steps in physic an logic
	mLogic->step();
	int physicEvents = mPhysicWorld->step( mTransformedInput[LEFT_PLAYER], mTransformedInput[RIGHT_PLAYER],
											mLogic->isBallValid(), mLogic->isGameRunning() );

	// check for all hit events
	if(!mRemote)
		events |= physicEvents;

	// process events
	if(events & EVENT_LEFT_BLOBBY_HIT)
		mLogic->onBallHitsPlayer(LEFT_PLAYER);

	if(events & EVENT_RIGHT_BLOBBY_HIT)
		mLogic->onBallHitsPlayer(RIGHT_PLAYER);

	if(events & EVENT_BALL_HIT_LEFT_GROUND)
		mLogic->onBallHitsGround(LEFT_PLAYER);

	if(events & EVENT_BALL_HIT_RIGHT_GROUND)
		mLogic->onBallHitsGround(RIGHT_PLAYER);

	if(events & EVENT_BALL_HIT_LEFT_WALL)
		mLogic->onBallHitsWall(LEFT_PLAYER);

	if(events & EVENT_BALL_HIT_RIGHT_WALL)
		mLogic->onBallHitsWall(RIGHT_PLAYER);

	if(events & EVENT_BALL_HIT_NET_LEFT)
		mLogic->onBallHitsNet(LEFT_PLAYER);

	if(events & EVENT_BALL_HIT_NET_RIGHT)
		mLogic->onBallHitsNet(RIGHT_PLAYER);

	if(events & EVENT_BALL_HIT_NET_TOP)
		mLogic->onBallHitsNet(NO_PLAYER);


	switch(mLogic->getLastErrorSide()){
		case LEFT_PLAYER:
			events |= EVENT_ERROR_LEFT;
		case RIGHT_PLAYER:
			// if the error was caused by the right player
			// reset EVENT_ERROR_LEFT
			events &= ~EVENT_ERROR_LEFT;
			events |= EVENT_ERROR_RIGHT;
			mPhysicWorld->setBallVelocity( mPhysicWorld->getBallVelocity().scale(0.6) );
			break;
		default:
			if ((events & EVENT_BALL_HIT_GROUND) && !mLogic->isBallValid())
			{
				mPhysicWorld->setBallVelocity( mPhysicWorld->getBallVelocity().scale(0.6) );
			}
			break;
	}

	// if the round is finished, we
	// reset BallDown, reset the World
	// to let the player serve
	// and trigger the EVENT_RESET
	if (!mLogic->isBallValid() && canStartRound(mLogic->getServingPlayer()))
	{
		resetBall( mLogic->getServingPlayer() );
		mLogic->onServe();
		events |= EVENT_RESET;
	}

	// reset external events
	external_events = 0;
}

void DuelMatch::setScore(int left, int right)
{
	mLogic->setScore(LEFT_PLAYER, left);
	mLogic->setScore(RIGHT_PLAYER, right);
}

void DuelMatch::setLastHitIntensity(float intensity)
{
	mPhysicWorld->setLastHitIntensity(intensity);
}

void DuelMatch::trigger(int event)
{
	external_events |= event;
}

void DuelMatch::resetTriggeredEvents()
{
	external_events = 0;
}

void DuelMatch::pause()
{
	mLogic->onPause();
	mPaused = true;
}

void DuelMatch::unpause()
{
	mLogic->onUnPause();
	mPaused = false;
}

PlayerSide DuelMatch::winningPlayer()
{
	return mLogic->getWinningPlayer();
}

int DuelMatch::getHitcount(PlayerSide player) const
{
	if (player == LEFT_PLAYER)
		return mLogic->getTouches(LEFT_PLAYER);
	else if (player == RIGHT_PLAYER)
		return mLogic->getTouches(RIGHT_PLAYER);
	else
		return 0;
}

int DuelMatch::getScore(PlayerSide player) const
{
	return mLogic->getScore(player);
}

int DuelMatch::getScoreToWin() const
{
	return mLogic->getScoreToWin();
}

bool DuelMatch::getBallDown() const
{
	return !mLogic->isBallValid();
}

bool DuelMatch::getBallActive() const
{
	return mLogic->isGameRunning();
}

bool DuelMatch::getBlobJump(PlayerSide player) const
{
	return !mPhysicWorld->blobHitGround(player);
}

Vector2 DuelMatch::getBlobPosition(PlayerSide player) const
{
	if (player == LEFT_PLAYER || player == RIGHT_PLAYER)
		return mPhysicWorld->getBlobPosition(player);
	else
		return Vector2(0.0, 0.0);
}

Vector2 DuelMatch::getBlobVelocity(PlayerSide player) const
{
	if (player == LEFT_PLAYER || player == RIGHT_PLAYER)
		return mPhysicWorld->getBlobVelocity(player);
	else
		return Vector2(0.0, 0.0);
}

Vector2 DuelMatch::getBallPosition() const
{
	return mPhysicWorld->getBallPosition();
}

Vector2 DuelMatch::getBallVelocity() const
{
	return mPhysicWorld->getBallVelocity();
}

PlayerSide DuelMatch::getServingPlayer() const
{	// NO_PLAYER hack was moved into ScriptedInpurSource.cpp
	return mLogic->getServingPlayer();
}

void DuelMatch::setState(const DuelMatchState& state)
{
	mPhysicWorld->setState(state.worldState);
	mLogic->setState(state.logicState);

	mTransformedInput[LEFT_PLAYER] = state.playerInput[LEFT_PLAYER];
	mTransformedInput[RIGHT_PLAYER] = state.playerInput[RIGHT_PLAYER];

	mInputSources[LEFT_PLAYER]->setInput( mTransformedInput[LEFT_PLAYER] );
	mInputSources[RIGHT_PLAYER]->setInput( mTransformedInput[RIGHT_PLAYER] );

	events &= ~EVENT_ERROR;
	switch (state.errorSide)
	{
		case LEFT_PLAYER:
			events |= EVENT_ERROR_LEFT;
			break;
		case RIGHT_PLAYER:
			events |= EVENT_ERROR_RIGHT;
			break;
	}
}

DuelMatchState DuelMatch::getState() const
{
	DuelMatchState state;
	state.worldState = mPhysicWorld->getState();
	state.logicState = mLogic->getState();
	state.playerInput[LEFT_PLAYER] = mTransformedInput[LEFT_PLAYER];
	state.playerInput[RIGHT_PLAYER] = mTransformedInput[RIGHT_PLAYER];

	state.errorSide = (events & EVENT_ERROR_LEFT) ? LEFT_PLAYER : (events & EVENT_ERROR_RIGHT) ? RIGHT_PLAYER : NO_PLAYER;

	return state;
}

void DuelMatch::setServingPlayer(PlayerSide side)
{
	mLogic->setServingPlayer( side );
	resetBall( side );
	mLogic->onServe( );
}

const Clock& DuelMatch::getClock() const
{
	return mLogic->getClock();
}

Clock& DuelMatch::getClock()
{
	return mLogic->getClock();
}

boost::shared_ptr<InputSource> DuelMatch::getInputSource(PlayerSide player) const
{
	return mInputSources[player];
}

void DuelMatch::resetBall( PlayerSide side )
{
	if (side == LEFT_PLAYER)
		mPhysicWorld->setBallPosition( Vector2(200, STANDARD_BALL_HEIGHT) );
	else if (side == RIGHT_PLAYER)
		mPhysicWorld->setBallPosition( Vector2(600, STANDARD_BALL_HEIGHT) );
	else
		mPhysicWorld->setBallPosition( Vector2(400, 450) );

	mPhysicWorld->setBallVelocity( Vector2(0, 0) );
	mPhysicWorld->setBallAngularVelocity( (side == RIGHT_PLAYER ? -1 : 1) * STANDARD_BALL_ANGULAR_VELOCITY );
	mPhysicWorld->setLastHitIntensity(0.0);
}

bool DuelMatch::canStartRound(PlayerSide servingPlayer) const
{
	Vector2 ballVelocity = mPhysicWorld->getBallVelocity();
	return (mPhysicWorld->blobHitGround(servingPlayer) && ballVelocity.y < 1.5 &&
				ballVelocity.y > -1.5 && mPhysicWorld->getBallPosition().y > 430);
}

PlayerIdentity DuelMatch::getPlayer(PlayerSide player) const
{
	return mPlayers[player];
}

PlayerIdentity& DuelMatch::getPlayer(PlayerSide player)
{
	return mPlayers[player];
}
