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

/**
 * @file SoundManager.h
 * @brief Contains a class which loads and plays sound
 */

#pragma once

#include <SDL.h>
#include <string>
#include <map>
#include <vector>
#include "BlobbyDebug.h"

/// \brief struct for holding sound data
struct Sound : public ObjectCounter<Sound>
{
	Sound(const Uint8* data_, int length_, float volume_);

	const Uint8* getCurrentSample() const;
	int advance(int amount);
	bool done() const;

	const Uint8* data = nullptr;
	int length = 0;
	int position = 0;
	float volume = 1.f;
};

/*! \class SoundManager
	\brief class managing game sound.
	\details Managing loading, converting to target format, muting, setting volume
			and, of couse, playing of sounds.
*/
class SoundManager : public ObjectCounter<SoundManager>
{
	public:
		SoundManager();
		~SoundManager();

		bool init();
		void deinit();
		bool playSound(const std::string& filename, float volume);
		void setVolume(float volume);
		void setMute(bool mute);

		// Known Sounds
		static constexpr const char* IMPACT  = "sounds/bums.wav";
		static constexpr const char* WHISTLE = "sounds/pfiff.wav";
		static constexpr const char* CHAT    = "sounds/chat.wav";

	private:

		SDL_AudioDeviceID mAudioDevice;

		/// This maps filenames to sound buffers, which are always in
		/// target format
		std::map<std::string, std::vector<Uint8>> mSoundCache;
		std::vector<Sound> mPlayingSound;
		SDL_AudioSpec mAudioSpec;
		bool mInitialised;
		float mVolume;
		bool mMute;

		void handleCallback(Uint8* stream, int length);

		std::vector<Uint8> loadSound(const std::string& filename) const;
		static void playCallback(void* sound_mgr, Uint8* stream, int length);
};
