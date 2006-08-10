#pragma once

#include <SDL/SDL.h>
#include <iostream>
#include <map>
#include <list>

struct Sound
{
	Sound()
	{
		data = 0;
	}
	
	Uint8* data;
	Uint32 length;
	int position;
	float volume;
};

class SoundManager
{
private:
	SoundManager();
	~SoundManager();

	static SoundManager* mSingleton;
	
	// This maps filenames to sound buffers, which are always in
	// target format
	std::map<std::string, Sound*> mSound;
	std::list<Sound> mPlayingSound;
	SDL_AudioSpec mAudioSpec;
	bool mInitialised;
	float mVolume;

	Sound* loadSound(const std::string& filename);	
	static void playCallback(void* singleton, Uint8* stream, int length);
public:
	static SoundManager* createSoundManager();
	static SoundManager& getSingleton();
	
	bool init();
	void deinit();
	bool playSound(const std::string& filename, float volume);
	void setVolume(float volume);

};
