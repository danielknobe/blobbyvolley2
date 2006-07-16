#include <iostream>
#include <physfs.h>
#include "RenderManagerGP2X.h"


SDL_Surface* RenderManagerGP2X::colorSurface(SDL_Surface *surface, Color color)
{
	SDL_Surface *newSurface = SDL_CreateRGBSurface(
		SDL_SWSURFACE | SDL_SRCALPHA | SDL_SRCCOLORKEY,
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
	SDL_Surface *convSurface = SDL_DisplayFormat(newSurface);
	SDL_FreeSurface(newSurface);
	return convSurface;
}

RenderManagerGP2X::RenderManagerGP2X()
	: RenderManager()
{
}

RenderManager* RenderManager::createRenderManagerGP2X()
{
	return new RenderManagerGP2X();
}

void RenderManagerGP2X::init(int xResolution, int yResolution, bool fullscreen)
{
	Uint32 screenFlags = SDL_HWSURFACE | SDL_HWACCEL | SDL_DOUBLEBUF;
//	screenFlags |= SDL_FULLSCREEN;
	mScreen = SDL_SetVideoMode(320, 240, 0, screenFlags);
	SDL_ShowCursor(0);

	PHYSFS_addToSearchPath("data", 0);
	PHYSFS_addToSearchPath("data/gf2x.zip", 1);
	
	SDL_Surface* tempBackground = loadSurface("gf2x/strand2.bmp");
	mBackground = SDL_DisplayFormat(tempBackground);
	SDL_FreeSurface(tempBackground);
	
	for (int i = 1; i <= 16; ++i)
	{
		char filename[64];
		sprintf(filename, "gf2x/ball%02d.bmp", i);
		SDL_Surface* ballImage = loadSurface(filename);
		SDL_SetColorKey(ballImage, SDL_SRCCOLORKEY, 
			SDL_MapRGB(ballImage->format, 0, 0, 0));
		SDL_Surface *convertedBallImage = SDL_DisplayFormat(ballImage);
		SDL_FreeSurface(ballImage);
		mBall.push_back(convertedBallImage);
	}
	
	SDL_Surface *tempBallShadow = loadSurface("gf2x/schball.bmp");
	SDL_SetColorKey(tempBallShadow, SDL_SRCCOLORKEY, 
			SDL_MapRGB(tempBallShadow->format, 0, 0, 0));
	SDL_SetAlpha(tempBallShadow, SDL_SRCALPHA, 127);
	mBallShadow = SDL_DisplayFormat(tempBallShadow);
	SDL_FreeSurface(tempBallShadow);

	for (int i = 1; i <= 5; ++i)
	{
		char filename[64];
		sprintf(filename, "gf2x/blobbym%d.bmp", i);
		SDL_Surface* blobImage = loadSurface(filename);
		mStandardBlob.push_back(blobImage);
		mLeftBlob.push_back(colorSurface(blobImage, Color(255, 0, 0)));
		mRightBlob.push_back(colorSurface(blobImage, Color(0, 255, 0)));
		
		sprintf(filename, "gf2x/sch1%d.bmp", i);
		SDL_Surface* blobShadow = loadSurface(filename);
		SDL_SetColorKey(blobShadow, SDL_SRCCOLORKEY, 
			SDL_MapRGB(blobShadow->format, 0, 0, 0));
		SDL_SetAlpha(blobShadow, SDL_SRCALPHA, 127);
		mStandardBlobShadow.push_back(blobShadow);
		mLeftBlobShadow.push_back(
			colorSurface(blobShadow, Color(255, 0, 0)));
		mRightBlobShadow.push_back(
			colorSurface(blobShadow, Color(0, 255, 0)));
			
	}

	for (int i = 0; i <= 51; ++i)
	{
		char filename[64];
		sprintf(filename, "gf2x/font%02d.bmp", i);
		SDL_Surface *tempFont = loadSurface(filename);
		SDL_SetColorKey(tempFont, SDL_SRCCOLORKEY, 
			SDL_MapRGB(tempFont->format, 0, 0, 0));
		SDL_Surface *newFont = SDL_DisplayFormat(tempFont);
		SDL_FreeSurface(tempFont);
		mFont.push_back(newFont);
		mHighlightFont.push_back(highlightSurface(newFont, 60));
	}
}

void RenderManagerGP2X::deinit()
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

void RenderManagerGP2X::draw()
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
		(200 - lround(mBallPosition.y)) / 4 - 19;
	position.y = 200 - (200 - lround(mBallPosition.y)) / 16 - 5;
	SDL_BlitSurface(mBallShadow, 0, mScreen, &position);
		
	// Left blob shadow
	position.x = lround(mLeftBlobPosition.x) +
		(200 - lround(mLeftBlobPosition.y)) / 4 - 19;
	position.y = 200 - (200 - lround(mLeftBlobPosition.y)) / 16 - 10;
	animationState = int(mLeftBlobAnimationState)  % 5;
	SDL_BlitSurface(mLeftBlobShadow[animationState], 0, mScreen, &position);

	// Right blob shadow
	position.x = lround(mRightBlobPosition.x) +
		(200 - lround(mRightBlobPosition.y)) / 4 - 19;
	position.y = 200 - (200 - lround(mRightBlobPosition.y)) / 16 - 10;
	animationState = int(mRightBlobAnimationState)  % 5;
	SDL_BlitSurface(mRightBlobShadow[animationState], 0,
			mScreen, &position);

	// Restore the rod
	position.x = 160 - 3;
	position.y = 120;
	SDL_Rect rodPosition;
	rodPosition.x = 160 - 3;
	rodPosition.y = 120;
	rodPosition.w = 7;
	rodPosition.h = 120;
	SDL_BlitSurface(mBackground, &rodPosition, mScreen, &position);
	
	// Drawing the Ball
	position.x = lround(mBallPosition.x) - 13;
	position.y = lround(mBallPosition.y) - 13;
	animationState = int(mBallRotation / M_PI / 2 * 16) % 16;
	SDL_BlitSurface(mBall[animationState], 0, mScreen, &position);

	// Drawing left blob
	
	position.x = lround(mLeftBlobPosition.x) - 15;
	position.y = lround(mLeftBlobPosition.y) - 18;
	animationState = int(mLeftBlobAnimationState)  % 5;
	SDL_BlitSurface(mLeftBlob[animationState], 0, mScreen, &position);
	
	// Drawing right blob
	
	position.x = lround(mRightBlobPosition.x) - 15;
	position.y = lround(mRightBlobPosition.y) - 18;
	animationState = int(mRightBlobAnimationState)  % 5;
	SDL_BlitSurface(mRightBlob[animationState], 0, mScreen, &position);

	// Drawing the score
	char textBuffer[8];
	snprintf(textBuffer, 8, mLeftPlayerWarning ? "%02d!" : "%02d",
			mLeftPlayerScore);
	drawText(textBuffer, Vector2(20, 10), false);
	snprintf(textBuffer, 8, mRightPlayerWarning ? "%02d!" : "%02d",
			mRightPlayerScore);	
	drawText(textBuffer, Vector2(320 - 65, 10), false);
}

