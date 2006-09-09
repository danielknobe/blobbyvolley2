#include <iostream>
#include <physfs.h>
#include "RenderManagerSDL.h"


SDL_Surface* RenderManagerSDL::colorSurface(SDL_Surface *surface, Color color)
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
		
		bool colorkey = !(pixel->r | pixel->g | pixel->b);
		
		if (fak > 0)
		{
			rr += fak;
			rg += fak;
			rb += fak;
		}
		rr = rr < 255 ? rr : 255;
		rg = rg < 255 ? rg : 255;
		rb = rb < 255 ? rb : 255;
		
		// This is clamped to 1 because dark colors would be
		// colorkeyed otherwise
		if (colorkey)
		{
			pixel->r = 0;
			pixel->g = 0;
			pixel->b = 0;
		}
		else
		{
			pixel->r = rr > 0 ? rr : 1;
			pixel->g = rg > 0 ? rg : 1;
			pixel->b = rb > 0 ? rb : 1;
		}
	}
	SDL_UnlockSurface(newSurface);
	SDL_Surface *convSurface = SDL_DisplayFormat(newSurface);
	SDL_FreeSurface(newSurface);
	return convSurface;
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
	SDL_WM_SetCaption("Blobby Volley 2 Alpha 5", "");
	mScreen = SDL_SetVideoMode(xResolution, yResolution, 0, screenFlags);
	SDL_ShowCursor(0);

	mOverlaySurface = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA,
		mScreen->w, mScreen->h, mScreen->format->BitsPerPixel,
		mScreen->format->Rmask, mScreen->format->Gmask,
		mScreen->format->Bmask, mScreen->format->Amask);
	SDL_Rect screenRect = {0, 0, xResolution, yResolution};
	SDL_FillRect(mOverlaySurface, &screenRect, SDL_MapRGB(mScreen->format, 0, 0, 0));
	
	
	SDL_Surface* tempBackground = loadSurface("gfx/strand2.bmp");
	mBackground = SDL_DisplayFormat(tempBackground);
	SDL_FreeSurface(tempBackground);
	
	for (int i = 1; i <= 16; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/ball%02d.bmp", i);
		SDL_Surface* ballImage = loadSurface(filename);
		SDL_SetColorKey(ballImage, SDL_SRCCOLORKEY, 
			SDL_MapRGB(ballImage->format, 0, 0, 0));
		SDL_Surface *convertedBallImage = SDL_DisplayFormat(ballImage);
		SDL_FreeSurface(ballImage);
		mBall.push_back(convertedBallImage);
	}
	
	SDL_Surface *tempBallShadow = loadSurface("gfx/schball.bmp");
	SDL_SetColorKey(tempBallShadow, SDL_SRCCOLORKEY, 
			SDL_MapRGB(tempBallShadow->format, 0, 0, 0));
	SDL_SetAlpha(tempBallShadow, SDL_SRCALPHA, 127);
	mBallShadow = SDL_DisplayFormat(tempBallShadow);
	SDL_FreeSurface(tempBallShadow);

	for (int i = 1; i <= 5; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/blobbym%d.bmp", i);
		SDL_Surface* blobImage = loadSurface(filename);
		mStandardBlob.push_back(blobImage);
		mLeftBlob.push_back(colorSurface(blobImage, Color(255, 0, 0)));
		mRightBlob.push_back(colorSurface(blobImage, Color(0, 255, 0)));
		
		sprintf(filename, "gfx/sch1%d.bmp", i);
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
		sprintf(filename, "gfx/font%02d.bmp", i);
		SDL_Surface *tempFont = loadSurface(filename);
		SDL_SetColorKey(tempFont, SDL_SRCCOLORKEY, 
			SDL_MapRGB(tempFont->format, 0, 0, 0));
		SDL_Surface *newFont = SDL_DisplayFormat(tempFont);
		SDL_FreeSurface(tempFont);
		mFont.push_back(newFont);
		mHighlightFont.push_back(highlightSurface(newFont, 60));
	}
}

void RenderManagerSDL::deinit()
{
	SDL_FreeSurface(mOverlaySurface);
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
	if (!mDrawGame)
		return;

	if (mNeedRedraw)
	{
		SDL_BlitSurface(mBackground, 0, mScreen, 0);
		mNeedRedraw = false;
	}
	
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
	
	// Mouse marker
	
	position.y = 590;
	position.x = lround(mMouseMarkerPosition - 2.5);
	position.w = 5;
	position.h = 5;
	SDL_FillRect(mScreen, &position, SDL_MapRGB(mScreen->format,
		     markerColor, markerColor, markerColor));
	
	// Ball Shadow
	position = ballShadowRect(ballShadowPosition(mBallPosition));
	SDL_BlitSurface(mBallShadow, 0, mScreen, &position);
		
	// Left blob shadow
	position = blobShadowRect(blobShadowPosition(mLeftBlobPosition));
	animationState = int(mLeftBlobAnimationState)  % 5;
	SDL_BlitSurface(mLeftBlobShadow[animationState], 0, mScreen, &position);

	// Right blob shadow
	position = blobShadowRect(blobShadowPosition(mRightBlobPosition));
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
	position = ballRect(mBallPosition);
	animationState = int(mBallRotation / M_PI / 2 * 16) % 16;
	SDL_BlitSurface(mBall[animationState], 0, mScreen, &position);

	// Drawing left blob
	
	position = blobRect(mLeftBlobPosition);
	animationState = int(mLeftBlobAnimationState)  % 5;
	SDL_BlitSurface(mLeftBlob[animationState], 0, mScreen, &position);
	
	// Drawing right blob

	position = blobRect(mRightBlobPosition);	
	animationState = int(mRightBlobAnimationState)  % 5;
	SDL_BlitSurface(mRightBlob[animationState], 0, mScreen, &position);

	// Drawing the score
	char textBuffer[8];
	snprintf(textBuffer, 8, mLeftPlayerWarning ? "%02d!" : "%02d",
			mLeftPlayerScore);
	drawText(textBuffer, Vector2(24, 24), false);
	snprintf(textBuffer, 8, mRightPlayerWarning ? "%02d!" : "%02d",
			mRightPlayerScore);	
	drawText(textBuffer, Vector2(800 - 96, 24), false);
}

