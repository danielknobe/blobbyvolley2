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
#include "RenderManager.h"

/* includes */
#include "FileRead.h"
#include "Blood.h"
#include "IUserConfigReader.h"

/* implementation */
#define INVALID_FONT_INDEX -1

RenderManager* RenderManager::mSingleton = nullptr;

RenderManager::~RenderManager() = default;

RenderManager::RenderManager() :
	mBloodMgr(new BloodManager(IUserConfigReader::createUserConfigReader("config.xml")->getBool("blood")))
{
	//assert(!mSingleton);
	if (mSingleton)
	{
		mSingleton->deinit();
		delete mSingleton;
	}

	mSingleton = this;
	mMouseMarkerPosition = -100.0;
}

BloodManager& RenderManager::getBlood() {
	return *mBloodMgr;
}

SDL_Surface* RenderManager::highlightSurface(SDL_Surface* surface, int luminance)
{
	SDL_Surface *newSurface = createEmptySurface(surface->w, surface->h);
	SDL_SetColorKey(surface, SDL_FALSE, 0);
	SDL_BlitSurface(surface, nullptr, newSurface, nullptr);
	SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(newSurface->format, 0, 0, 0));

	Uint8 alpha;
	SDL_GetSurfaceAlphaMod(surface, &alpha);
	SDL_SetSurfaceAlphaMod(newSurface, alpha);
	SDL_SetColorKey(newSurface, SDL_TRUE, SDL_MapRGB(newSurface->format, 0, 0, 0));

	SDL_LockSurface(newSurface);
	for (int y = 0; y < surface->h; ++y)
	{
		for (int x = 0; x < surface->w; ++x)
		{
			// this seems overly complicated:
			// aren't we just calculating newSurface->pixels + (y * newSurface->w + x)
			SDL_Color* pixel = &( ((SDL_Color*)newSurface->pixels)[y * newSurface->w + x] );
			Uint32 colorKey;
			SDL_GetColorKey(surface, &colorKey);
			// we index newSurface->pixels once as Uint32[] and once as SDL_Color[], so these must have the same size
			static_assert( sizeof(Uint32) == sizeof(SDL_Color), "Uint32 must have the same size as SDL_Color" );

			if (colorKey != ((Uint32*)newSurface->pixels)[y * newSurface->w +x])
			{
				pixel->r = pixel->r + luminance > 255 ? 255 : pixel->r + luminance;
				pixel->g = pixel->g + luminance > 255 ? 255 : pixel->g + luminance;
				pixel->b = pixel->b + luminance > 255 ? 255 : pixel->b + luminance;
			}
		}
	}
	SDL_UnlockSurface(newSurface);
	// no DisplayFormatAlpha, because of problems with
	// OpenGL RenderManager
	return newSurface;
}

SDL_Surface* RenderManager::loadSurface(const std::string& filename)
{
	FileRead file(filename);
	int fileLength = file.length();

	// just read the whole file
	auto fileContent = file.readRawBytes(fileLength);

	SDL_RWops* rwops = SDL_RWFromMem(fileContent.data(), fileLength);
	SDL_Surface* newSurface = SDL_LoadBMP_RW(rwops , 1);

	if (!newSurface)
		BOOST_THROW_EXCEPTION ( FileLoadException(filename) );

	return newSurface;
}


SDL_Surface* RenderManager::createEmptySurface(unsigned int width, unsigned int height)
{
	SDL_Surface* newSurface = SDL_CreateRGBSurface(0, width, height, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000);

	return newSurface;
}

