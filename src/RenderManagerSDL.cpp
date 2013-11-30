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
#include "RenderManagerSDL.h"

/* includes */
#include "FileExceptions.h"

/* implementation */
SDL_Surface* RenderManagerSDL::colorSurface(SDL_Surface *surface, Color color)
{
	// Create new surface
	SDL_Surface *newSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);

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
			pixel->a = 0;
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

		// This is clamped to 1 because dark colors may would be
		// colorkeyed otherwise
		pixel->r = rr > 0 ? rr : 1;
		pixel->g = rg > 0 ? rg : 1;
		pixel->b = rb > 0 ? rb : 1;

	}
	SDL_UnlockSurface(newSurface);

	// Use a black colorkey
	SDL_SetColorKey(newSurface, SDL_TRUE,
			SDL_MapRGB(newSurface->format, 0, 0, 0));

	return newSurface;
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
	// Set modesetting
	Uint32 screenFlags = 0;
	if (fullscreen)
		screenFlags |= SDL_WINDOW_FULLSCREEN;

	// Create window
	mWindow = SDL_CreateWindow(AppTitle,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		xResolution, yResolution,
		screenFlags);

	// Set icon
	SDL_Surface* icon = SDL_LoadBMP("data/Icon.bmp");
	SDL_SetColorKey(icon, SDL_TRUE,
			SDL_MapRGB(icon->format, 0, 0, 0));
	SDL_SetWindowIcon(mWindow, icon);
	SDL_FreeSurface(icon);

	// Create renderer to draw in window
	mRenderer = SDL_CreateRenderer(mWindow, -1, 0);

	// Hide mousecursor
	SDL_ShowCursor(0);

	// Load all textures and surfaces to render the game
	SDL_Surface* tmpSurface;

	// Create a 1x1 black surface which will be scaled to draw an overlay
	tmpSurface = SDL_CreateRGBSurface(0, 1, 1, 32,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	// Because of SDL bug we can't check at the moment if color mod is available... no risk no fun ;)
	SDL_FillRect(tmpSurface, NULL, SDL_MapRGB(tmpSurface->format, 255, 255, 255));
	mOverlayTexture = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	SDL_FreeSurface(tmpSurface);

	// Create marker texture for mouse and ball
	tmpSurface = SDL_CreateRGBSurface(0, 5, 5, 32,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	SDL_FillRect(tmpSurface, NULL, SDL_MapRGB(tmpSurface->format, 255, 255, 255));
	mMarker[0] = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	SDL_FillRect(tmpSurface, NULL, SDL_MapRGB(tmpSurface->format, 0, 0, 0));
	mMarker[1] = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	SDL_FreeSurface(tmpSurface);

	// Load background
	tmpSurface = loadSurface("backgrounds/strand2.bmp");
	mBackground = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	BufferedImage* bgImage = new BufferedImage;
	bgImage->w = tmpSurface->w;
	bgImage->h = tmpSurface->h;
	bgImage->sdlImage = mBackground;
	SDL_FreeSurface(tmpSurface);
	mImageMap["background"] = bgImage;

	// Load ball
	for (int i = 1; i <= 16; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/ball%02d.bmp", i);
		tmpSurface = loadSurface(filename);
		SDL_SetColorKey(tmpSurface, SDL_TRUE,
				SDL_MapRGB(tmpSurface->format, 0, 0, 0));

		SDL_Texture *ballTexture = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
		SDL_FreeSurface(tmpSurface);
		mBall.push_back(ballTexture);
	}

	// Load ball shadow
	tmpSurface = loadSurface("gfx/schball.bmp");
	SDL_SetColorKey(tmpSurface, SDL_TRUE,
			SDL_MapRGB(tmpSurface->format, 0, 0, 0));

	SDL_SetSurfaceAlphaMod(tmpSurface, 127);
	mBallShadow = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	SDL_FreeSurface(tmpSurface);

	// Load blobby and shadows surface
	// Load streamed textures for coloring
	for (int i = 1; i <= 5; ++i)
	{
		// Load blobby surface
		char filename[64];
		sprintf(filename, "gfx/blobbym%d.bmp", i);
		SDL_Surface* blobImage = loadSurface(filename);
		SDL_Surface* formatedBlobImage = SDL_ConvertSurfaceFormat(blobImage, SDL_PIXELFORMAT_ABGR8888, 0);
		SDL_FreeSurface(blobImage);

		SDL_SetColorKey(formatedBlobImage, SDL_TRUE,
				SDL_MapRGB(formatedBlobImage->format, 0, 0, 0));
		for(int j = 0; j < formatedBlobImage->w * formatedBlobImage->h; j++)
		{
			SDL_Color* pixel = &(((SDL_Color*)formatedBlobImage->pixels)[j]);
			if (!(pixel->r | pixel->g | pixel->b))
			{
				pixel->a = 0;
			}
		}

		mStandardBlob.push_back(formatedBlobImage);

		// Load blobby shadow surface
		sprintf(filename, "gfx/sch1%d.bmp", i);
		SDL_Surface* blobShadow = loadSurface(filename);
		SDL_Surface* formatedBlobShadowImage = SDL_ConvertSurfaceFormat(blobShadow, SDL_PIXELFORMAT_ABGR8888, 0);
		SDL_FreeSurface(blobShadow);

		SDL_SetSurfaceAlphaMod(formatedBlobShadowImage, 127);
		SDL_SetColorKey(formatedBlobShadowImage, SDL_TRUE, SDL_MapRGB(formatedBlobShadowImage->format, 0, 0, 0));
		for(int j = 0; j < formatedBlobShadowImage->w * formatedBlobShadowImage->h; j++)
		{
			SDL_Color* pixel = &(((SDL_Color*)formatedBlobShadowImage->pixels)[j]);
			if (!(pixel->r | pixel->g | pixel->b))
			{
				pixel->a = 0;
			} else {
				pixel->a = 127;
			}
		}

		mStandardBlobShadow.push_back(formatedBlobShadowImage);

		// Prepare blobby textures
		SDL_Texture* leftBlobTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formatedBlobImage->w, formatedBlobImage->h);
		SDL_SetTextureBlendMode(leftBlobTex, SDL_BLENDMODE_BLEND);
		SDL_UpdateTexture(leftBlobTex, NULL, formatedBlobImage->pixels, formatedBlobImage->pitch);

		mLeftBlob.push_back(DynamicColoredTexture(
				leftBlobTex,
				Color(255, 255, 255)));

		SDL_Texture* rightBlobTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formatedBlobImage->w, formatedBlobImage->h);
		SDL_SetTextureBlendMode(rightBlobTex, SDL_BLENDMODE_BLEND);
		SDL_UpdateTexture(rightBlobTex, NULL, formatedBlobImage->pixels, formatedBlobImage->pitch);

		mRightBlob.push_back(DynamicColoredTexture(
				rightBlobTex,
				Color(255, 255, 255)));

		// Prepare blobby shadow textures
		SDL_Texture* leftBlobShadowTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formatedBlobShadowImage->w, formatedBlobShadowImage->h);
		SDL_SetTextureBlendMode(leftBlobShadowTex, SDL_BLENDMODE_BLEND);
		mLeftBlobShadow.push_back(DynamicColoredTexture(
				leftBlobShadowTex,
				Color(255, 255, 255)));
		SDL_UpdateTexture(leftBlobShadowTex, NULL, formatedBlobShadowImage->pixels, formatedBlobShadowImage->pitch);

		SDL_Texture* rightBlobShadowTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formatedBlobShadowImage->w, formatedBlobShadowImage->h);
		SDL_SetTextureBlendMode(rightBlobShadowTex, SDL_BLENDMODE_BLEND);
		mRightBlobShadow.push_back(DynamicColoredTexture(
				rightBlobShadowTex,
				Color(255, 255, 255)));
		SDL_UpdateTexture(rightBlobShadowTex, NULL, formatedBlobShadowImage->pixels, formatedBlobShadowImage->pitch);
	}

	// Load font
	for (int i = 0; i <= 53; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/font%02d.bmp", i);
		SDL_Surface* tempFont = loadSurface(filename);

		SDL_SetColorKey(tempFont, SDL_TRUE, SDL_MapRGB(tempFont->format, 0, 0, 0));
		mFont.push_back(SDL_CreateTextureFromSurface(mRenderer, tempFont));
		SDL_Surface* tempFont2 = highlightSurface(tempFont, 60);
		mHighlightFont.push_back(SDL_CreateTextureFromSurface(mRenderer, tempFont2));
		SDL_FreeSurface(tempFont);
		SDL_FreeSurface(tempFont2);
	}

	// Load blood surface
	SDL_Surface* blobStandardBlood = loadSurface("gfx/blood.bmp");
	SDL_Surface* formatedBlobStandardBlood = SDL_ConvertSurfaceFormat(blobStandardBlood, SDL_PIXELFORMAT_ABGR8888, 0);
	SDL_FreeSurface(blobStandardBlood);

	SDL_SetColorKey(formatedBlobStandardBlood, SDL_TRUE, SDL_MapRGB(formatedBlobStandardBlood->format, 0, 0, 0));
	for(int j = 0; j < formatedBlobStandardBlood->w * formatedBlobStandardBlood->h; j++)
	{
		SDL_Color* pixel = &(((SDL_Color*)formatedBlobStandardBlood->pixels)[j]);
		if (!(pixel->r | pixel->g | pixel->b))
		{
			pixel->a = 0;
		} else {
			pixel->a = 255;
		}
	}

	mStandardBlobBlood = formatedBlobStandardBlood;

	// Create streamed textures for blood
	SDL_Texture* leftBlobBlood = SDL_CreateTexture(mRenderer,
			SDL_PIXELFORMAT_ABGR8888,
			SDL_TEXTUREACCESS_STREAMING,
			formatedBlobStandardBlood->w, formatedBlobStandardBlood->h);
	SDL_SetTextureBlendMode(leftBlobBlood, SDL_BLENDMODE_BLEND);
	mLeftBlobBlood = DynamicColoredTexture(
			leftBlobBlood,
			Color(255, 0, 0));
	SDL_UpdateTexture(leftBlobBlood, NULL, formatedBlobStandardBlood->pixels, formatedBlobStandardBlood->pitch);

	SDL_Texture* rightBlobBlood = SDL_CreateTexture(mRenderer,
			SDL_PIXELFORMAT_ABGR8888,
			SDL_TEXTUREACCESS_STREAMING,
			formatedBlobStandardBlood->w, formatedBlobStandardBlood->h);
	SDL_SetTextureBlendMode(rightBlobBlood, SDL_BLENDMODE_BLEND);
	mRightBlobBlood = DynamicColoredTexture(
			rightBlobBlood,
			Color(255, 0, 0));
	SDL_UpdateTexture(rightBlobBlood, NULL, formatedBlobStandardBlood->pixels, formatedBlobStandardBlood->pitch);
}

