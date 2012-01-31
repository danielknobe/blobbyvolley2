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

/* implementation */
RenderManager* RenderManager::mSingleton = 0;

RenderManager::RenderManager()
{
	//assert(!mSingleton);
	if (mSingleton)
	{
		mSingleton->deinit();
		delete mSingleton;
	}
	mSingleton = this;
	mMouseMarkerPosition = -100.0;
	mNeedRedraw = true;
}

SDL_Surface* RenderManager::highlightSurface(SDL_Surface* surface, int luminance)
{
	SDL_Surface *newSurface = SDL_CreateRGBSurface(
		SDL_SWSURFACE | SDL_SRCALPHA | SDL_SRCCOLORKEY,
		surface->w, surface->h, 32,
		0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000);
	SDL_BlitSurface(surface, 0, newSurface, 0);
	SDL_SetAlpha(newSurface, SDL_SRCALPHA, surface->format->alpha);
	SDL_SetColorKey(newSurface, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(newSurface->format, 0, 0, 0));
	
    SDL_LockSurface(newSurface);
	for (int y = 0; y < surface->h; ++y)
	for (int x = 0; x < surface->w; ++x)
	{
		SDL_Color* pixel = &(((SDL_Color*)newSurface->pixels)
			[y * newSurface->w +x]);
		if (surface->format->colorkey != ((Uint32*)newSurface->pixels)[y * newSurface->w +x])
		{
			pixel->r = pixel->r + luminance > 255 ? 255 : pixel->r + luminance;
			pixel->g = pixel->g + luminance > 255 ? 255 : pixel->g + luminance;
			pixel->b = pixel->b + luminance > 255 ? 255 : pixel->b + luminance;
		}
	}
	SDL_UnlockSurface(newSurface);
	// no DisplayFormatAlpha, because of problems with
	// OpenGL RenderManager
	SDL_Surface *convSurface = SDL_DisplayFormat(newSurface);
	SDL_FreeSurface(newSurface);
	return convSurface;
}

SDL_Surface* RenderManager::loadSurface(std::string filename)
{
	FileRead file(filename);
	int fileLength = file.length();
	
	// just read the whole file
	boost::shared_array<char> fileContent = file.readRawBytes(fileLength);

	SDL_RWops* rwops = SDL_RWFromMem(fileContent.get(), fileLength);
	SDL_Surface* newSurface = SDL_LoadBMP_RW(rwops , 1);
	
	if (!newSurface)
		throw FileLoadException(filename);
	return newSurface;
}


SDL_Surface* RenderManager::createEmptySurface(unsigned int width, unsigned int height)
{
	SDL_Surface* newSurface = SDL_CreateRGBSurface(
		SDL_SWSURFACE | SDL_SRCALPHA | SDL_SRCCOLORKEY,
		width, height, 32,
		0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000);
	
	return newSurface;
}


int RenderManager::getNextFontIndex(std::string& string)
{
	if (string.empty())
		return -1;
	int index = 47;
	wchar_t testChar = string.at(0);
	string.erase(string.begin());
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
	else if (testChar == std::string("ß")[0]) // UTF-8 escape
	{
		testChar = string.at(0);
			string.erase(string.begin());
		if (testChar == std::string("ß")[1])
			index = 40;
		else if (testChar == std::string("ä")[1])
			index = 41;
		else if (testChar == std::string("ö")[1])
			index = 42;
		else if (testChar == std::string("ü")[1])
			index = 43;
		else if (testChar == std::string("Ä")[1])
			index = 41;
		else if (testChar == std::string("Ö")[1])
			index = 42;
		else if (testChar == std::string("Ü")[1])
			index = 43;
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
	return Vector2(
		position.x + (500.0 - position.y) / 4 + 16.0,
		500.0 - (500.0 - position.y) / 16.0 - 10.0		
	);
}

SDL_Rect RenderManager::ballShadowRect(const Vector2& position)
{
	SDL_Rect rect = {
		short(lround(position.x) - 64),
		short(lround(position.y) - 16),
		128,
		32
	};
	return rect;
}

Vector2 RenderManager::blobShadowPosition(const Vector2& position)
{
	return Vector2(
		position.x + (500.0 - position.y) / 4 + 16.0,
		500.0 - (500.0 - position.y) / 16.0 - 10.0
	);
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

void RenderManager::redraw()
{
	mNeedRedraw = true;
}

void RenderManager::drawGame(bool draw)
{
	mDrawGame = draw;
}

void RenderManager::setTitle(const std::string& title)
{
	SDL_WM_SetCaption(title.c_str(), "");
}
