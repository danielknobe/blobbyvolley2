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

#include "ReplayPlayer.h"

#include <cassert>
#include <iostream> // debugging

#include "IReplayLoader.h"
#include "DuelMatch.h"


ReplayPlayer::ReplayPlayer()
{
}

ReplayPlayer::~ReplayPlayer()
{
}

bool ReplayPlayer::endOfFile() const
{
	return (mPosition >= mLength);
}

void ReplayPlayer::load(const std::string& filename)
{
	loader.reset(IReplayLoader::createReplayLoader(filename));
	
	mServingPlayer = loader->getServingPlayer();
	mPlayerNames[LEFT_PLAYER] = loader->getPlayerName(LEFT_PLAYER);
	mPlayerNames[RIGHT_PLAYER] = loader->getPlayerName(RIGHT_PLAYER);
	
	mPosition = 0;
	mLength = loader->getLength();
	
	// DEBUG:
	// show all replay data
	std::cout << "Speed: " << loader->getSpeed() << "\n";
	std::cout << "Duration: " << loader->getDuration() << "\n";
	std::cout << "Date: " << loader->getDate() << "\n";
}

std::string ReplayPlayer::getPlayerName(const PlayerSide side) const
{
	return mPlayerNames[side];
}

PacketType ReplayPlayer::getPacketType() const
{
	/*assert(mReplayData != 0);
	if (!endOfFile()) {
		uint8_t packet = mReplayData[mReplayOffset];
		return static_cast<PacketType>(packet >> 6);
	} else {
		return ID_ERROR;
	}*/
}

PlayerSide ReplayPlayer::getServingPlayer() const
{
	return mServingPlayer;
}

float ReplayPlayer::getPlayProgress() const
{
	return (float)mPosition / mLength;
}

int ReplayPlayer::getReplayLength() const
{
	return mLength;
}

bool ReplayPlayer::play(DuelMatch* virtual_match)
{
	mPosition++;
	if(/*getPacketType() == ID_INPUT*/ mPosition < mLength)
	{
		
		PlayerInput left;
		PlayerInput right;
		loader->getInputAt(mPosition, left, right);
		virtual_match->setPlayersInput(left, right);
		virtual_match->step();
		// everything was as expected
		return true;
	} 
	
	// error or end of file
	return false;
}

bool ReplayPlayer::gotoPlayingPosition(int rep_position, DuelMatch* virtual_match)
{
	/// \todo add validity check for rep_position
	/// \todo replay clock does not work!
	
	if(rep_position < mPosition)
	{
		// reset the match and simulate from start!
		virtual_match->reset(/*mReplayPlayer->getServingPlayer()*/);
		mPosition = 0;
	}
	
	// maximum: 100 steps
	for(int i = 0; i < 100; ++i)
	{
		// check if we have to do another step
		if(endOfFile() || rep_position == mPosition)
			return true;
		
		// do one play step
		play(virtual_match);
		
	}
	
	return false;
}