bool RenderManagerSDL::setBackground(const std::string& filename)
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
	SDL_Rect restore = ballRect(mBallPosition);
	SDL_BlitSurface(mBackground, &restore, mScreen, &restore);
	restore = ballShadowRect(ballShadowPosition(mBallPosition));
	SDL_BlitSurface(mBackground, &restore, mScreen, &restore);
	restore.x = lround(mBallPosition.x - 2.5);
	restore.y = 5;
	restore.w = 5;
	restore.h = 5;
	SDL_BlitSurface(mBackground, &restore, mScreen, &restore);

	mBallPosition = position;
	mBallRotation = rotation;
}

void RenderManagerSDL::setMouseMarker(float position)
{
	SDL_Rect restore = {
		lround(mMouseMarkerPosition - 2.5),
		5,
		5,
		5
	};
	SDL_BlitSurface(mBackground, &restore, mScreen, &restore);
	mMouseMarkerPosition = position;
}
        
void RenderManagerSDL::setBlob(int player, 
		const Vector2& position, float animationState)
{
	SDL_Rect blobRestore;
	SDL_Rect shadowRestore;
	if (player == 0)
	{
		blobRestore = blobRect(mLeftBlobPosition);
		shadowRestore = blobShadowRect(
			blobShadowPosition(mLeftBlobPosition));
		mLeftBlobPosition = position;
		mLeftBlobAnimationState = animationState;
	}

	if (player == 1)
	{
		blobRestore = blobRect(mRightBlobPosition);
		shadowRestore = blobShadowRect(
			blobShadowPosition(mRightBlobPosition));
		mRightBlobPosition = position;
		mRightBlobAnimationState = animationState;
	}
	SDL_BlitSurface(mBackground, &blobRestore, mScreen, &blobRestore);
	SDL_BlitSurface(mBackground, &shadowRestore, mScreen, &shadowRestore);
}

void RenderManagerSDL::setScore(int leftScore, int rightScore,
	       bool leftWarning, bool rightWarning)
{
	SDL_Rect restore = {
		24,
		24,
		96,
		24
	};
	SDL_BlitSurface(mBackground, &restore, mScreen, &restore);
	restore.x = 800 - 96;
	SDL_BlitSurface(mBackground, &restore, mScreen, &restore);	
	
	mLeftPlayerScore = leftScore;
	mRightPlayerScore = rightScore;
	mLeftPlayerWarning = leftWarning;
	mRightPlayerWarning = rightWarning;
}

void RenderManagerSDL::drawText(const std::string& text, Vector2 position, bool highlight)
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

void RenderManagerSDL::drawImage(const std::string& filename, Vector2 position)
{
	BufferedImage* imageBuffer = mImageMap[filename];
	if (!imageBuffer)
	{
		imageBuffer = new BufferedImage;
		imageBuffer->sdlImage = loadSurface(filename);
		SDL_SetColorKey(imageBuffer->sdlImage, SDL_SRCCOLORKEY, 
			SDL_MapRGB(mScreen->format, 0, 0, 0));
		mImageMap[filename] = imageBuffer;
	}
	
	SDL_Rect blitRect = {
		lround(position.x - float(imageBuffer->sdlImage->w) / 2.0),
		lround(position.y - float(imageBuffer->sdlImage->h) / 2.0),
		lround(position.x + float(imageBuffer->sdlImage->w) / 2.0),
		lround(position.y + float(imageBuffer->sdlImage->h) / 2.0),
	};
	
	SDL_BlitSurface(imageBuffer->sdlImage, 0, mScreen, &blitRect);
}

void RenderManagerSDL::drawOverlay(float opacity, Vector2 pos1, Vector2 pos2)
{
	redraw();
	SDL_Rect ovRect;
	ovRect.x = lround(pos1.x);
	ovRect.y = lround(pos1.y);
	ovRect.w = lround(pos2.x - pos1.x);
	ovRect.h = lround(pos2.y - pos1.y);
	SDL_SetAlpha(mOverlaySurface, SDL_SRCALPHA, lround(opacity * 255));
	
	SDL_SetClipRect(mScreen, &ovRect);
	SDL_BlitSurface(mOverlaySurface, 0, mScreen, 0);
	SDL_SetClipRect(mScreen, 0);
}

void RenderManagerSDL::refresh()
{
	SDL_Flip(mScreen);
}
