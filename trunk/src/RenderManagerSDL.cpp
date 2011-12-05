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

#include <physfs.h>
#include "RenderManagerSDL.h"


RenderManagerSDL::DynamicColoredSurface RenderManagerSDL::colorSurface(SDL_Surface *surface, Color color)
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
	for (int p = 0; p < newSurface->w * newSurface->h; ++p)
	{
		SDL_Color* pixel = &(((SDL_Color*)newSurface->pixels)[p]);

		int rr = (int(pixel->r) * int(color.r)) >> 8;
		int rg = (int(pixel->g) * int(color.g)) >> 8;
		int rb = (int(pixel->b) * int(color.b)) >> 8;
		int fak = int(pixel->r) * 5 - 4 * 256 - 138;

		bool colorkey = !(pixel->r | pixel->g | pixel->b);

		if (colorkey)
		{
			pixel->r = 0;
			pixel->g = 0;
			pixel->b = 0;
			continue;
		}

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
		pixel->r = rr > 0 ? rr : 1;
		pixel->g = rg > 0 ? rg : 1;
		pixel->b = rb > 0 ? rb : 1;
		
	}
	SDL_UnlockSurface(newSurface);
	SDL_SetColorKey(newSurface, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(newSurface->format, 0, 0, 0));
	SDL_Surface *convSurface = SDL_DisplayFormatAlpha(newSurface);
	SDL_FreeSurface(newSurface);
	return DynamicColoredSurface(convSurface, color);
}

RenderManagerSDL::RenderManagerSDL()
	: RenderManager()
{
	mBallRotation = 0.0;
	mLeftBlobAnimationState = 0.0;
	mRightBlobAnimationState = 0.0;
}

RenderManager* RenderManager::createRenderManagerSDL()
{
	return new RenderManagerSDL();
}

void RenderManagerSDL::init(int xResolution, int yResolution, bool fullscreen)
{
	
	mLeftPlayerNameTexture = 0;
	mRightPlayerNameTexture = 0;
	
	Uint32 screenFlags = SDL_HWSURFACE | SDL_HWACCEL | SDL_DOUBLEBUF;
	if (fullscreen)
		screenFlags |= SDL_FULLSCREEN;
	SDL_WM_SetCaption(AppTitle, "");
	SDL_WM_SetIcon(SDL_LoadBMP("data/Icon.bmp"), NULL);
	mScreen = SDL_SetVideoMode(xResolution, yResolution, 0, screenFlags);
	SDL_ShowCursor(0);

	mOverlaySurface = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA,
		mScreen->w, mScreen->h, mScreen->format->BitsPerPixel,
		mScreen->format->Rmask, mScreen->format->Gmask,
		mScreen->format->Bmask, mScreen->format->Amask);
	SDL_Rect screenRect = {0, 0, (short)xResolution, (short)yResolution};
	SDL_FillRect(mOverlaySurface, &screenRect, SDL_MapRGB(mScreen->format, 0, 0, 0));


	SDL_Surface* tempBackground = loadSurface("backgrounds/strand2.bmp");
	mBackground = SDL_DisplayFormat(tempBackground);
	BufferedImage* bgImage = new BufferedImage;
	bgImage->w = mBackground->w;
	bgImage->h = mBackground->h;
	bgImage->sdlImage = mBackground;
	mImageMap["background"] = bgImage;
	SDL_FreeSurface(tempBackground);

	for (int i = 1; i <= 16; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/ball%02d.bmp", i);
		SDL_Surface* ballImage = loadSurface(filename);
		SDL_SetColorKey(ballImage, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(ballImage->format, 0, 0, 0));
		SDL_Surface *convertedBallImage = SDL_DisplayFormatAlpha(ballImage);
		SDL_FreeSurface(ballImage);
		mBall.push_back(convertedBallImage);
	}

	SDL_Surface *tempBallShadow = loadSurface("gfx/schball.bmp");
	SDL_SetColorKey(tempBallShadow, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(tempBallShadow->format, 0, 0, 0));
	SDL_SetAlpha(tempBallShadow, SDL_SRCALPHA, 127);
	mBallShadow = SDL_DisplayFormatAlpha(tempBallShadow);
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
		SDL_SetColorKey(blobShadow, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(blobShadow->format, 0, 0, 0));
		SDL_SetAlpha(blobShadow, SDL_SRCALPHA, 127);
		mStandardBlobShadow.push_back(blobShadow);
		mLeftBlobShadow.push_back(
			colorSurface(blobShadow, Color(255, 0, 0)));
		mRightBlobShadow.push_back(
			colorSurface(blobShadow, Color(0, 255, 0)));

	}

	for (int i = 0; i <= 53; ++i)
	{
		char filename[64], filename2[64];
		sprintf(filename, "gfx/font%02d.bmp", i);
		sprintf(filename2, "gfx/font_small/font%02d.bmp", i);
		SDL_Surface *tempFont = loadSurface(filename);
		SDL_Surface *tempFont2 = loadSurface(filename2);
		SDL_SetColorKey(tempFont, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(tempFont->format, 0, 0, 0));
		SDL_SetColorKey(tempFont2, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(tempFont2->format, 0, 0, 0));
		SDL_Surface *newFont = SDL_DisplayFormatAlpha(tempFont);
		SDL_Surface *newFont2 = SDL_DisplayFormatAlpha(tempFont2);
		SDL_FreeSurface(tempFont);
		SDL_FreeSurface(tempFont2);
		
		mFont.push_back(newFont);
		mHighlightFont.push_back(highlightSurface(newFont, 60));
		mSmallFont.push_back(newFont2);
		mHighlightSmallFont.push_back(highlightSurface(newFont2, 60));
	}

	mScroll = loadSurface("gfx/scrollbar.bmp");
	
	mStandardBlobBlood = loadSurface("gfx/blood.bmp");
	mLeftBlobBlood = loadSurface("gfx/blood.bmp");
	mRightBlobBlood = loadSurface("gfx/blood.bmp");
}

