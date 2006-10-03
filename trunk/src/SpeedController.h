#pragma once

// This class can control the game speed and the displayed FPS.
// It is updated once a frame and waits the necessary time.
// A distinction is made between game FPS and real FPS.
// Game FPS is the number of game loop iterations per second,
// real FPS is the number of screen updates per second. The real
// FPS is reached with framedropping
// The class can report how much time is actually waited. If this value
// is close to zero, the real speed can be altered.


class SpeedController
{
public:
	SpeedController(float gameFPS, float realFPS);
	~SpeedController();
	
	void setGameSpeed(float fps);
	void setRealSpeed(float fps);

// This reports whether a framedrop is necessary to hold the real FPS
	bool doFramedrop();

//gives the caller the fps of the drawed frames:
	int getFPS() { return mFPS; }
	void setDrawFPS(bool draw) { mDrawFPS = draw; }  //help methods
	bool getDrawFPS() { return mDrawFPS; }

// This updates everything and waits the necessary time	
	void update();

	static void setMainInstance(SpeedController* inst) { mMainInstance = inst; }
	static SpeedController* getMainInstance() { return mMainInstance; }
private:
	float mGameFPS;
	float mRealFPS;
	int mFPS;
	int mFPSCounter;
	bool mFramedrop;
	bool mDrawFPS;
	static SpeedController* mMainInstance;
	int mOldTicks;

};