void RenderManagerSDL::deinit()
{
	SDL_DestroyTexture(mOverlayTexture);

	for(unsigned int i = 0; i < 2; i++) {
		SDL_DestroyTexture(mMarker[i]);
	}
	SDL_DestroyTexture(mBackground);

	for (unsigned int i = 0; i < mBall.size(); ++i)
		SDL_DestroyTexture(mBall[i]);

	SDL_DestroyTexture(mBallShadow);

	for (unsigned int i = 0; i < mStandardBlob.size(); ++i)
	{
		SDL_FreeSurface(mStandardBlob[i]);
		SDL_FreeSurface(mStandardBlobShadow[i]);
		SDL_DestroyTexture(mLeftBlob[i].mSDLsf);
		SDL_DestroyTexture(mLeftBlobShadow[i].mSDLsf);
		SDL_DestroyTexture(mRightBlob[i].mSDLsf);
		SDL_DestroyTexture(mRightBlobShadow[i].mSDLsf);
	}

	SDL_FreeSurface(mStandardBlobBlood);
	SDL_DestroyTexture(mLeftBlobBlood.mSDLsf);
	SDL_DestroyTexture(mRightBlobBlood.mSDLsf);

	for (unsigned int i = 0; i < mFont.size(); ++i)
	{
		SDL_DestroyTexture(mFont[i]);
		SDL_DestroyTexture(mHighlightFont[i]);
	}

	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
}

