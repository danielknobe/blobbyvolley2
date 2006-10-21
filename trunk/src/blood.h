#pragma once

#include "Vector.h"
#include <list>

//Bleeding blobs can be a lot of fun :)

class Blood
{
	Vector2 mPos;
	Vector2 mDir;
public:
	Blood(Vector2 position, Vector2 direction);
	void step();
	Vector2 getPosition() { return mPos; }
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
	void spillBlood(Vector2 pos);
	void enable(bool enable) { mEnabled = enable; }
	static BloodManager& getSingleton()
	{
		if (!mSingleton)
			mSingleton = new BloodManager;
		return *mSingleton;
	}
};
