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

#include <SDL/SDL.h>
#include <string>
#include <map>
#include <list>

/// \brief struct for holding sound data
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

/// \brief class managing game sound
class SoundManager
{
private:
	SoundManager();
	~SoundManager();

	static SoundManager* mSingleton;
	
	/// This maps filenames to sound buffers, which are always in
	/// target format
	std::map<std::string, Sound*> mSound;
	std::list<Sound> mPlayingSound;
	SDL_AudioSpec mAudioSpec;
	bool mInitialised;
	float mVolume;
	bool mMute;

	Sound* loadSound(const std::string& filename);	
	static void playCallback(void* singleton, Uint8* stream, int length);
public:
	static SoundManager* createSoundManager();
	static SoundManager& getSingleton();
	
	bool init();
	void deinit();
	bool playSound(const std::string& filename, float volume);
	void setVolume(float volume);
	void setMute(bool mute);
};
