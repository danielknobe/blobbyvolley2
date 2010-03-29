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

#include <cassert>

#include "GameLogic.h"

// how many steps must pass until the next hit can happen
const int SQUISH_TOLERANCE = 10;


CGameLogic::CGameLogic():	mLastError(NO_PLAYER), 
							mServingPlayer(NO_PLAYER), 
							mWinningPlayer(NO_PLAYER),
							mScoreToWin(15)
{
	reset();
}

void CGameLogic::setScoreToWin(int stw)
{
	if(stw > 0)
		mScoreToWin = stw;
}

void CGameLogic::onBallHitsGround(PlayerSide side)
{
	onError(side);
}

bool CGameLogic::isCollisionValid(PlayerSide side) const{
	// check whether the ball is squished
	return mSquish[side2index(side)] < 0;
}

void CGameLogic::onBallHitsPlayer(PlayerSide side)
{
	if(!isCollisionValid(side))
		return;
	
	// otherwise, set the squish value
	mSquish[side2index(side)] = SQUISH_TOLERANCE;
	
	// count the touches
	mTouches[side2index(other_side(side))] = 0;
	if(++(mTouches[side2index(side)]) > 3)
		// if a player hits a forth time, it is an error
		onError(side);
}

void CGameLogic::step()
{
	--mSquish[0];
	--mSquish[1];
}

int CGameLogic::getScore(PlayerSide side) const
{
	return mScores[side2index(side)];
}

void CGameLogic::setScore(PlayerSide side, int score)
{
	mScores[side2index(side)] = score;
}

int CGameLogic::getHits(PlayerSide side) const
{
	return mTouches[side2index(side)];
}

PlayerSide CGameLogic::getServingPlayer() const
{
	return mServingPlayer;
}

PlayerSide CGameLogic::getWinningPlayer() const
{
	return mWinningPlayer;
}

PlayerSide CGameLogic::getLastErrorSide()
{
	PlayerSide t = mLastError;
	mLastError = NO_PLAYER;
	return t;
}

void CGameLogic::score(PlayerSide side)
{
	++mScores[side2index(side)];
	mTouches[0] = 0;
	mTouches[1] = 0;
	mWinningPlayer = checkWin();
}

PlayerSide CGameLogic::checkWin() const
{
	if(mScores[LEFT_PLAYER] >= mScoreToWin && mScores[LEFT_PLAYER] >= mScores[RIGHT_PLAYER] + 2)
		return LEFT_PLAYER;
	
	if(mScores[RIGHT_PLAYER] >= mScoreToWin && mScores[RIGHT_PLAYER] >= mScores[LEFT_PLAYER] + 2)
		return RIGHT_PLAYER;
	
	return NO_PLAYER;
}

void CGameLogic::reset()
{
	mScores[0] = 0;
	mScores[1] = 0;
	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
}

void CGameLogic::onError(PlayerSide side)
{
	mLastError = side;
	
	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
	
	onOppError(other_side(side));
	mServingPlayer = other_side(side);
}

GameLogic createGameLogic(RuleVersion rv)
{
	class GLImplOR:public CGameLogic
	{
		private:
			virtual void onOppError(PlayerSide side)
			{
				if(getServingPlayer() == side)
					score(side);
			}
			
	};
	
	class GLImplNR:public CGameLogic
	{
		private:
			virtual void onOppError(PlayerSide side)
			{
				score(side);
			}
			
	};
	switch(rv){
		case OLD_RULES:
			return std::auto_ptr<CGameLogic>(new GLImplOR);
		case NEW_RULES:
			return std::auto_ptr<CGameLogic>(new GLImplNR);
		default:
			assert(0);
	}
}
