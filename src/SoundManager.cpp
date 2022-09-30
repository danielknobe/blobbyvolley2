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
#include "SoundManager.h"

/* includes */
#include <iostream>
#include <cassert>

#include "FileRead.h"

/* implementation */
SoundManager* SoundManager::mSingleton = nullptr;

namespace {
	struct WavDeleter
	{
		void operator()(Uint8* ptr)
		{
			SDL_FreeWAV(ptr);
		}
	};

	using WavPtr = std::unique_ptr<Uint8, WavDeleter>;

	/// Struct that collects all data from reading a sound file.
	struct WavData {
		WavPtr Buffer;
		unsigned Length;
		SDL_AudioSpec Spec;
	};


	/// Reads in an entire WAV file and returns the audio samples in the original
	/// format.
	WavData readSoundFile(const std::string& filename)
	{
		// read the entire file into memory
		FileRead file(filename);
		int fileLength = file.length();
		auto fileBuffer = file.readRawBytes( fileLength );
		file.close();

		// prepare output variables
		SDL_AudioSpec newSoundSpec;
		Uint32 newSoundLength = 0;
		Uint8* temp = nullptr;

		// Do the actual file decoding.
		// Note: rwops is always closed by this function
		SDL_RWops* rwops = SDL_RWFromMem(fileBuffer.data(), fileLength);
		if (!SDL_LoadWAV_RW(rwops, 1, &newSoundSpec, &temp, &newSoundLength))
			BOOST_THROW_EXCEPTION(FileLoadException(filename));

		return {WavPtr(temp), newSoundLength, newSoundSpec};
	}

	/// Clips the value between 0 and 1, ensuring a valid volume value.
	float clip_volume(float vol) {
		return std::min(1.f, std::max(0.f, vol));
	}
}

const Uint8* Sound::getCurrentSample() const {
	return data + position;
}

int Sound::advance(int amount) {
	if(position + amount > length) {
		amount = length - position;
	}
	position += amount;
	return amount;
}

bool Sound::done() const {
	return position >= length;
}

Sound::Sound(const Uint8* data_, int length_, float volume_) :
	data(data_), length(length_), volume(clip_volume(volume_))
{
}

std::vector<Uint8> SoundManager::loadSound(const std::string& filename) const
{
	auto newSound = readSoundFile(filename);

	// check if current audio format is target format
	if (newSound.Spec.freq == mAudioSpec.freq &&
		newSound.Spec.format == mAudioSpec.format &&
		newSound.Spec.channels == mAudioSpec.channels)
	{
		std::vector<Uint8> result(newSound.Buffer.get(), newSound.Buffer.get() + newSound.Length);
		return result;
	}
	else	// otherwise, convert audio
	{
		SDL_AudioCVT conversionStructure;
		if (!SDL_BuildAudioCVT(&conversionStructure,
			newSound.Spec.format, newSound.Spec.channels, newSound.Spec.freq,
			mAudioSpec.format, mAudioSpec.channels, mAudioSpec.freq))
				BOOST_THROW_EXCEPTION ( FileLoadException(filename) );

		// allocate new sound buffer as a vector, and put the corresponding pointers into
		// the conversion structure
		std::vector<Uint8> result(newSound.Length * conversionStructure.len_mult);
		std::copy(newSound.Buffer.get(), newSound.Buffer.get() + newSound.Length, result.begin());

		conversionStructure.buf = result.data();
		conversionStructure.len = newSound.Length;

		if (SDL_ConvertAudio(&conversionStructure))
			BOOST_THROW_EXCEPTION ( FileLoadException(filename) );

		result.resize(conversionStructure.len_cvt);

		return result;
	}
}

bool SoundManager::playSound(const std::string& filename, float volume)
{
	if (!mInitialised)
		return false;

	// everything is fine, so we return true,
	// but we don't need to play the sound
	if( mMute )
		return true;
	try
	{
		auto cached_sound = mSoundCache.find(filename);
		if (cached_sound == mSoundCache.end())
		{
			auto inserted = mSoundCache.insert({filename, loadSound(filename)});
			cached_sound = inserted.first;
		}
		const auto& buffer = cached_sound->second;
		SDL_LockAudioDevice(mAudioDevice);
		mPlayingSound.emplace_back(buffer.data(), buffer.size(), volume);
		SDL_UnlockAudioDevice(mAudioDevice);
	}
	catch (const FileLoadException& exception)
	{
		std::cerr << "Warning: " << exception.what() << std::endl;
		return false;
	}
	return true;
}

bool SoundManager::init()
{
	SDL_AudioSpec desiredSpec;
	desiredSpec.freq = 44100;
	desiredSpec.format = AUDIO_S16LSB;
	desiredSpec.channels = 2;
	desiredSpec.samples = 1024;
	desiredSpec.callback = playCallback;
	desiredSpec.userdata = mSingleton;

	mAudioDevice = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, &mAudioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

	if (desiredSpec.format != mAudioSpec.format)
	{
		std::cerr << "Warning: Can't create device with desired audio spec!" << std::endl;
		std::cerr << "Reason: " << SDL_GetError() << std::endl;
	}

	if (mAudioDevice == 0)
	{
		std::cerr << "Warning: Couldn't open audio Device!" << std::endl;
		std::cerr << "Reason: " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_PauseAudioDevice(mAudioDevice, 0);
	mInitialised = true;
	mVolume = 1.0;
	return true;
}

void SoundManager::playCallback(void* sound_mgr, Uint8* stream, int length)
{
	reinterpret_cast<SoundManager*>(sound_mgr)->handleCallback(stream, length);
}

void SoundManager::handleCallback(Uint8* stream, int length) {
	SDL_memset(stream, 0, length);
	int volume = int(SDL_MIX_MAXVOLUME * mVolume);

	for (auto& sound : mPlayingSound)
	{
		auto start = sound.getCurrentSample();
		auto avail = sound.advance(length);
		SDL_MixAudioFormat(stream, start, mAudioSpec.format, avail, int(volume * sound.volume));
	}
	auto new_end = std::remove_if(begin(mPlayingSound), end(mPlayingSound), [](const Sound& sound) {
		return sound.done();
	});
	mPlayingSound.erase(new_end, mPlayingSound.end());
}


void SoundManager::deinit()
{
	mSoundCache.clear();
	SDL_UnlockAudioDevice(mAudioDevice);
	SDL_CloseAudioDevice(mAudioDevice);
	mInitialised = false;
}

SoundManager* SoundManager::createSoundManager()
{
	return new SoundManager();
}

SoundManager::SoundManager()
{
	mMute = false;
	mSingleton = this;
	mInitialised = false;
	mAudioDevice = 0;
}

SoundManager::~SoundManager()
{
	if (mInitialised)
	{
		deinit();
	}
}

SoundManager& SoundManager::getSingleton()
{
	assert(mSingleton);
	return *mSingleton;
}

void SoundManager::setVolume(float volume)
{
	mVolume = clip_volume(volume);
}

void SoundManager::setMute(bool mute)
{
	// don't do anything if mute is set.
	// this prevents the crash under xp when mute is set to false, as the second call to SDL_PauseAudio(false)
	// never returns in that case.
	if( mute == mMute )
		return;

	static bool locked = false;
	if (mute)
	{
		if (!locked)  //locking audio twice leads to a bug(??)
		{
			locked = true;
			SDL_LockAudioDevice(mAudioDevice);
		}
	}
	else
	{
		mPlayingSound.clear();
		SDL_UnlockAudioDevice(mAudioDevice);
		locked = false;
	}
	mMute = mute;
	SDL_PauseAudioDevice(mAudioDevice, (int)mute);
}
