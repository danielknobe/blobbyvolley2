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

#include "GameLogic.h"

const int SQUISH_TOLERANCE = 10;


CGameLogic::CGameLogic():mLastError(NO_PLAYER), mServingPlayer(NO_PLAYER){
	Reset();
}

void CGameLogic::OnBallHitsGround(PlayerSide side){
	OnError(side);
}

bool CGameLogic::OnBallHitsPlayer(PlayerSide side){
	if(mSquish[side2index(side)] > 0)
		return false;
	
	mSquish[side2index(side)] = SQUISH_TOLERANCE;
	
	mTouches[side2index(other_side(side))] = 0;
	if(++(mTouches[side2index(side)]) > 3)
		OnError(side);
	
	return true;
}

void CGameLogic::OnStep(){
	--mSquish[0];
	--mSquish[1];
}

int CGameLogic::getScore(PlayerSide side) const{
	return mScores[side2index(side)];
}

int CGameLogic::getHits(PlayerSide side) const{
	return mTouches[side2index(side)];
}

PlayerSide CGameLogic::getServingPlayer() const{
	return mServingPlayer;
}

PlayerSide CGameLogic::getLastErrorSide(){
	PlayerSide t = mLastError;
	mLastError = NO_PLAYER;
	return t;
}

void CGameLogic::Score(PlayerSide side){
	++mScores[side2index(side)];
	mTouches[0] = 0;
	mTouches[1] = 0;
}

void CGameLogic::Reset(){
	mScores[0] = 0;
	mScores[1] = 0;
	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
}

void CGameLogic::OnError(PlayerSide side){
	mLastError = side;
	
	mTouches[0] = 0;
	mTouches[1] = 0;
	mSquish[0] = 0;
	mSquish[1] = 0;
	
	OnOppError(other_side(side));
	mServingPlayer = other_side(side);
}

GameLogic createGameLogic(){
	class GLImpl:public CGameLogic{
		private:
			virtual void OnOppError(PlayerSide side){
				if(getServingPlayer() == side)
					Score(side);
			}
			
	};
	return std::auto_ptr<CGameLogic>(new GLImpl);
}
