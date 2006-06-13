#include <iostream>
#include <physfs.h>
#include "RenderManagerSDL.h"

struct FileLoadException
{
	std::string filename;
	FileLoadException(std::string name) : filename(name) {}
};

SDL_Surface* RenderManagerSDL::loadImage(std::string filename)
{
	PHYSFS_file* fileHandle = PHYSFS_openRead(filename.c_str());
	if (!fileHandle)
		throw FileLoadException(std::string(filename));
	int fileLength = PHYSFS_fileLength(fileHandle);
	PHYSFS_uint8* fileBuffer = 
		new PHYSFS_uint8[fileLength];
	PHYSFS_read(fileHandle, fileBuffer, 1, fileLength);
	SDL_RWops* rwops = SDL_RWFromMem(fileBuffer, fileLength);
	SDL_Surface* newSurface = SDL_LoadBMP_RW(rwops , 1);
	if (!newSurface)
		throw FileLoadException(filename);
	delete[] fileBuffer;
	PHYSFS_close(fileHandle);
	return newSurface;
}

SDL_Surface* RenderManagerSDL::colorSurface(SDL_Surface *surface, Color color)
{
	SDL_Surface *newSurface = SDL_CreateRGBSurface(
		SDL_HWSURFACE | SDL_SRCALPHA | SDL_SRCCOLORKEY,
		surface->w, surface->h, 32,
		0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000);
	SDL_BlitSurface(surface, 0, newSurface, 0);
	SDL_SetAlpha(newSurface, SDL_SRCALPHA, surface->format->alpha);
	SDL_SetColorKey(newSurface, SDL_SRCCOLORKEY,
			SDL_MapRGB(newSurface->format, 0, 0, 0));

	SDL_LockSurface(newSurface);
	SDL_PixelFormat* format = newSurface->format;
	for (int y = 0; y < surface->h; ++y)
	for (int x = 0; x < surface->w; ++x)
	{
		SDL_Color* pixel = &(((SDL_Color*)newSurface->pixels)
			[y * newSurface->w +x]);

		int rr = (int(pixel->r) * int(color.r)) >> 8;
		int rg = (int(pixel->g) * int(color.g)) >> 8;
		int rb = (int(pixel->b) * int(color.b)) >> 8;
		int fak = int(pixel->r) * 5 - 4 * 256 - 138;
		if (fak > 0)
		{
			rr += fak;
			rg += fak;
			rb += fak;
		}
		pixel->r = rr < 255 ? rr : 255;
		pixel->g = rg < 255 ? rg : 255;
		pixel->b = rb < 255 ? rb : 255;


	}
	SDL_UnlockSurface(newSurface);
	return newSurface;
}

RenderManagerSDL::RenderManagerSDL()
	: RenderManager()
{
}

RenderManager* RenderManager::createRenderManagerSDL()
{
	return new RenderManagerSDL();
}

void RenderManagerSDL::init(int xResolution, int yResolution, bool fullscreen)
{
	Uint32 screenFlags = SDL_HWSURFACE | SDL_HWACCEL | SDL_DOUBLEBUF;
	if (fullscreen)
		screenFlags |= SDL_FULLSCREEN;
	mScreen = SDL_SetVideoMode(xResolution, yResolution, 0, screenFlags);

	PHYSFS_addToSearchPath("data", 0);
	PHYSFS_addToSearchPath("data/gfx.zip", 1);
	
	SDL_Surface* tempBackground = loadImage("gfx/strand2.bmp");
	mBackground = SDL_DisplayFormat(tempBackground);
	SDL_FreeSurface(tempBackground);
	
	for (int i = 1; i <= 16; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/ball%02d.bmp", i);
		SDL_Surface* ballImage = loadImage(filename);
		SDL_SetColorKey(ballImage, SDL_SRCCOLORKEY, 
			SDL_MapRGB(ballImage->format, 0, 0, 0));
		SDL_Surface *convertedBallImage = SDL_DisplayFormat(ballImage);
		SDL_FreeSurface(ballImage);
		mBall.push_back(convertedBallImage);
	}
	
	SDL_Surface *tempBallShadow = loadImage("gfx/schball.bmp");
	SDL_SetColorKey(tempBallShadow, SDL_SRCCOLORKEY, 
			SDL_MapRGB(tempBallShadow->format, 0, 0, 0));
	SDL_SetAlpha(tempBallShadow, SDL_SRCALPHA, 127);
	mBallShadow = SDL_DisplayFormat(tempBallShadow);
	SDL_FreeSurface(tempBallShadow);

	for (int i = 1; i <= 5; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/blobbym%d.bmp", i);
		SDL_Surface* tempBlobImage = loadImage(filename);
		SDL_Surface* blobImage = SDL_DisplayFormat(tempBlobImage);
		SDL_FreeSurface(tempBlobImage);
		mStandardBlob.push_back(blobImage);
		mLeftBlob.push_back(colorSurface(blobImage, Color(255, 0, 0)));
		mRightBlob.push_back(colorSurface(blobImage, Color(0, 255, 0)));
		
		sprintf(filename, "gfx/sch1%d.bmp", i);
		SDL_Surface* tempBlobShadow = loadImage(filename);
		SDL_SetColorKey(tempBlobShadow, SDL_SRCCOLORKEY, 
			SDL_MapRGB(tempBlobShadow->format, 0, 0, 0));
		SDL_SetAlpha(tempBlobShadow, SDL_SRCALPHA, 127);
		SDL_Surface* blobShadow = SDL_DisplayFormat(tempBlobShadow);
		SDL_FreeSurface(tempBlobShadow);
		mStandardBlobShadow.push_back(blobShadow);
		mLeftBlobShadow.push_back(
			colorSurface(blobShadow, Color(255, 0, 0)));
		mRightBlobShadow.push_back(
			colorSurface(blobShadow, Color(0, 255, 0)));
			
	}

	for (int i = 0; i <= 50; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/font%02d.bmp", i);
		SDL_Surface *tempFont = loadImage(filename);
		SDL_SetColorKey(tempFont, SDL_SRCCOLORKEY, 
			SDL_MapRGB(tempFont->format, 0, 0, 0));
		SDL_Surface *newFont = SDL_DisplayFormat(tempFont);
		SDL_FreeSurface(tempFont);
		mFont.push_back(newFont);
	}
}

