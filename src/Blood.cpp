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
#include "Blood.h"

/* includes */
#include <cstdlib>

#include "RenderManager.h"
#include "IUserConfigReader.h"

/* implementation */


BloodManager* BloodManager::mSingleton = NULL;

// Blood class
// ------------

Blood::Blood(const Vector2& position, const Vector2& direction, int player):
	mPos(position),
	mDir(direction),
	mPlayer(player),
	mLastFrame(SDL_GetTicks())
{
}

void Blood::step()
{
	const float GRAVITY = 3;
	/// \todo this is the only place where we do step-rate independent calculations.
	///			is this intended behaviour???
	int diff = SDL_GetTicks() - mLastFrame;
	RenderManager::getSingleton().drawParticle(mPos, mPlayer);
	const int SPEED = 45;
	
	//this calculation is NOT based on physical rules
	mDir.y += GRAVITY / SPEED * diff;
	mPos.x += mDir.x / SPEED * diff;
	mPos.y += mDir.y / SPEED * diff;
	mLastFrame = SDL_GetTicks();
}

// BloodManager class
// -------------------


BloodManager::BloodManager()
{
	mEnabled =  IUserConfigReader::createUserConfigReader("config.xml")->getBool("blood");
}

void BloodManager::step()
{
	// don't do any processing if there are no particles
	if ( !mEnabled || mParticles.empty() )
		return;
	
	// start drawing
	RenderManager::getSingleton().startDrawParticles();
	
	// iterate over all particles
	std::list<Blood>::iterator it = mParticles.begin();
	while (it != mParticles.end())
	{
		std::list<Blood>::iterator it2 = it;
		++it;	
		it2->step();
		// delete particles below lower screen border
		if (it2->getPosition().y > 600)
			mParticles.erase(it2);
	}
	
	// finish drawing
	RenderManager::getSingleton().endDrawParticles();
}

void BloodManager::spillBlood(Vector2 pos, float intensity, int player)
{
	const double EL_X_AXIS = 30;
	const double EL_Y_AXIS = 50;
	for (int c = 0; c <= int(intensity*50); c++)
	{
		/// \todo maybe we can find a better algorithm, but for now,
		///		we just discard particles outside the ellipses
		///		so it doesn't look that much like a square.
		int x = random(int(-EL_X_AXIS * intensity), int(EL_X_AXIS * intensity));
		int y = random(int(-EL_Y_AXIS * intensity), 3);
		
		if( ( y * y / (EL_Y_AXIS * EL_Y_AXIS) + x * x / (EL_X_AXIS * EL_X_AXIS) ) > intensity * intensity)
			continue;
		
		mParticles.push_front( Blood(pos, Vector2(x, y), player) );
	}
}

int BloodManager::random(int min, int max)
{
	/// \todo is this really a good way of creating these numbers?
	return (int)((double(rand()) / RAND_MAX) * (max - min)) + min;
}