void RenderManagerSDL::deinit()
{
	SDL_FreeSurface(mOverlaySurface);
	SDL_FreeSurface(mBackground);
	SDL_FreeSurface(mBallShadow);
	SDL_FreeSurface(mScroll);
	
	SDL_FreeSurface(mLeftBlobBlood);
	SDL_FreeSurface(mRightBlobBlood);
	
	SDL_FreeSurface(mLeftPlayerNameTexture);
	SDL_FreeSurface(mRightPlayerNameTexture);
	
	for (unsigned int i = 0; i < mBall.size(); ++i)
		SDL_FreeSurface(mBall[i]);
	for (unsigned int i = 0; i < mStandardBlob.size(); ++i)
	{
		SDL_FreeSurface(mStandardBlob[i]);
		SDL_FreeSurface(mStandardBlobShadow[i]);
		SDL_FreeSurface(mLeftBlob[i].mSDLsf);
		SDL_FreeSurface(mLeftBlobShadow[i].mSDLsf);
		SDL_FreeSurface(mRightBlob[i].mSDLsf);
		SDL_FreeSurface(mRightBlobShadow[i].mSDLsf);
	}

	for (unsigned int i = 0; i < mFont.size(); ++i)
	{
		SDL_FreeSurface(mFont[i]);
		SDL_FreeSurface(mHighlightFont[i]);
		SDL_FreeSurface(mSmallFont[i]);
		SDL_FreeSurface(mHighlightSmallFont[i]);
	}
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

	if(mShowShadow)
	{
		// Ball Shadow
		position = ballShadowRect(ballShadowPosition(mBallPosition));
		SDL_BlitSurface(mBallShadow, 0, mScreen, &position);

		// Left blob shadow
		position = blobShadowRect(blobShadowPosition(mLeftBlobPosition));
		animationState = int(mLeftBlobAnimationState)  % 5;
		SDL_BlitSurface(mLeftBlobShadow[animationState].mSDLsf, 0, mScreen, &position);

		// Right blob shadow
		position = blobShadowRect(blobShadowPosition(mRightBlobPosition));
		animationState = int(mRightBlobAnimationState)  % 5;
		SDL_BlitSurface(mRightBlobShadow[animationState].mSDLsf, 0, mScreen, &position);
	}

	// Restore the rod
	position.x = 400 - 7;
	position.y = 300;
	SDL_Rect rodPosition;
	rodPosition.x = 400 - 7;
	rodPosition.y = 300;
	rodPosition.w = 14;
	rodPosition.h = 300;
	SDL_BlitSurface(mBackground, &rodPosition, mScreen, &position);
	// restore the background for clock
	position.x = 400 - mTime.length()*12;
	position.y =  24;
	rodPosition.x = 400 - mTime.length()*12;
	rodPosition.y = 24;
	rodPosition.w = mTime.length()*24;
	rodPosition.h = 24;
	SDL_BlitSurface(mBackground, &rodPosition, mScreen, &position);

	// Drawing the Ball
	position = ballRect(mBallPosition);
	animationState = int(mBallRotation / M_PI / 2 * 16) % 16;
	SDL_BlitSurface(mBall[animationState], 0, mScreen, &position);

	// update blob colors
	colorizeBlobs(LEFT_PLAYER);
	colorizeBlobs(RIGHT_PLAYER);

	// Drawing left blob
	position = blobRect(mLeftBlobPosition);
	animationState = int(mLeftBlobAnimationState)  % 5;
	SDL_BlitSurface(mLeftBlob[animationState].mSDLsf, 0, mScreen, &position);

	// Drawing right blob
	position = blobRect(mRightBlobPosition);
	animationState = int(mRightBlobAnimationState)  % 5;
	SDL_BlitSurface(mRightBlob[animationState].mSDLsf, 0, mScreen, &position);

	// Drawing the score
	char textBuffer[8];
	snprintf(textBuffer, sizeof(textBuffer), mLeftPlayerWarning ? "%02d!" : "%02d",
			mLeftPlayerScore);
	drawText(textBuffer, Vector2(24, 24), false);
	snprintf(textBuffer, sizeof(textBuffer), mRightPlayerWarning ? "%02d!" : "%02d",
			mRightPlayerScore);
	drawText(textBuffer, Vector2(800 - 96, 24), false);

	// Drawing the names
	//drawText(mLeftPlayerName, Vector2(12, 550), false);
	SDL_Rect rect;
	rect.x = 12;
	rect.y = 550;
	SDL_BlitSurface(mLeftPlayerNameTexture, 0, mScreen, &rect);

	drawText(mRightPlayerName, Vector2(788-(24*mRightPlayerName.length()), 550), false);
	
	// Drawing the Clock
	drawText(mTime, Vector2(400 - mTime.length()*12, 24), false);
}