bool RenderManagerGP2X::setBackground(const std::string& filename)
{
	SDL_Surface *newBackground;
	try
	{
		SDL_Surface *tempBackground = loadSurface(filename);
		mBackground = SDL_DisplayFormat(tempBackground);
		SDL_FreeSurface(tempBackground);
	}
	catch (FileLoadException)
	{
		return false;
	}
	return true;
}

void RenderManagerGP2X::setBlobColor(int player, Color color)
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

void RenderManagerGP2X::setBall(const Vector2& position, float rotation)
{
	mBallPosition = position * 0.4;
	mBallRotation = rotation;
}

void RenderManagerGP2X::setBlob(int player, 
		const Vector2& position, float animationState)
{
	if (player == 0)
	{
		mLeftBlobPosition = position * 0.4;
		mLeftBlobAnimationState = animationState;
	}

	if (player == 1)
	{
		mRightBlobPosition = position * 0.4;
		mRightBlobAnimationState = animationState;
	}
}

void RenderManagerGP2X::setScore(int leftScore, int rightScore,
	       bool leftWarning, bool rightWarning)
{
	mLeftPlayerScore = leftScore;
	mRightPlayerScore = rightScore;
	mLeftPlayerWarning = leftWarning;
	mRightPlayerWarning = rightWarning;
}

void RenderManagerGP2X::drawText(const std::string& text, Vector2 position, bool highlight)
{
	int length = 0;
	std::string string = text;
	int index = getNextFontIndex(string);
	while (index != -1)
	{
		length += 24;
		SDL_Rect charPosition;
		charPosition.x = lround(position.x) + length - 24;
		charPosition.y = lround(position.y);
		if (highlight)
			SDL_BlitSurface(mHighlightFont[index], 0, mScreen, &charPosition);
		else
			SDL_BlitSurface(mFont[index], 0, mScreen, &charPosition);
		index = getNextFontIndex(string);
	}
}

void RenderManagerGP2X::refresh()
{
	SDL_Flip(mScreen);
}
