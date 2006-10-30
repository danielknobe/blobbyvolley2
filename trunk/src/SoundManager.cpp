#include <iostream>
#include <cassert>
#include <physfs.h> 
#include "SoundManager.h"
#include "Global.h"

SoundManager* SoundManager::mSingleton;

Sound* SoundManager::loadSound(const std::string& filename)
{
	PHYSFS_file* fileHandle = PHYSFS_openRead(filename.c_str());
	if (!fileHandle)
		throw FileLoadException(std::string(filename));
	int fileLength = PHYSFS_fileLength(fileHandle);
	PHYSFS_uint8* fileBuffer = 
		new PHYSFS_uint8[fileLength];
	PHYSFS_read(fileHandle, fileBuffer, 1, fileLength);
	SDL_RWops* rwops = SDL_RWFromMem(fileBuffer, fileLength);

	SDL_AudioSpec newSoundSpec;
	Uint8* newSoundBuffer;
	Uint32 newSoundLength;
	if (!SDL_LoadWAV_RW(rwops , 1,
			&newSoundSpec, &newSoundBuffer, &newSoundLength))
		throw FileLoadException(filename);
	delete[] fileBuffer;
	PHYSFS_close(fileHandle);

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
	else
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
	catch (FileLoadException exception)
	{
		std::cerr << "Warning: Couldn't load "
			<< exception.filename << " !" << std::endl;
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
	SDL_PauseAudio((int)mute);
}