bool RenderManagerSDL::setBackground(const std::string& filename)
{
	try
	{
		SDL_Surface *tempBackground = loadSurface(filename);
		SDL_FreeSurface(mBackground);
		delete mImageMap["background"];
		BufferedImage* newImage = new BufferedImage;
		newImage->w = tempBackground->w;
		newImage->h = tempBackground->h;
		newImage->sdlImage =  SDL_DisplayFormat(tempBackground);
		SDL_FreeSurface(tempBackground);
		mBackground = newImage->sdlImage;
		mImageMap["background"] = newImage;
	}
	catch (FileLoadException)
	{
		return false;
	}
	return true;
}

void RenderManagerSDL::setBlobColor(int player, Color color)
{
	if (color != mBlobColor[player]) {
		mBlobColor[player] = color;
	} else {
		return;
	}
	
	SDL_Surface** handledBlobBlood = 0;

	if (player == LEFT_PLAYER)
	{
		handledBlobBlood = &mLeftBlobBlood;
	}
	if (player == RIGHT_PLAYER)
	{
		handledBlobBlood = &mRightBlobBlood;
	}
	
	SDL_FreeSurface(*handledBlobBlood);
	*handledBlobBlood = colorSurface(mStandardBlobBlood, color).mSDLsf;
	
}


