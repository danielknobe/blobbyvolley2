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

#include "Vector.h"
#include <list>
#include <boost/noncopyable.hpp>

//Bleeding blobs can be a lot of fun :)

/*!	\class Blood
	\brief Container to hold the data of a single drop of blood
*/
class Blood
{
	public:
		/// \brief constructor, takes position, direction and player
		/// \param position Position this drop starts at
		/// \param direction initial velocity of the drop
		///	\param player Player this blood drop started from.
		Blood(const Vector2& position, const Vector2& direction, int player);
		
		/// this function has to be called each step
		/// it updates position and velocity.
		void step();
		
		/// gets the current position of this drop
		const Vector2& getPosition() const { return mPos; }
		
	private:
		Vector2 mPos;	///< the drops position
		Vector2 mDir;	///< the drops current velocity
		int mPlayer;	///< player who spilled this blood drop
		int mLastFrame;	///< time this drop was updated for the last time
};

/*!	\class BloodManager
	\brief Manages blood effects
	\details this class is responsible for managing blood effects, creating and deleting the particles, 
			updating their positions etc. It is designed as a singleton, so it is noncopyable.
*/
class BloodManager : private boost::noncopyable
{
	public:
		/// update function, to be called each step.
		void step();
		
		/// \brief creates a blood effect
		/// \param pos Position the effect occurs
		/// \param intensity intensity of the hit. determines the number of particles
		/// \param player player which was hit, determines the colour of the particles
		void spillBlood(Vector2 pos, float intensity, int player);
		
		/// enables or disables blood effects
		void enable(bool enable) { mEnabled = enable; }
		
		/// gets the instance of BloodManager, creating one if it does not exists
		static BloodManager& getSingleton()
		{
			if (!mSingleton)
				mSingleton = new BloodManager;
			
			return *mSingleton;
		}
		
	private:
		/// default constructor, sets mEnabled to the value
		///	set in config.xml
		BloodManager();
		
		/// helper function which returns an integer between 
		/// min and max, boundaries included
		static int random(int min, int max);
		
		/// list which contains all currently existing blood particles
		std::list<Blood> mParticles;
		
		/// true, if blood should be handled/drawn
		bool mEnabled;
		
		/// singleton
		static BloodManager* mSingleton;
};