void RenderManagerSDL::deinit()
{
	SDL_FreeSurface(mBackground);
	SDL_FreeSurface(mBallShadow);
	for (int i = 0; i < mBall.size(); ++i)
		SDL_FreeSurface(mBall[i]);
	for (int i = 0; i < mStandardBlob.size(); ++i)
	{
		SDL_FreeSurface(mStandardBlob[i]);
		SDL_FreeSurface(mStandardBlobShadow[i]);
		SDL_FreeSurface(mLeftBlob[i]);
		SDL_FreeSurface(mLeftBlobShadow[i]);
		SDL_FreeSurface(mRightBlob[i]);
		SDL_FreeSurface(mRightBlobShadow[i]);
	}

	for (int i = 0; i < mFont.size(); ++i)
		SDL_FreeSurface(mFont[i]);
}

void RenderManagerSDL::draw()
{
	SDL_BlitSurface(mBackground, 0, mScreen, 0);
	
	int animationState;
	SDL_Rect position;
	
	// Ball marker
	Uint8 markerColor = SDL_GetTicks() % 1000 >= 500 ? 255 : 0;
	position.y = 5;
	position.x = lround(mBallPosition.x - 2.5);
	position.w = 5;
	position.h = 5;
	SDL_FillRect(mScreen, &position, SDL_MapRGB(mScreen->format,
			markerColor, markerColor, markerColor));
	
	// Ball Shadow
	position.x = lround(mBallPosition.x) +
		(500 - lround(mBallPosition.y)) / 4 - 48;
	position.y = 500 - (500 - lround(mBallPosition.y)) / 16 - 12;
	SDL_BlitSurface(mBallShadow, 0, mScreen, &position);
		
	// Left blob shadow
	position.x = lround(mLeftBlobPosition.x) +
		(500 - lround(mLeftBlobPosition.y)) / 4 - 48;
	position.y = 500 - (500 - lround(mLeftBlobPosition.y)) / 16 - 22;
	animationState = int(mLeftBlobAnimationState)  % 5;
	SDL_BlitSurface(mLeftBlobShadow[animationState], 0, mScreen, &position);

	// Right blob shadow
	position.x = lround(mRightBlobPosition.x) +
		(500 - lround(mRightBlobPosition.y)) / 4 - 48;
	position.y = 500 - (500 - lround(mRightBlobPosition.y)) / 16 - 22;
	animationState = int(mRightBlobAnimationState)  % 5;
	SDL_BlitSurface(mRightBlobShadow[animationState], 0,
			mScreen, &position);

	// Restore the rod
	position.x = 400 - 7;
	position.y = 300;
	SDL_Rect rodPosition;
	rodPosition.x = 400 - 7;
	rodPosition.y = 300;
	rodPosition.w = 14;
	rodPosition.h = 300;
	SDL_BlitSurface(mBackground, &rodPosition, mScreen, &position);
	
	// Drawing the Ball
	position.x = lround(mBallPosition.x) - 32;
	position.y = lround(mBallPosition.y) - 32;
	animationState = int(mBallRotation / M_PI / 2 * 16) % 16;
	SDL_BlitSurface(mBall[animationState], 0, mScreen, &position);

	// Drawing left blob
	
	position.x = lround(mLeftBlobPosition.x) - 37;
	position.y = lround(mLeftBlobPosition.y) - 44;
	animationState = int(mLeftBlobAnimationState)  % 5;
	SDL_BlitSurface(mLeftBlob[animationState], 0, mScreen, &position);
	
	// Drawing right blob
	
	position.x = lround(mRightBlobPosition.x) - 37;
	position.y = lround(mRightBlobPosition.y) - 44;
	animationState = int(mRightBlobAnimationState)  % 5;
	SDL_BlitSurface(mRightBlob[animationState], 0, mScreen, &position);

	// Drawing the score
	char textBuffer[8];
	snprintf(textBuffer, 8, mLeftPlayerWarning ? "%02d!" : "%02d",
			mLeftPlayerScore);
	drawText(textBuffer, Vector2(24, 24));
	snprintf(textBuffer, 8, mRightPlayerWarning ? "%02d!" : "%02d",
			mRightPlayerScore);	
	drawText(textBuffer, Vector2(800 - 96, 24));
}