void RenderManagerSDL::colorizeBlobs(int player)
{
	std::vector<DynamicColoredSurface> *handledBlob = 0;
	std::vector<DynamicColoredSurface> *handledBlobShadow = 0;
	int frame;

	if (player == LEFT_PLAYER)
	{
		handledBlob = &mLeftBlob;
		handledBlobShadow = &mLeftBlobShadow;
		frame = mLeftBlobAnimationState;
	}
	if (player == RIGHT_PLAYER)
	{
		handledBlob = &mRightBlob;
		handledBlobShadow = &mRightBlobShadow;
		frame = mRightBlobAnimationState;
	}
	
	if( (*handledBlob)[frame].mColor != mBlobColor[player]) 
	{
		SDL_FreeSurface((*handledBlob)[frame].mSDLsf);
		(*handledBlob)[frame] = colorSurface(mStandardBlob[frame], mBlobColor[player]);
		SDL_FreeSurface((*handledBlobShadow)[frame].mSDLsf);
		(*handledBlobShadow)[frame] = colorSurface(mStandardBlobShadow[frame], mBlobColor[player]);
	}
}


void RenderManagerSDL::showShadow(bool shadow)
{
	mShowShadow = shadow;
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
		short(lround(mMouseMarkerPosition - 2.5)),
		590,
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
	if (player == LEFT_PLAYER)
	{
		blobRestore = blobRect(mLeftBlobPosition);
		shadowRestore = blobShadowRect(
			blobShadowPosition(mLeftBlobPosition));
		mLeftBlobPosition = position;
		mLeftBlobAnimationState = animationState;
	}

	if (player == RIGHT_PLAYER)
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

void RenderManagerSDL::setPlayernames(std::string leftName, std::string rightName)
{
	mLeftPlayerName = leftName;
	mRightPlayerName = rightName;
	
	int tl = mLeftPlayerName.size() * FONT_WIDTH_NORMAL;
	SDL_FreeSurface(mLeftPlayerNameTexture);
	SDL_Surface* surface = createEmptySurface(tl, 24);
	SDL_SetAlpha(surface, SDL_SRCALPHA, 255);
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(surface->format, 0, 0, 0));
	drawTextImpl(mLeftPlayerName, Vector2(0,0), TF_NORMAL, surface);
	
	mLeftPlayerNameTexture = SDL_DisplayFormatAlpha(surface);
	SDL_FreeSurface(surface);
	
	
	tl = mRightPlayerName.size() * FONT_WIDTH_NORMAL;
	SDL_FreeSurface(mRightPlayerNameTexture);
	surface = createEmptySurface(tl, 24);
	SDL_SetAlpha(surface, SDL_SRCALPHA, 255);
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(surface->format, 0, 0, 0));
	drawTextImpl(mRightPlayerName, Vector2(0,0), TF_NORMAL, surface);
	
	mRightPlayerNameTexture = SDL_DisplayFormatAlpha(surface);
	SDL_FreeSurface(surface);
}

void RenderManagerSDL::setTime(const std::string& t)
{
	mTime = t;
}


void RenderManagerSDL::drawText(const std::string& text, Vector2 position, unsigned int flags)
{
	drawTextImpl(text, position, flags, mScreen);
}

void RenderManagerSDL::drawTextImpl(const std::string& text, Vector2 position, unsigned int flags, SDL_Surface* screen)
{
	int FontSize = (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL);
	int length = 0;
	std::string string = text;
	int index = getNextFontIndex(string);
	while (index != -1)
	{
		if (flags & TF_OBFUSCATE)
			index = FONT_INDEX_ASTERISK;
		
		SDL_Rect charPosition;
		charPosition.x = lround(position.x) + length;
		charPosition.y = lround(position.y);
		
		if (flags & TF_SMALL_FONT)
			if (flags & TF_HIGHLIGHT)
				SDL_BlitSurface( mHighlightSmallFont[index], 0, screen, &charPosition );
			else
				SDL_BlitSurface( mSmallFont[index], 0, screen, &charPosition );
		else
			if (flags & TF_HIGHLIGHT)
				SDL_BlitSurface( mHighlightFont[index], 0, screen, &charPosition );
			else
				SDL_BlitSurface( mFont[index], 0, screen, &charPosition );
		
		index = getNextFontIndex(string);
		length += FontSize;
	}
}