void RenderManagerSDL::draw()
{
	if (!mDrawGame)
		return;

	SDL_RenderCopy(mRenderer,
			mBackground,
			NULL,
			NULL);

	int animationState;
	SDL_Rect position;

	// Ball marker
	position.y = 5;
	position.x = lround(mBallPosition.x - 2.5);
	position.w = 5;
	position.h = 5;
	SDL_RenderCopy(mRenderer, mMarker[(int)SDL_GetTicks() % 1000 >= 500], 0, &position);

	// Mouse marker
	position.y = 590;
	position.x = lround(mMouseMarkerPosition - 2.5);
	position.w = 5;
	position.h = 5;
	SDL_RenderCopy(mRenderer, mMarker[(int)SDL_GetTicks() % 1000 >= 500], 0, &position);

	if(mShowShadow)
	{
		// Ball Shadow
		position = ballShadowRect(ballShadowPosition(mBallPosition));
		SDL_RenderCopy(mRenderer, mBallShadow, 0, &position);

		// Left blob shadow
		position = blobShadowRect(blobShadowPosition(mLeftBlobPosition));
		animationState = int(mLeftBlobAnimationState) % 5;
		SDL_RenderCopy(mRenderer, mLeftBlobShadow[animationState].mSDLsf, 0, &position);

		// Right blob shadow
		position = blobShadowRect(blobShadowPosition(mRightBlobPosition));
		animationState = int(mRightBlobAnimationState) % 5;
		SDL_RenderCopy(mRenderer, mRightBlobShadow[animationState].mSDLsf, 0, &position);
	}

	// Restore the rod
	position.x = 400 - 7;
	position.y = 300;
	SDL_Rect rodPosition;
	rodPosition.x = 400 - 7;
	rodPosition.y = 300;
	rodPosition.w = 14;
	rodPosition.h = 300;
	SDL_RenderCopy(mRenderer, mBackground, &rodPosition, &rodPosition);

	// Drawing the Ball
	position = ballRect(mBallPosition);
	animationState = int(mBallRotation / M_PI / 2 * 16) % 16;
	SDL_RenderCopy(mRenderer, mBall[animationState], 0, &position);

	// update blob colors
	colorizeBlobs(LEFT_PLAYER);
	colorizeBlobs(RIGHT_PLAYER);

	// Drawing left blob
	position = blobRect(mLeftBlobPosition);
	animationState = int(mLeftBlobAnimationState) % 5;
	SDL_RenderCopy(mRenderer, mLeftBlob[animationState].mSDLsf, 0, &position);

	// Drawing right blob
	position = blobRect(mRightBlobPosition);
	animationState = int(mRightBlobAnimationState) % 5;
	SDL_RenderCopy(mRenderer, mRightBlob[animationState].mSDLsf, 0, &position);

	// Drawing the score
	char textBuffer[8];
	snprintf(textBuffer, sizeof(textBuffer), mLeftPlayerWarning ? "%02d!" : "%02d",
			mLeftPlayerScore);
	drawText(textBuffer, Vector2(24, 24), false);
	snprintf(textBuffer, sizeof(textBuffer), mRightPlayerWarning ? "%02d!" : "%02d",
			mRightPlayerScore);
	drawText(textBuffer, Vector2(800 - 96, 24), false);

	// Drawing the names
	drawText(mLeftPlayerName, Vector2(12, 550), false);
	drawText(mRightPlayerName, Vector2(788-(24*mRightPlayerName.length()), 550), false);

	// Drawing the Clock
	drawText(mTime, Vector2(400 - mTime.length()*12, 24), false);
}

