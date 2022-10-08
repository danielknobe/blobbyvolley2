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
#include <algorithm>

#include "RenderManager.h"

/* implementation */

// Blood class
// ------------

Blood::Blood(const Vector2& position, const Vector2& direction, int player):
	mPos(position),
	mDir(direction),
	mPlayer(player),
	mLastFrame(SDL_GetTicks())
{
}

void Blood::step(RenderManager& renderer)
{
	const float GRAVITY = 3;
	/// \todo this is the only place where we do step-rate independent calculations.
	///			is this intended behaviour???
	int diff = SDL_GetTicks() - mLastFrame;
	renderer.drawParticle(mPos, mPlayer);
	const int SPEED = 45;
	
	//this calculation is NOT based on physical rules
	mDir.y += GRAVITY / SPEED * diff;
	mPos.x += mDir.x / SPEED * diff;
	mPos.y += mDir.y / SPEED * diff;
	mLastFrame = SDL_GetTicks();
}

// BloodManager class
// -------------------


BloodManager::BloodManager(bool enabled) : mEnabled(enabled)
{
}

void BloodManager::step(RenderManager& renderer)
{
	// don't do any processing if there are no particles
	if ( !mEnabled || mParticles.empty() )
		return;
	
	// start drawing
	renderer.startDrawParticles();
	
	// iterate over all particles
	for(auto& pt : mParticles) {
		pt.step(renderer);
	}
	
	// finish drawing
	renderer.endDrawParticles();

	// delete old particles
	mParticles.erase(std::remove_if(begin(mParticles), end(mParticles), [](const Blood& particle) {
		return particle.getPosition().y > 600;
	}), mParticles.end());
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
		
		mParticles.emplace_back(pos, Vector2(x, y), player );
	}
}

int BloodManager::random(int min, int max)
{
	std::uniform_int_distribution<int> dist(min, max);
	return dist(mRng);
}
