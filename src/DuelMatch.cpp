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
#include "DuelMatchState.h"
#include "MatchEvents.h"
#include "PhysicWorld.h"
#include "GenericIO.h"
#include "GameConstants.h"
#include "InputSource.h"
#include "IUserConfigReader.h"
#include "Clock.h"

/* implementation */

DuelMatch::DuelMatch(bool remote, const std::string& rules, int score_to_win) :
		mPaused(false),
		mRemote(remote)
{
	if(score_to_win == 0) {
		score_to_win = IUserConfigReader::createUserConfigReader("config.xml")->getInteger("scoretowin");
	}

	mLogic = createGameLogic(rules, score_to_win);
	mPhysicWorld.reset( new PhysicWorld() );

	setInputSources(std::make_shared<InputSource>(), std::make_shared<InputSource>());

	if(!mRemote)
		mPhysicWorld->setEventCallback( [this]( const MatchEvent& event ) { mNewEvents.push_back(event); } );
}

void DuelMatch::setPlayers(PlayerIdentity left_player, PlayerIdentity right_player)
{
	mPlayers[LEFT_PLAYER] = std::move(left_player);
	mPlayers[RIGHT_PLAYER] = std::move(right_player);
}

void DuelMatch::setInputSources(std::shared_ptr<InputSource> left_input, std::shared_ptr<InputSource> right_input )
{
	if(left_input)
		mInputSources[LEFT_PLAYER] = std::move(left_input);

	if(right_input)
		mInputSources[RIGHT_PLAYER] = std::move(right_input);
}

void DuelMatch::reset()
{
	mPhysicWorld.reset(new PhysicWorld());
	mLogic = mLogic->clone();
}

DuelMatch::~DuelMatch() = default;

void DuelMatch::setRules(const std::string& rulesFile, int score_to_win)
{
	if( score_to_win == 0)
		score_to_win = getScoreToWin();
	mLogic = createGameLogic(rulesFile, score_to_win);
}


void DuelMatch::step()
{
	// in pause mode, step does nothing
	if(mPaused)
		return;

	mTransformedInput[LEFT_PLAYER] = mInputSources[LEFT_PLAYER]->updateInput().toPlayerInput(this);
	mTransformedInput[RIGHT_PLAYER] = mInputSources[RIGHT_PLAYER]->updateInput().toPlayerInput(this);

	if(!mRemote)
	{
		mTransformedInput[LEFT_PLAYER] = mLogic->transformInput( mTransformedInput[LEFT_PLAYER], LEFT_PLAYER );
		mTransformedInput[RIGHT_PLAYER] = mLogic->transformInput( mTransformedInput[RIGHT_PLAYER], RIGHT_PLAYER );
	}

	// do steps in physic and logic
	mPhysicWorld->step( mTransformedInput[LEFT_PLAYER], mTransformedInput[RIGHT_PLAYER],
						mLogic->isBallValid(), mLogic->isGameRunning() );
	mLogic->step( getState() );

	// check for all hit events

	// process events
	// process all physics events and relay them to logic
	for( const auto& event : mNewEvents )
	{
		switch( event.event )
		{
			case MatchEvent::BALL_HIT_BLOB:
				mLogic->onBallHitsPlayer( event.side );
				break;
			case MatchEvent::BALL_HIT_GROUND:
				mLogic->onBallHitsGround( event.side );
				// if not valid, reduce velocity
				if(!mLogic->isBallValid())
					mPhysicWorld->setBallVelocity( mPhysicWorld->getBallVelocity().scale(0.6) );
				break;
			case MatchEvent::BALL_HIT_NET:
				mLogic->onBallHitsNet( event.side );
				break;
			case MatchEvent::BALL_HIT_NET_TOP:
				mLogic->onBallHitsNet( NO_PLAYER );
				break;
			case MatchEvent::BALL_HIT_WALL:
				mLogic->onBallHitsWall( event.side );
				break;
			default:
				break;
		}
	}

	auto errorside = mLogic->getLastErrorSide();
	if(errorside != NO_PLAYER)
	{
		mNewEvents.emplace_back( MatchEvent::PLAYER_ERROR, errorside, 0 );
		mPhysicWorld->setBallVelocity( mPhysicWorld->getBallVelocity().scale(0.6) );
	}

	// if the round is finished, we
	// reset BallDown, reset the World
	// to let the player serve
	// and trigger the EVENT_RESET
	if (!mLogic->isBallValid() && canStartRound(mLogic->getServingPlayer()))
	{
		resetBall( mLogic->getServingPlayer() );
		mLogic->onServe();
		mNewEvents.emplace_back( MatchEvent::RESET_BALL, NO_PLAYER, 0 );
	}

	// reset events
	std::move(begin(mNewEvents), end(mNewEvents), std::back_inserter(mLastEvents));
	mNewEvents.clear();
}

void DuelMatch::setScore(int left, int right)
{
	mLogic->setScore(LEFT_PLAYER, left);
	mLogic->setScore(RIGHT_PLAYER, right);
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

PlayerSide DuelMatch::winningPlayer() const
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

int DuelMatch::getTouches(PlayerSide player) const
{
	return mLogic->getTouches(player);
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
		return {0.f, 0.f};
}

float DuelMatch::getBlobState(PlayerSide player) const
{
	if (player == LEFT_PLAYER || player == RIGHT_PLAYER)
		return mPhysicWorld->getBlobState(player);
	else
		return 0.f;
}

Vector2 DuelMatch::getBlobVelocity(PlayerSide player) const
{
	if (player == LEFT_PLAYER || player == RIGHT_PLAYER)
		return mPhysicWorld->getBlobVelocity(player);
	else
		return {0.f, 0.f};
}

Vector2 DuelMatch::getBallPosition() const
{
	return mPhysicWorld->getBallPosition();
}

Vector2 DuelMatch::getBallVelocity() const
{
	return mPhysicWorld->getBallVelocity();
}

float DuelMatch::getBallRotation() const
{
	return mPhysicWorld->getBallRotation();
}

PlayerSide DuelMatch::getServingPlayer() const
{	// NO_PLAYER hack was moved into ScriptedInputSource.cpp
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
}

void DuelMatch::trigger( const MatchEvent& event )
{
	mNewEvents.push_back( event );
}

DuelMatchState DuelMatch::getState() const
{
	DuelMatchState state;
	state.worldState = mPhysicWorld->getState();
	state.logicState = mLogic->getState();
	state.playerInput[LEFT_PLAYER] = mTransformedInput[LEFT_PLAYER];
	state.playerInput[RIGHT_PLAYER] = mTransformedInput[RIGHT_PLAYER];


	return state;
}

void DuelMatch::setServingPlayer(PlayerSide side)
{
	mLogic->setServingPlayer( side );
	resetBall( side );
	mLogic->onServe( );
}

const std::string& DuelMatch::getTimeString() const
{
	return mLogic->getClock().getTimeString();
}

void DuelMatch::setMatchTimeMs(int milliseconds)
{
	mLogic->getClock().setTime( Clock::milliseconds(milliseconds) );
}

std::shared_ptr<InputSource> DuelMatch::getInputSource(PlayerSide player) const
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

void DuelMatch::updateEvents()
{
	/// \todo more economical with a swap?
	mLastEvents = mNewEvents;
	mNewEvents.clear();
}