bool RenderManagerSDL::setBackground(const std::string& filename)
{
	try
	{
		SDL_Surface *tempBackgroundSurface = loadSurface(filename);
		SDL_Texture *tempBackgroundTexture = SDL_CreateTextureFromSurface(mRenderer, tempBackgroundSurface);
		BufferedImage* oldBackground = mImageMap["background"];
		SDL_DestroyTexture(oldBackground->sdlImage);
		delete oldBackground;

		BufferedImage* newImage = new BufferedImage;
		newImage->w = tempBackgroundSurface->w;
		newImage->h = tempBackgroundSurface->h;
		newImage->sdlImage = tempBackgroundTexture;
		SDL_FreeSurface(tempBackgroundSurface);
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

	DynamicColoredTexture* handledBlobBlood;

	if (player == LEFT_PLAYER)
	{
		handledBlobBlood = &mLeftBlobBlood;
	}
	if (player == RIGHT_PLAYER)
	{
		handledBlobBlood = &mRightBlobBlood;
	}

	SDL_Surface* tempSurface = colorSurface(mStandardBlobBlood, mBlobColor[player]);
	SDL_UpdateTexture(handledBlobBlood->mSDLsf, NULL, tempSurface->pixels, tempSurface->pitch);
	SDL_FreeSurface(tempSurface);
}


void RenderManagerSDL::colorizeBlobs(int player)
{
	std::vector<DynamicColoredTexture> *handledBlob = 0;
	std::vector<DynamicColoredTexture> *handledBlobShadow = 0;
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
		SDL_Surface* tempSurface = colorSurface(mStandardBlob[frame], mBlobColor[player]);
		SDL_UpdateTexture((*handledBlob)[frame].mSDLsf, NULL, tempSurface->pixels, tempSurface->pitch);
		SDL_FreeSurface(tempSurface);

		SDL_Surface* tempSurface2 = colorSurface(mStandardBlobShadow[frame], mBlobColor[player]);
		SDL_UpdateTexture((*handledBlobShadow)[frame].mSDLsf, NULL, tempSurface2->pixels, tempSurface2->pitch);
		SDL_FreeSurface(tempSurface2);

		(*handledBlob)[frame].mColor = mBlobColor[player];
	}
}