void RenderManagerSDL::drawImage(const std::string& filename, Vector2 position)
{
	mNeedRedraw = true;
	BufferedImage* imageBuffer = mImageMap[filename];
	if (!imageBuffer)
	{
		imageBuffer = new BufferedImage;
		imageBuffer->sdlImage = loadSurface(filename);
		SDL_SetColorKey(imageBuffer->sdlImage, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			SDL_MapRGB(mScreen->format, 0, 0, 0));
		mImageMap[filename] = imageBuffer;
	}

	SDL_Rect blitRect = {
		(short)lround(position.x - float(imageBuffer->sdlImage->w) / 2.0),
		(short)lround(position.y - float(imageBuffer->sdlImage->h) / 2.0),
		(short)lround(position.x + float(imageBuffer->sdlImage->w) / 2.0),
		(short)lround(position.y + float(imageBuffer->sdlImage->h) / 2.0),
	};

	SDL_BlitSurface(imageBuffer->sdlImage, 0, mScreen, &blitRect);
}

void RenderManagerSDL::drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col)
{
	SDL_Rect ovRect;
	ovRect.x = lround(pos1.x);
	ovRect.y = lround(pos1.y);
	ovRect.w = lround(pos2.x - pos1.x);
	ovRect.h = lround(pos2.y - pos1.y);
	SDL_SetAlpha(mOverlaySurface, SDL_SRCALPHA, lround(opacity * 255));

	SDL_FillRect(mOverlaySurface, NULL, SDL_MapRGB(mScreen->format, col.r, col.g, col.b));

	SDL_SetClipRect(mScreen, &ovRect);
	SDL_BlitSurface(mOverlaySurface, 0, mScreen, 0);
	SDL_SetClipRect(mScreen, 0);
}

void RenderManagerSDL::drawBlob(const Vector2& pos, const Color& col)
{
	SDL_Rect position;
	position.x = lround(pos.x);
	position.y = lround(pos.y);
	// Workarround, so that surface will only be loaded when color is changed 
	// Works only when 2 blobs are on the screen (like in IMGUI)
	static Color lastColor[2] = {Color( -1, -1, -1), Color( -1, -1, -1)};
	
	static int toDraw = 0;

	if (lastColor[toDraw].val != col.val)
	{
		setBlobColor(toDraw, col);
		lastColor[toDraw] = col;
	}

	//  Second dirty workaround in the function to have the right position of blobs in the GUI
	position.x = position.x - (int)(75/2);
	position.y = position.y - (int)(89/2);

	if(toDraw == 1)
	{
		SDL_BlitSurface(mRightBlob[0].mSDLsf, 0, mScreen, &position);
		toDraw = 0;
	}
	else
	{
		SDL_BlitSurface(mLeftBlob[0].mSDLsf, 0, mScreen, &position);
		toDraw = 1;
	}
}

void RenderManagerSDL::drawParticle(const Vector2& pos, int player)
{
	mNeedRedraw = true;
	
	SDL_Rect blitRect = {
		(short)lround(pos.x - float(9) / 2.0),
		(short)lround(pos.y - float(9) / 2.0),
		(short)lround(pos.x + float(9) / 2.0),
		(short)lround(pos.y + float(9) / 2.0),
	};
	
	SDL_Surface* blood = player == LEFT_PLAYER ? mLeftBlobBlood : mRightBlobBlood;

	SDL_BlitSurface(blood, 0, mScreen, &blitRect);
}

void RenderManagerSDL::refresh()
{
	SDL_Flip(mScreen);
}
