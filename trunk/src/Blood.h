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

#include "Vector.h"
#include <list>

//Bleeding blobs can be a lot of fun :)

class Blood
{
	Vector2 mPos;
	Vector2 mDir;
	int mPlayer;
public:
	Blood(Vector2 position, Vector2 direction, int player);
	void step();
	Vector2 getPosition() { return mPos; }
	int mLastFrame;
};

class BloodManager
{
	static BloodManager* mSingleton;
	std::list<Blood> mParticles;
	bool mEnabled;
	BloodManager();
	int random(int min, int max);
public:
	void step();
	void spillBlood(Vector2 pos, float intensity, int player);
	void enable(bool enable) { mEnabled = enable; }
	static BloodManager& getSingleton()
	{
		if (!mSingleton)
			mSingleton = new BloodManager;
		return *mSingleton;
	}
};
