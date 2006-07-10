#include <iostream>
#include <cassert>
#include <physfs.h> 
#include "SoundManager.h"

struct FileLoadException
{
	std::string filename;
	FileLoadException(std::string name) : filename(name) {}
};

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


bool SoundManager::playSound(const std::string& filename, float volume)
{
	if (!mInitialised)
		return false;
	Sound* soundBuffer = mSound[filename];
	if (!soundBuffer)
	{
		try
		{
			soundBuffer = loadSound(filename);
		}
		catch (FileLoadException exception)
		{
			std::cerr << "Warning: Couldn't load "
				<< exception.filename << " !" << std::endl;
			return false;
		}
		mSound[filename] = soundBuffer;
	}
	Sound soundInstance = Sound(*soundBuffer);
	soundInstance.volume = volume;
	SDL_LockAudio();
	mPlayingSound.push_back(soundInstance);
	SDL_UnlockAudio();
	return true;
}

bool SoundManager::init()
{
	PHYSFS_addToSearchPath("data/sounds.zip", 1);
	SDL_AudioSpec desiredSpec;
	desiredSpec.freq = 44100;
	desiredSpec.format = AUDIO_S16SYS;
	desiredSpec.channels = 1;
	desiredSpec.samples = 1024;
	desiredSpec.callback = playCallback;
	desiredSpec.userdata = mSingleton;

	SDL_AudioSpec obtainedSpec;
	
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
		delete[] iter->second->data;
		delete iter->second;
	}
	SDL_CloseAudio();
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
