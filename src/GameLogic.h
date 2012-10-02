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
#include <cassert>

#include "Global.h"
#include "Clock.h"

/// \class IGameLogic
/// \brief Interface for managing game rules, score counting etc.
/// \details this class is told what happens in the game and it applies the rules to count 
/// the points. it is designed as a abstract base class to provide different 
/// implementations (ie old/new volleyball rules)
class IGameLogic
{
	public:
		// constuctor and destructor
		IGameLogic();
		virtual ~IGameLogic();
		
		// -----------------------------------------------------------------------------------------
		// 								Read/Write Basic Data
		// -----------------------------------------------------------------------------------------
		
		// methods for querying the score/touches of a patricular team
		/// returns current points of one player
		int getScore(PlayerSide side) const;
		
		/// sets the score of the specified player
		void setScore(PlayerSide side, int score);		// when might need such a method if we add saved games
		
		
		/// returns the number of times a player has hit the ball.
		int getHits(PlayerSide side) const;
		
		// method for querying and setting the serving player
		/// returns which player is the serving player
		PlayerSide getServingPlayer() const;
		/// sets which player is currently the serving player
		void setServingPlayer(PlayerSide side);
		
				
		/// returns the winning player or NO_PLAYER if the
		/// game still runs
		PlayerSide getWinningPlayer() const;
		
		/// \brief returns which player made the last mistake.
		/// After this request, that value is reset.
		PlayerSide getLastErrorSide();
		
		// methods for setting/getting the target score
		/// sets the score required for winning.
		void setScoreToWin(int stw);
		/// returns the score required for winning.
		int getScoreToWin() const;
		
		/// gets the associated clock
		Clock& getClock();
		
		// -----------------------------------------------------------------------------------------
		// 								Event - Handlers
		// -----------------------------------------------------------------------------------------
	
		// methods to inform the game logic what is happening in the game
		
		
		/// called when ball hits ground
		void onBallHitsGround(PlayerSide side);
		
		/// called when ball hits player
		void onBallHitsPlayer(PlayerSide side);
				
		/// returns whether the collision was valid (max. 3 hits)
		bool isCollisionValid(PlayerSide side) const;

		
		// set/unset pause mode
		/// pauses the game logic. 
		void onPause();
		/// disables pause mode
		void onUnPause();
		
		/// must be called every step
		void step();		

	protected:
		/// this method must be called if a team scores
		/// is increments the points of that team
		void score(PlayerSide side);

		// helper functions
		
		/// convert player side into array index
		static inline int side2index(PlayerSide side)
		{
			assert(side == LEFT_PLAYER || side == RIGHT_PLAYER);
			return side - LEFT_PLAYER;
		}
		
		/// determine the opposite player side
		static inline PlayerSide other_side(PlayerSide side)
		{
			switch(side)
			{
				case LEFT_PLAYER:
					return RIGHT_PLAYER;
				case RIGHT_PLAYER:
					return LEFT_PLAYER;
				default:
					assert(0);
			}
		}

	private:	
		/// resets score and touches
		void reset();
		
		/// this is called when a player makes a mistake
		void onError(PlayerSide side);
		
		/// this function is called by onError, it contains the customizable part of the 
		/// error handling
		virtual void OnMistake(PlayerSide side) = 0;
		
		/// this function handles ball/player hits
		virtual bool OnBallHitsPlayerHandler(PlayerSide ply, int numOfHits) = 0;
		
		
		/// this function checks whether a player has won the game
		virtual PlayerSide checkWin() const = 0;
			
		
		// data memberss
		/// this array contains the scores
		int mScores[2];
		/// in this array the number of touches are counted
		int mTouches[2];
		/// this is an helper array to prevent counting hits that happen too fast twice
		int mSquish[2];
		
		/// last side that made an error
		PlayerSide mLastError;
		/// player that is currently serving
		PlayerSide mServingPlayer;
		
		/// player that has won the game
		/// \todo do we really need to cache this information here??
		PlayerSide mWinningPlayer;
		
		/// config parameter: score to win
		/// \todo how do we use config parameters with lua rules?
		int mScoreToWin;
		
		/// clock for determining game tome
		Clock clock;
};

/// typedef to make GameLogic an auto_ptr
/// \todo is auto_ptr the best choice here?
typedef std::auto_ptr<IGameLogic> GameLogic;

// function for creating a game logic object
GameLogic createGameLogic(const std::string& rulefile);


