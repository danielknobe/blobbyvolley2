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

#pragma once


#include <memory>
#include <cassert>
#include <string>

#include "Global.h"
#include "Clock.h"
#include "BlobbyDebug.h"

class IGameLogic;

/// typedef to make GameLogic an auto_ptr
/// \todo is auto_ptr the best choice here?
typedef std::auto_ptr<IGameLogic> GameLogic;

class GameLogicState;
class DuelMatch;
struct PlayerInput;

/// \class IGameLogic
/// \brief Interface for managing game rules, score counting etc.
/// \details this class is told what happens in the game and it applies the rules to count
/// the points. it is designed as a abstract base class to provide different
/// implementations (ie old/new volleyball rules)
class IGameLogic: public ObjectCounter<IGameLogic>
{
	public:
		// constuctor and destructor
		IGameLogic();
		virtual ~IGameLogic();

		virtual GameLogic clone() const = 0;
		virtual std::string getSourceFile() const = 0;

		// -----------------------------------------------------------------------------------------
		// 								Read/Write Basic Data
		// -----------------------------------------------------------------------------------------

		// methods for querying the score/touches of a patricular team
		/// returns hits count of the specified player
		int getTouches(PlayerSide side) const;

		/// returns current points of one player
		int getScore(PlayerSide side) const;

		/// sets the score of the specified player
		void setScore(PlayerSide side, int score);		// when might need such a method if we add saved games


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

		// method for getting the target score
		/// returns the score required for winning.
		int getScoreToWin() const;

		/// gets the associated clock
		Clock& getClock();

		/// transform input
		PlayerInput transformInput(PlayerInput ip, PlayerSide player);

		// -----------------------------------------------------------------------------------------
		//						Read / Write - State
		// -----------------------------------------------------------------------------------------

		GameLogicState getState() const;
		void setState(GameLogicState gls);


		// -----------------------------------------------------------------------------------------
		// 								Event - Handlers
		// -----------------------------------------------------------------------------------------

		// methods to inform the game logic what is happening in the game


		/// called when the serve begins
		void onServe();

		/// called when ball hits ground
		void onBallHitsGround(PlayerSide side);

		/// called when ball hits player
		void onBallHitsPlayer(PlayerSide side);

		/// called when ball hits wall
		void onBallHitsWall(PlayerSide side);

		/// called when ball hits net
		void onBallHitsNet(PlayerSide side);

		/// returns whether ball is valid. The timespan between making a mistake and resetting the ball,
		/// it is not considered valid, which means no collisions with blobs are calculated.
		bool isBallValid() const;
		/// returns whether game is running
		bool isGameRunning() const;
		/// returns whether the collision was valid (not squished)
		bool isCollisionValid(PlayerSide side) const;
		bool isWallCollisionValid() const;
		bool isGroundCollisionValid() const;

		// set/unset pause mode
		/// pauses the game logic.
		void onPause();
		/// disables pause mode
		void onUnPause();

		/// must be called every step
		void step();

		// script information
		virtual std::string getAuthor() const = 0;
		virtual std::string getTitle() const  = 0;

	protected:
		/// this method must be called if a team scores
		/// it increments the points of that team
		void score(PlayerSide side, int amount);

		// helper functions

		/// convert player side into array index
		static inline int side2index(PlayerSide side)
		{
			return side == NO_PLAYER ? MAX_PLAYERS : side - LEFT_PLAYER;
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

		/// this is called when a player makes a mistake
		void onError(PlayerSide errorSide, PlayerSide serveSide);



		/// thi function can change input made by a player
		virtual PlayerInput handleInput(PlayerInput ip, PlayerSide player) = 0;

		/// this function handles ball/player hits
		virtual void OnBallHitsPlayerHandler(PlayerSide side) = 0;

		/// this function handles ball/wall hits
		virtual void OnBallHitsWallHandler(PlayerSide side) = 0;

		/// this function handles ball/ground hits
		virtual void OnBallHitsGroundHandler(PlayerSide side) = 0;

		/// this function handles ball/net hits
		virtual void OnBallHitsNetHandler(PlayerSide side) = 0;


		/// this function gets called every frame
		virtual void OnGameHandler() = 0;

		/// this function checks whether a player has won the game
		virtual PlayerSide checkWin() const = 0;

		/// config parameter: score to win
		/// lua rules can change it by changing SCORE_TO_WIN variable in the global scope
		int mScoreToWin;

	private:
		// data members
		/// this array contains the scores
		int mScores[2];
		/// in this array the touches are counted
		int mTouches[2];

		/// these are helper arrays to prevent counting hits that happen too fast twice
		int mSquish[2];
		int mSquishWall;	// also for net squishes
		int mSquishGround;

		/// last side that made an error
		PlayerSide mLastError;
		/// player that is currently serving
		PlayerSide mServingPlayer;
		/// whether ball is touchable
		bool mIsBallValid;
		/// whether game is running (first ball hit was made)
		bool mIsGameRunning;

		/// player that has won the game
		/// \todo do we really need to cache this information here??
		PlayerSide mWinningPlayer;

		/// clock for determining game time
		Clock clock;
};

extern const std::string DUMMY_RULES_NAME;
extern const std::string FALLBACK_RULES_NAME;

// functions for creating a game logic object
GameLogic createGameLogic(const std::string& rulefile, DuelMatch* match);


