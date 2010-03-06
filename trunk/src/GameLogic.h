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


#include <memory>


#include "Global.h"


// this class is told what happens in the game and it applies the rules to count 
// the points. it is designed as a abstract base class to provide different 
// implementations (ie old/new volleyball rules)
class CGameLogic{
	public:
		// methods to inform the game logic what is happening in the game
		void OnBallHitsGround(PlayerSide side);
				
		// returns whether the collision was valid
		bool OnBallHitsPlayer(PlayerSide side);
		
		void OnStep();
		
		
		// method for querying the score/touches of a patricular team
		int getScore(PlayerSide side) const;
		int getHits(PlayerSide side) const;
		
		// method for querying the serving player
		PlayerSide getServingPlayer() const;
		
		// returns who has made the last mistake
		// after this request, the value is reset
		PlayerSide getLastErrorSide();
		
		virtual ~CGameLogic(){
		};
		
	protected:
		// this method must be called if a team scores
		// is increments the points of that team
		void Score(PlayerSide side);
		
		CGameLogic();
	
	private:
		int mScores[RIGHT_PLAYER - LEFT_PLAYER + 1];
		int mTouches[RIGHT_PLAYER - LEFT_PLAYER + 1];
		int mSquish[RIGHT_PLAYER - LEFT_PLAYER + 1];
		
		PlayerSide mServingPlayer;
		
		PlayerSide mLastError;
		
		// helper funktion to convert player side into array index
		inline int side2index(PlayerSide side) const{
			assert(side - LEFT_PLAYER >= 0);
			assert(side - LEFT_PLAYER < sizeof(mScores));
			return side - LEFT_PLAYER;
		}
		
		// helper function to determine the opposit player side
		inline PlayerSide other_side(PlayerSide side) const{
			switch(side){
				case LEFT_PLAYER:
					return RIGHT_PLAYER;
				case RIGHT_PLAYER:
					return LEFT_PLAYER;
				default:
					assert(0);
			}
		}
		
		// resets score and touches
		void Reset();
		
		// this is called when a player makes a
		// mistake
		void OnError(PlayerSide side);
		
		// this is called when a player makes a mistake
		// so the other player has won the rally. this 
		// method is to be implemented to perform scoring
		virtual void OnOppError(PlayerSide side) = 0;
};

// typedef to make GameLogic an auto_ptr
typedef std::auto_ptr<CGameLogic> GameLogic;

// function for creating a game logic object
GameLogic createGameLogic();