void RenderManagerSDL::showShadow(bool shadow)
{
	mShowShadow = shadow;
}

void RenderManagerSDL::setBall(const Vector2& position, float rotation)
{
	mBallPosition = position;
	mBallRotation = rotation;
}

void RenderManagerSDL::setMouseMarker(float position)
{
	mMouseMarkerPosition = position;
}

void RenderManagerSDL::setBlob(int player,
		const Vector2& position, float animationState)
{
	if (player == LEFT_PLAYER)
	{
		mLeftBlobPosition = position;
		mLeftBlobAnimationState = animationState;
	}

	if (player == RIGHT_PLAYER)
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

void RenderManagerSDL::setPlayernames(std::string leftName, std::string rightName)
{
	mLeftPlayerName = leftName;
	mRightPlayerName = rightName;
}

void RenderManagerSDL::setTime(const std::string& t)
{
	mTime = t;
}


void RenderManagerSDL::drawText(const std::string& text, Vector2 position, unsigned int flags)
{
	drawTextImpl(text, position, flags);
}

void RenderManagerSDL::drawTextImpl(const std::string& text, Vector2 position, unsigned int flags)
{
	int FontSize = (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL);
	int length = 0;
	std::string string = text;
	int index = getNextFontIndex(string);
	while (index != -1)
	{
		if (flags & TF_OBFUSCATE)
			index = FONT_INDEX_ASTERISK;

		SDL_Rect charRect;
		charRect.x = lround(position.x) + length;
		charRect.y = lround(position.y);

		if (flags & TF_SMALL_FONT)
		{
			charRect.w = FONT_WIDTH_SMALL;
			charRect.h = FONT_WIDTH_SMALL;
			if (flags & TF_HIGHLIGHT)
			{
				SDL_RenderCopy(mRenderer, mHighlightFont[index], NULL, &charRect);
			}
			else
			{
				SDL_RenderCopy(mRenderer,mFont[index], NULL, &charRect);
			}
		}
		else
		{
			if (flags & TF_HIGHLIGHT)
			{
				SDL_QueryTexture(mHighlightFont[index], NULL, NULL, &charRect.w, &charRect.h);
				SDL_RenderCopy(mRenderer, mHighlightFont[index], NULL, &charRect);
			}
			else
			{
				SDL_QueryTexture(mHighlightFont[index], NULL, NULL, &charRect.w, &charRect.h);
				SDL_RenderCopy(mRenderer,mFont[index], NULL, &charRect);
			}
		}

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
		SDL_Surface* tmpSurface = loadSurface(filename);
		SDL_SetColorKey(tmpSurface, SDL_TRUE,
				SDL_MapRGB(tmpSurface->format, 0, 0, 0));
		imageBuffer->sdlImage = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
		imageBuffer->w = tmpSurface->w;
		imageBuffer->h = tmpSurface->h;
		SDL_FreeSurface(tmpSurface);
		mImageMap[filename] = imageBuffer;
	}

	const SDL_Rect blitRect = {
		(short)lround(position.x - float(imageBuffer->w) / 2.0),
		(short)lround(position.y - float(imageBuffer->h) / 2.0),
		(short)imageBuffer->w,
		(short)imageBuffer->h
	};

	SDL_RenderCopy(mRenderer, imageBuffer->sdlImage, NULL, &blitRect);
}

void RenderManagerSDL::drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col)
{
	SDL_Rect ovRect;
	ovRect.x = lround(pos1.x);
	ovRect.y = lround(pos1.y);
	ovRect.w = lround(pos2.x - pos1.x);
	ovRect.h = lround(pos2.y - pos1.y);
	SDL_SetTextureAlphaMod(mOverlayTexture, lround(opacity * 255));
	SDL_SetTextureColorMod(mOverlayTexture, col.r, col.g, col.b);
	SDL_RenderCopy(mRenderer, mOverlayTexture, NULL, &ovRect);
}

