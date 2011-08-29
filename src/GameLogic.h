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
#include "Clock.h"

class lua_State;

// this class is told what happens in the game and it applies the rules to count 
// the points. it is designed as a abstract base class to provide different 
// implementations (ie old/new volleyball rules)
class CGameLogic
{
	public:
		// methods to inform the game logic what is happening in the game
		void onBallHitsGround(PlayerSide side);
				
		// returns whether the collision was valid
		bool isCollisionValid(PlayerSide side) const;
		void onBallHitsPlayer(PlayerSide side);
		
		// set/unset pause mode
		void onPause();
		void onUnPause();
		
		// must be called every step
		void step();		
		
		// method for querying the score/touches of a patricular team
		int getScore(PlayerSide side) const;
		int getHits(PlayerSide side) const;
		
		void setScore(PlayerSide side, int score);
		
		// method for querying the serving player
		PlayerSide getServingPlayer() const;
		void setServingPlayer(PlayerSide side);
		
		// returns the winning player or NO_PLAYER if the
		// game still runs
		PlayerSide getWinningPlayer() const;
		
		// returns who has made the last mistake
		// after this request, the value is reset
		PlayerSide getLastErrorSide();
		
		// method for setting/getting the target score
		void setScoreToWin(unsigned int stw);
		unsigned int getScoreToWin() const;
		
		// constuctor and destructor
		CGameLogic();
		
		virtual ~CGameLogic()
		{
		};
		
		Clock& getClock(){
			return clock;
		}
	
	private:	
		// this method must be called if a team scores
		// is increments the points of that team
		void score(PlayerSide side);
		
		// lua functions
		static int luaScore(lua_State* state); 
		static int luaGetOpponent(lua_State* state);
		static int luaGetServingPlayer(lua_State* state);
		
		// resets score and touches
		void reset();
		
		// this is called when a player makes a
		// mistake
		void onError(PlayerSide side);
		
		PlayerSide checkWin() const;
			
		
		// data members
		
		int mScores[RIGHT_PLAYER - LEFT_PLAYER + 1];
		int mTouches[RIGHT_PLAYER - LEFT_PLAYER + 1];
		int mSquish[RIGHT_PLAYER - LEFT_PLAYER + 1];
		
		unsigned int mScoreToWin;
		
		PlayerSide mServingPlayer;
		
		PlayerSide mLastError;
		
		PlayerSide mWinningPlayer;
		
		// clock
		Clock clock;
		
		// lua state
		lua_State* mState;
	
	private:
		// helper functions
		
		// convert player side into array index
		static inline int side2index(PlayerSide side)
		{
			assert(side == LEFT_PLAYER || side == RIGHT_PLAYER);
			return side - LEFT_PLAYER;
		}
		
		// determine the opposit player side
		static inline PlayerSide other_side(PlayerSide side)
		{
			switch(side){
				case LEFT_PLAYER:
					return RIGHT_PLAYER;
				case RIGHT_PLAYER:
					return LEFT_PLAYER;
				default:
					assert(0);
			}
		}
};

// typedef to make GameLogic an auto_ptr
typedef std::auto_ptr<CGameLogic> GameLogic;

// enumeration for rule types
enum RuleVersion{
	OLD_RULES,		// point only for serving player
	NEW_RULES		// point for each error
};

// function for creating a game logic object
GameLogic createGameLogic(RuleVersion rv);


