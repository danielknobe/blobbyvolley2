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

#include "Global.h"
#include "FileRead.h"

/* implementation */
SoundManager* SoundManager::mSingleton;

Sound* SoundManager::loadSound(const std::string& filename)
{
	FileRead file(filename);
	int fileLength = file.length();

	// safe file data into a shared_array to ensure it is deleten properly
	// in case of exceptions
	boost::shared_array<char> fileBuffer = file.readRawBytes( fileLength );

	SDL_RWops* rwops = SDL_RWFromMem(fileBuffer.get(), fileLength);

	// we don't need the file handle anymore
	file.close();

	SDL_AudioSpec newSoundSpec;
	Uint8* newSoundBuffer;
	Uint32 newSoundLength;
	if (!SDL_LoadWAV_RW(rwops , 1,
			&newSoundSpec, &newSoundBuffer, &newSoundLength))
		BOOST_THROW_EXCEPTION(FileLoadException(filename));


	// check if current audio format is target format
	if (newSoundSpec.freq == mAudioSpec.freq &&
		newSoundSpec.format == mAudioSpec.format &&
		newSoundSpec.channels == mAudioSpec.channels)
	{
		Sound *newSound = new Sound;
		newSound->data = new Uint8[newSoundLength];
		memcpy(newSound->data, newSoundBuffer, newSoundLength);
		newSound->length = newSoundLength;
		newSound->position = 0;
		SDL_FreeWAV(newSoundBuffer);
                return newSound;
	}
	else	// otherwise, convert audio
	{
		SDL_AudioCVT conversionStructure;
		if (!SDL_BuildAudioCVT(&conversionStructure,
			newSoundSpec.format, newSoundSpec.channels, newSoundSpec.freq,
			mAudioSpec.format, mAudioSpec.channels, mAudioSpec.freq))
				BOOST_THROW_EXCEPTION ( FileLoadException(filename) );
		conversionStructure.buf =
			new Uint8[newSoundLength * conversionStructure.len_mult];
		memcpy(conversionStructure.buf, newSoundBuffer, newSoundLength);
		conversionStructure.len = newSoundLength;

		if (SDL_ConvertAudio(&conversionStructure))
			BOOST_THROW_EXCEPTION ( FileLoadException(filename) );
		SDL_FreeWAV(newSoundBuffer);

		Sound *newSound = new Sound;
		newSound->data = conversionStructure.buf;
		newSound->length = Uint32(conversionStructure.len_cvt);
		newSound->position = 0;
		return newSound;
	}
}

bool SoundManager::playSound(const std::string& filename, float volume)
{
	if (!mInitialised)
		return false;

	// everything is fine, so we return true
	// but we don't need to play the sound
	if( mMute )
		return true;
	try
	{
		Sound* soundBuffer = mSound[filename];
		if (!soundBuffer)
		{
			soundBuffer = loadSound(filename);
			mSound[filename] = soundBuffer;
		}
		Sound soundInstance = Sound(*soundBuffer);
		soundInstance.volume =
			volume > 0.0 ? (volume < 1.0 ? volume : 1.0) : 0.0;
		SDL_LockAudioDevice(mAudioDevice);
		mPlayingSound.push_back(soundInstance);
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

	mAudioDevice = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &mAudioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

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

void SoundManager::playCallback(void* singleton, Uint8* stream, int length)
{
	SDL_memset(stream, 0, length);
	std::list<Sound>& playingSound = ((SoundManager*)singleton)->mPlayingSound;
	int volume = int(SDL_MIX_MAXVOLUME * ((SoundManager*)singleton)->mVolume);

	for (auto iter = playingSound.begin(); iter != playingSound.end(); ++iter)
	{
		int bytesLeft = iter->length - iter->position;
		if (bytesLeft < length)
		{
			SDL_MixAudio(stream, iter->data + iter->position, bytesLeft, int(volume * iter->volume));
			auto eraseIter = iter;
			auto nextIter = ++iter;
			playingSound.erase(eraseIter);
			iter = nextIter;
			// prevents increment of past-end-interator
			if(iter == playingSound.end())
				break;
		}
		else
		{
			SDL_MixAudioFormat(stream, iter->data + iter->position, mSingleton->mAudioSpec.format, length, int(volume * iter->volume));
			iter->position += length;
		}
	}
}

void SoundManager::deinit()
{
	for (std::map<std::string, Sound*>::iterator iter = mSound.begin();
			iter != mSound.end(); ++iter)
	{
		if (iter->second)
		{
			if (iter->second->data != 0)
			{
				delete[] iter->second->data;
			}
			delete iter->second;
		}
	}
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
	volume = volume > 0.0 ? (volume < 1.0 ? volume : 1.0) : 0.0;
	mVolume = volume;
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