void RenderManagerSDL::drawBlob(const Vector2& pos, const Color& col)
{
	SDL_Rect position;
	position.x = lround(pos.x);
	position.y = lround(pos.y);

	static int toDraw = 0;

	mLeftBlobAnimationState = 0;
	mRightBlobAnimationState = 0;

	setBlobColor(toDraw, col);
	/// \todo this recolores the current frame (0)
	/// + shadows; thats not exactly what we want
	colorizeBlobs(toDraw);


	//  Second dirty workaround in the function to have the right position of blobs in the GUI
	position.x = position.x - (int)(75/2);
	position.y = position.y - (int)(89/2);

	if(toDraw == 1)
	{
		SDL_QueryTexture(mRightBlob[mRightBlobAnimationState].mSDLsf, NULL, NULL, &position.w, &position.h);
		SDL_RenderCopy(mRenderer, mRightBlob[mRightBlobAnimationState].mSDLsf, 0, &position);
		toDraw = 0;
	}
	else
	{
		SDL_QueryTexture(mLeftBlob[mRightBlobAnimationState].mSDLsf, NULL, NULL, &position.w, &position.h);
		SDL_RenderCopy(mRenderer, mLeftBlob[mRightBlobAnimationState].mSDLsf, 0, &position);
		toDraw = 1;
	}
}

void RenderManagerSDL::drawParticle(const Vector2& pos, int player)
{
	mNeedRedraw = true;

	SDL_Rect blitRect = {
		(short)lround(pos.x - float(9) / 2.0),
		(short)lround(pos.y - float(9) / 2.0),
		(short)9,
		(short)9,
	};

	DynamicColoredTexture blood = player == LEFT_PLAYER ? mLeftBlobBlood : mRightBlobBlood;

	SDL_RenderCopy(mRenderer, blood.mSDLsf, 0, &blitRect);
}

void RenderManagerSDL::refresh()
{
	SDL_RenderPresent(mRenderer);
}