bool RenderManagerSDL::setBackground(const std::string& filename)
{
	SDL_Surface *newBackground;
	try
	{
		SDL_Surface *tempBackground = loadImage(filename);
		mBackground = SDL_DisplayFormat(tempBackground);
		SDL_FreeSurface(tempBackground);
	}
	catch (FileLoadException)
	{
		return false;
	}
	return true;
}

void RenderManagerSDL::setBlobColor(int player, Color color)
{
	std::vector<SDL_Surface*> *handledBlob;
	std::vector<SDL_Surface*> *handledBlobShadow;

	if (player == 0)
	{
		handledBlob = &mLeftBlob;
		handledBlobShadow = &mLeftBlobShadow;
	}
	if (player == 1)
	{
		handledBlob = &mRightBlob;
		handledBlobShadow = &mRightBlobShadow;
	}

	for (int i = 0; i < 5; ++i)
	{
		SDL_FreeSurface((*handledBlob)[i]);
		SDL_FreeSurface((*handledBlobShadow)[i]);
	}

	handledBlob->clear();
	handledBlobShadow->clear();

	for (int i = 0; i < 5; ++i)
	{
		SDL_Surface *tempBlob = 
			colorSurface(mStandardBlob[i], color);
		handledBlob->push_back(
			SDL_DisplayFormat(tempBlob));
		SDL_FreeSurface(tempBlob);
		
		SDL_Surface *tempShadow = colorSurface(
				mStandardBlobShadow[i], color);
		handledBlobShadow->push_back(
			SDL_DisplayFormat(tempShadow));
		SDL_FreeSurface(tempShadow);
	}
}

void RenderManagerSDL::setBall(const Vector2& position, float rotation)
{
	mBallPosition = position;
	mBallRotation = rotation;
}

void RenderManagerSDL::setBlob(int player, 
		const Vector2& position, float animationState)
{
	if (player == 0)
	{
		mLeftBlobPosition = position;
		mLeftBlobAnimationState = animationState;
	}

	if (player == 1)
	{
		mRightBlobPosition = position;
		mRightBlobAnimationState = animationState;
	}
}

void RenderManagerSDL::setScore(int leftScore, int rightScore,
	       bool leftWarning, bool rightWarning)
{
	mLeftPlayerScore = leftScore;
	mRightPlayerScore = rightScore;
	mLeftPlayerWarning = leftWarning;
	mRightPlayerWarning = rightWarning;
}

void RenderManagerSDL::drawText(const std::string& text, Vector2 position)
{
	int length = 0;
	for (int i = 0; i < text.length(); i++)
	{
		int index = 0;
		wchar_t testChar = text[i];
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
		else if (testChar == std::string("�")[0]) // UTF-8 escape
		{
			testChar = text[++i];
//			length -= 24;
			if (testChar == std::string("�")[1])
				index = 40;
			else if (testChar == std::string("�")[1])
				index = 41;
			else if (testChar == std::string("�")[1])
				index = 42;
			else if (testChar == std::string("�")[1])
				index = 43;
		}
		else if (testChar == ' ')
		{
			length += 24;
			continue;
		}
		else index = 47;
		length += 24;
		SDL_Rect charPosition;
		charPosition.x = lround(position.x) + length - 24;
		charPosition.y = lround(position.y);
		SDL_BlitSurface(mFont[index], 0, mScreen, &charPosition);
	}
}

void RenderManagerSDL::refresh()
{
	SDL_Flip(mScreen);
}
