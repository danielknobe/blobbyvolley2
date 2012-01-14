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

#include <iostream>
#include <cassert>
#include "FileRead.h"
#include "SoundManager.h"
#include "Global.h"

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
		throw FileLoadException(filename);


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
				throw FileLoadException(filename);
		conversionStructure.buf = 
			new Uint8[newSoundLength * conversionStructure.len_mult];
		memcpy(conversionStructure.buf, newSoundBuffer, newSoundLength);
		conversionStructure.len = newSoundLength;

		if (SDL_ConvertAudio(&conversionStructure))
			throw FileLoadException(filename);
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
		SDL_LockAudio();
		mPlayingSound.push_back(soundInstance);
		SDL_UnlockAudio();
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

	
	if (SDL_OpenAudio(&desiredSpec, &mAudioSpec))
	{
		std::cerr << "Warning: Couldn't open audio Device!"
			<< std::endl;
		std::cerr << "Reason: " << SDL_GetError() << std::endl;
		return false;
	}
	SDL_PauseAudio(0);
	mInitialised = true;
	mVolume = 1.0;
	return true;
}

void SoundManager::playCallback(void* singleton, Uint8* stream, int length)
{	
	std::list<Sound>& playingSound = 
		((SoundManager*)singleton)->mPlayingSound;
	int volume = int(SDL_MIX_MAXVOLUME * 
		((SoundManager*)singleton)->mVolume);
	for (std::list<Sound>::iterator iter = playingSound.begin();
		       iter != playingSound.end(); ++iter)
	{
		int bytesLeft = iter->length
			- iter->position;
		if (bytesLeft < length)
		{
			SDL_MixAudio(stream, iter->data + iter->position,
				bytesLeft, int(volume * iter->volume));
			std::list<Sound>::iterator eraseIter = iter;
			std::list<Sound>::iterator nextIter = ++iter;
			playingSound.erase(eraseIter);
			iter = nextIter;
			// prevents increment of past-end-interator
			if(iter == playingSound.end())
				break;
		}
		else
		{
			SDL_MixAudio(stream, iter->data + iter->position,
				length, int(volume * iter->volume));
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
	SDL_CloseAudio();
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
	static bool locked = false;
	if (mute)
	{
		if (!locked)  //locking audio twice leads to a bug(??)
		{
			locked = true;
			SDL_LockAudio();
		}
	}
	else
	{
		mPlayingSound.clear();
		SDL_UnlockAudio();
		locked = false;
	}
	mMute = mute;
	SDL_PauseAudio((int)mute);
}
