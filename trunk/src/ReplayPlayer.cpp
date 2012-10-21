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
#include "ReplayPlayer.h"

/* includes */
#include <cassert>

#include "IReplayLoader.h"
#include "DuelMatch.h"

/* implementation */
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
	
	mPlayerNames[LEFT_PLAYER] = loader->getPlayerName(LEFT_PLAYER);
	mPlayerNames[RIGHT_PLAYER] = loader->getPlayerName(RIGHT_PLAYER);
	
	mPosition = 0;
	mLength = loader->getLength();
}

std::string ReplayPlayer::getPlayerName(const PlayerSide side) const
{
	return mPlayerNames[side];
}

Color ReplayPlayer::getBlobColor(const PlayerSide side) const
{
	return loader->getBlobColor(side);
}

int ReplayPlayer::getGameSpeed() const 
{
	return loader->getSpeed();
}

float ReplayPlayer::getPlayProgress() const
{
	return (float)mPosition / mLength;
}

int ReplayPlayer::getReplayPosition() const
{
	return mPosition;
}

int ReplayPlayer::getReplayLength() const
{
	return mLength;
}

bool ReplayPlayer::play(DuelMatch* virtual_match)
{
	mPosition++;
	if( mPosition < mLength )
	{
		
		PlayerInput left;
		PlayerInput right;
		loader->getInputAt(mPosition, left, right);
		virtual_match->setPlayersInput(left, right);
		virtual_match->step();
		
		/*
		int point;
		if(loader->isSavePoint(mPosition, point))
		{
			ReplaySavePoint reference;
			loader->readSavePoint(point, reference);
			
			DuelMatchState current = virtual_match->getState();
			assert(reference.state == current);
		}*/
		
		
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
	
	// find next safepoint
	int save_position = -1;
	int savepoint = loader->getSavePoint(rep_position, save_position);
	// save position contains game step at which the save point is
	// savepoint is index of save point in array
	
	// now compare safepoint and actual position
	// if we have to forward and save_position is nearer than current position, jump
	if( (rep_position < mPosition || save_position > mPosition) && savepoint >= 0)
	{
		// can't use mPosition
		// set match to safepoint
		ReplaySavePoint state;
		loader->readSavePoint(savepoint, state);
		
		// set position and update match
		mPosition = save_position;
		virtual_match->setState(state.state);
	}
	// otherwise, use current position
	
	// this is legacy code which will make fast forwarding possible even 
	// when we have no safepoint and have to go back
	if(rep_position < mPosition)
	{
		// reset the match and simulate from start!
		virtual_match->reset();
		mPosition = 0;
	}
	
	// in the end, simulate the remaining steps
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
