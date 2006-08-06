#pragma once

#include <string>
#include <exception>
#include <SDL/SDL.h>


// I hope the GP2X is the only combination of these systems
#if defined(__linux__) && defined(__arm__)
#define GP2X GP2X
#endif

// This is a temporary solution until a more advanced state management
// is developed

enum GameMode
{
	MODE_NORMAL_DUEL,
	MODE_RECORDING_DUEL,
	MODE_REPLAY_DUEL,
	MODE_AI_DUEL
};

enum PlayerSide
{
	NO_PLAYER = -1,
	LEFT_PLAYER = 0,
	RIGHT_PLAYER = 1,
	MAX_PLAYERS // This is always one more than the highest player enum
			// and can be used to declare arrays
};

struct Color
{
	Color(int red, int green, int blue)
		: r(red), g(green), b(blue) {}
	Color() {}
	union
	{
		struct
		{
			Uint8 r;
			Uint8 g;
			Uint8 b;
		};
		Uint8 val[3];
	};
};

struct BufferedImage
{
	int w;
	int h;
	union
	{
		SDL_Surface* sdlImage;
		unsigned glHandle;
	};
};

struct FileLoadException : public std::exception
{
	std::string filename;
	FileLoadException(std::string name) : filename(name) {}
	~FileLoadException() throw() {}
};


struct ExtensionUnsupportedException : public std::exception
{
	std::string extension;
	ExtensionUnsupportedException(std::string name) : extension(name) {}
	~ExtensionUnsupportedException() throw() {}
};

struct ScriptException : public std::exception
{
	~ScriptException() throw() {}
};