int RenderManager::getNextFontIndex(std::string::const_iterator& iter)
{
	int index = INVALID_FONT_INDEX;
	wchar_t testChar = *iter;
	++iter;
	if (testChar >= '0' && testChar <= '9')
		index = testChar - '0';
	else if (testChar >= 'a' && testChar <= 'z')
		index = testChar - 'a' + 10;
	else if (testChar >= 'A' && testChar <= 'Z')
		index = testChar - 'A' + 10;
	else if (testChar == '.')
		index = 36;
	else if (testChar == '!')
		index = 37;
	else if (testChar == '(')
		index = 38;
	else if (testChar == ')')
		index = 39;
	else if (testChar == '\'')
		index = 44;
	else if (testChar == ':')
		index = 45;
	else if (testChar == ';')
		index = 46;
	else if (testChar == '?')
		index = 47;
	else if (testChar == ',')
		index = 48;
	else if (testChar == '/')
		index = 49;
	else if (testChar == '_')
		index = 50;
	else if (testChar == ' ')
		index = 51;
	else if (testChar == '-')
		index = 52;
	else if (testChar == '%')
		index = 53;
	else if (testChar == '+')
		index = 54;
	else if (testChar == std::string("¿")[0]) // UTF-8 escape (c2xx)
	{
		testChar = *iter;
		++iter;
		if (testChar == std::string("¿")[1])
			index = 56;
	}
	else if (testChar == std::string("👁")[0]) // UTF-8 escape (f0xxxxxx)
	{
		testChar = *iter;
		++iter;
		if (testChar == std::string("👁")[1]) // UTF-8 escape (xx9fxxxx)
		{
			testChar = *iter;
			++iter;
			if (testChar == std::string("👁")[2]) // UTF-8 escape (xxxx91xx)
			{
				testChar = *iter;
				++iter;
				if (testChar == std::string("👁")[3]) // UTF-8 escape (xxxxxx81)
				{
					index = 57;
				}
			}
			else if (testChar == std::string("🔒")[2]) // UTF-8 escape (xxxx94xx)
			{
				testChar = *iter;
				++iter;
				if (testChar == std::string("🔒")[3]) // UTF-8 escape (xxxxxx92)
				{
					index = 58;
				}
			}
		}
	}
	else if (testChar == std::string("ß")[0]) // UTF-8 escape (c3xx)
	{
		testChar = *iter;
		++iter;
		if (testChar == std::string("ß")[1])
			index = 40;
		else if (testChar == std::string("ä")[1])
			index = 41;
		else if (testChar == std::string("ö")[1])
			index = 42;
		else if (testChar == std::string("ü")[1])
			index = 43;
		else if (testChar == std::string("ì")[1])
			index = 55;
		else if (testChar == std::string("Ä")[1])
			index = 41;
		else if (testChar == std::string("Ö")[1])
			index = 42;
		else if (testChar == std::string("Ü")[1])
			index = 43;
		else if (testChar == std::string("Ì")[1])
			index = 55;
		else if ((testChar >= std::string("à")[1]) && (testChar <= std::string("å")[1]))
			index = 10; // Map to A
		else if ((testChar >= std::string("À")[1]) && (testChar <= std::string("Å")[1]))
			index = 10; // Map to A
		else if ((testChar >= std::string("è")[1]) && (testChar <= std::string("ë")[1]))
			index = 14; // Map to E
		else if ((testChar >= std::string("ò")[1]) && (testChar <= std::string("õ")[1]))
			index = 24; // Map to O
		else if ((testChar >= std::string("Ò")[1]) && (testChar <= std::string("Õ")[1]))
			index = 24; // Map to O
		else if (testChar == std::string("ý")[1])
			index = 34; // Map to Y
		else if ((testChar >= std::string("ì")[1]) && (testChar <= std::string("ï")[1]))
			index = 18; // Map to I
		else if (testChar == std::string("ñ")[1])
			index = 23; // Map to N
		else if (testChar == std::string("Ñ")[1])
			index = 23; // Map to N
	}
	else if (testChar == std::string("Ć")[0]) // UTF-8 escape (c4xx)
	{
		testChar = *iter;
		++iter;
		if ((testChar >= std::string("Ć")[1]) && (testChar <= std::string("č")[1]))
			index = 12; // Map to C
		else if ((testChar >= std::string("Ē")[1]) && (testChar <= std::string("ě")[1]))
			index = 14; // Map to E
	}
	else if (testChar == std::string("Ś")[0]) // UTF-8 escape (c5xx)
	{
		testChar = *iter;
		++iter;
		if ((testChar >= std::string("Ŕ")[1]) && (testChar <= std::string("ř")[1]))
			index = 27; // Map to R
		else if ((testChar >= std::string("Ś")[1]) && (testChar <= std::string("š")[1]))
			index = 28; // Map to S
		else if ((testChar >= std::string("Ũ")[1]) && (testChar <= std::string("ų")[1]))
			index = 30; // Map to U
		else if ((testChar >= std::string("Ŷ")[1]) && (testChar <= std::string("Ÿ")[1]))
			index = 34; // Map to Y
		else if ((testChar >= std::string("Ź")[1]) && (testChar <= std::string("ž")[1]))
			index = 35; // Map to Z
	}

	if (index == INVALID_FONT_INDEX)
	{
#ifdef DEBUG
		std::cerr << "Warning: Missing font for character code " << testChar << std::endl;
#endif
		index = 47;
	}

	return index;
}

void RenderManager::setMouseMarker(float position)
{
	mMouseMarkerPosition = position;
}

SDL_Rect RenderManager::blobRect(const Vector2& position)
{
	SDL_Rect rect = {
		(short)(lround(position.x) - 37),
		(short)(lround(position.y) - 44),
		75,
		89
	};

	return rect;
}

SDL_Rect RenderManager::ballRect(const Vector2& position)
{
	SDL_Rect rect = {
		(short)(lround(position.x) - 32),
		(short)(lround(position.y) - 32),
		64,
		64
	};
	return rect;
}

Vector2 RenderManager::ballShadowPosition(const Vector2& position)
{
	return {
		position.x + (500.f - position.y) / 4.f + 16.f,
		500.f - (500.f - position.y) / 16.f - 10.f
	};
}

SDL_Rect RenderManager::ballShadowRect(const Vector2& position)
{
	SDL_Rect rect = {
		short(lround(position.x) - 64),
		short(lround(position.y) - 16),
		69,
		17
	};

	return rect;
}

Vector2 RenderManager::blobShadowPosition(const Vector2& position)
{
	return {
		position.x + (500.f - position.y) / 4 + 16.f,
		500.f - (500.f - position.y) / 16.f - 10.f
	};
}

SDL_Rect RenderManager::blobShadowRect(const Vector2& position)
{
	SDL_Rect rect = {
		short(lround(position.x) - 64),
		short(lround(position.y) - 16),
		128,
		32
	};
	return rect;
}

void RenderManager::setTitle(const std::string& title)
{
	SDL_SetWindowTitle(mWindow, title.c_str());
}

SDL_Window* RenderManager::getWindow()
{
	return mWindow;
}

Color RenderManager::getOscillationColor() const
{
	float time = float(SDL_GetTicks()) / 1000.f;

	return {
		int((std::sin(time*1.5) + 1.0) * 128),
		int((std::sin(time*2.5) + 1.0) * 128),
		int((std::sin(time*3.5) + 1.0) * 128)
	};
}
