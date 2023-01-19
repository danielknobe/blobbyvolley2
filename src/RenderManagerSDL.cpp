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
#include "DuelMatchState.h"

/* implementation */
SDL_Surface* RenderManagerSDL::colorSurface(SDL_Surface *surface, Color color)
{
	// Create new surface
	SDL_Surface *newSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);

	SDL_LockSurface(newSurface);

	Uint32* surface_pixels = (Uint32*)newSurface->pixels;

	for (int p = 0; p < newSurface->w * newSurface->h; ++p)
	{
		SDL_Color pixel;
		SDL_GetRGBA(surface_pixels[p], newSurface->format, &pixel.r, &pixel.g, &pixel.b, &pixel.a);

		const bool color_key = !(pixel.r | pixel.g | pixel.b);

		if(!color_key)
		{
			int rr = (int(pixel.r) * int(color.r)) >> 8;
			int rg = (int(pixel.g) * int(color.g)) >> 8;
			int rb = (int(pixel.b) * int(color.b)) >> 8;
			int fak = int(pixel.r) * 5 - 4 * 256 - 138;

			auto clamp = [fak](int value) {
				// Bright pixels in the original image should remain bright!
				if (fak > 0)
					value += fak;
				// This is clamped to 1 because dark colors may would be
				// color-keyed otherwise
				return std::max(1, std::min(value, 255));
			};

			surface_pixels[p] = SDL_MapRGBA(newSurface->format, clamp(rr), clamp(rg), clamp(rb), pixel.a);
		}


	}
	SDL_UnlockSurface(newSurface);

	// Use a black color-key
	SDL_SetColorKey(newSurface, SDL_TRUE, SDL_MapRGB(newSurface->format, 0, 0, 0));

	return newSurface;
}

RenderManagerSDL::RenderManagerSDL() = default;

std::unique_ptr<RenderManager> RenderManager::createRenderManagerSDL()
{
	return std::unique_ptr<RenderManager>{new RenderManagerSDL()};
}

void RenderManagerSDL::init(int xResolution, int yResolution, bool fullscreen)
{
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	// Set modesetting
	Uint32 screenFlags = 0;
#ifdef __SWITCH__
	screenFlags |= SDL_WINDOW_FULLSCREEN;
#else
	if (fullscreen)
	{
		screenFlags |= SDL_WINDOW_FULLSCREEN;
	}
	else
	{
		screenFlags |= SDL_WINDOW_RESIZABLE;
	}
#endif
	// Create window
	mWindow = SDL_CreateWindow(AppTitle,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		xResolution, yResolution,
		screenFlags);

#ifndef __SWITCH__
	// Set icon
	SDL_Surface* icon = loadSurface("Icon.bmp");
	SDL_SetColorKey(icon, SDL_TRUE,
			SDL_MapRGB(icon->format, 0, 0, 0));
	SDL_SetWindowIcon(mWindow, icon);
	SDL_FreeSurface(icon);
#endif

	// Create renderer to draw in window
#ifdef __SWITCH__
	mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#else
	mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
#endif

	// Hide mousecursor
	SDL_ShowCursor(0);

	// Create rendertarget to make window resizeable
	mRenderTarget = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, xResolution, yResolution);

	// Load all textures and surfaces to render the game
	SDL_Surface* tmpSurface;

	// Create a 1x1 black surface which will be scaled to draw an overlay
	tmpSurface = SDL_CreateRGBSurface(0, 1, 1, 32,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	// Because of SDL bug we can't check at the moment if color mod is available... no risk no fun ;)
	SDL_FillRect(tmpSurface, nullptr, SDL_MapRGB(tmpSurface->format, 255, 255, 255));
	mOverlayTexture = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	SDL_FreeSurface(tmpSurface);

	// Create marker texture for mouse and ball
	tmpSurface = SDL_CreateRGBSurface(0, 5, 5, 32,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	SDL_FillRect(tmpSurface, nullptr, SDL_MapRGB(tmpSurface->format, 255, 255, 255));
	mMarker[0] = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	SDL_FillRect(tmpSurface, nullptr, SDL_MapRGB(tmpSurface->format, 0, 0, 0));
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
		SDL_Surface* formattedBlobImage = SDL_ConvertSurfaceFormat(blobImage, SDL_PIXELFORMAT_ABGR8888, 0);
		SDL_FreeSurface(blobImage);

		SDL_SetColorKey(formattedBlobImage, SDL_TRUE,
				SDL_MapRGB(formattedBlobImage->format, 0, 0, 0));
		for(int j = 0; j < formattedBlobImage->w * formattedBlobImage->h; j++)
		{
			SDL_Color* pixel = &(((SDL_Color*)formattedBlobImage->pixels)[j]);
			if (!(pixel->r | pixel->g | pixel->b))
			{
				pixel->a = 0;
			}
		}

		mStandardBlob.push_back(formattedBlobImage);

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
				formattedBlobImage->w, formattedBlobImage->h);
		SDL_SetTextureBlendMode(leftBlobTex, SDL_BLENDMODE_BLEND);
		SDL_UpdateTexture(leftBlobTex, nullptr, formattedBlobImage->pixels, formattedBlobImage->pitch);

		mLeftBlob.emplace_back(
				leftBlobTex,
				Color(255, 255, 255));

		SDL_Texture* rightBlobTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formattedBlobImage->w, formattedBlobImage->h);
		SDL_SetTextureBlendMode(rightBlobTex, SDL_BLENDMODE_BLEND);
		SDL_UpdateTexture(rightBlobTex, nullptr, formattedBlobImage->pixels, formattedBlobImage->pitch);

		mRightBlob.emplace_back(
				rightBlobTex,
				Color(255, 255, 255));

		// Prepare blobby shadow textures
		SDL_Texture* leftBlobShadowTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formatedBlobShadowImage->w, formatedBlobShadowImage->h);
		SDL_SetTextureBlendMode(leftBlobShadowTex, SDL_BLENDMODE_BLEND);
		mLeftBlobShadow.emplace_back(
				leftBlobShadowTex,
				Color(255, 255, 255));
		SDL_UpdateTexture(leftBlobShadowTex, nullptr, formatedBlobShadowImage->pixels, formatedBlobShadowImage->pitch);

		SDL_Texture* rightBlobShadowTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formatedBlobShadowImage->w, formatedBlobShadowImage->h);
		SDL_SetTextureBlendMode(rightBlobShadowTex, SDL_BLENDMODE_BLEND);
		mRightBlobShadow.emplace_back(
				rightBlobShadowTex,
				Color(255, 255, 255));
		SDL_UpdateTexture(rightBlobShadowTex, nullptr, formatedBlobShadowImage->pixels, formatedBlobShadowImage->pitch);

		// Load specific icon to cancel a game
#if !BLOBBY_FEATURE_HAS_BACKBUTTON
		tmpSurface = loadSurface("gfx/flag.bmp");
		SDL_SetColorKey(tmpSurface, SDL_TRUE, SDL_MapRGB(tmpSurface->format, 0, 0, 0));
		mBackFlag = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
		SDL_FreeSurface(tmpSurface);
#endif
	}

	// Load font
	for (int i = 0; i <= 58; ++i)
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
	SDL_UpdateTexture(leftBlobBlood, nullptr, formatedBlobStandardBlood->pixels, formatedBlobStandardBlood->pitch);

	SDL_Texture* rightBlobBlood = SDL_CreateTexture(mRenderer,
			SDL_PIXELFORMAT_ABGR8888,
			SDL_TEXTUREACCESS_STREAMING,
			formatedBlobStandardBlood->w, formatedBlobStandardBlood->h);
	SDL_SetTextureBlendMode(rightBlobBlood, SDL_BLENDMODE_BLEND);
	mRightBlobBlood = DynamicColoredTexture(
			rightBlobBlood,
			Color(255, 0, 0));
	SDL_UpdateTexture(rightBlobBlood, nullptr, formatedBlobStandardBlood->pixels, formatedBlobStandardBlood->pitch);

SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
}

RenderManagerSDL::~RenderManagerSDL()
{
	SDL_DestroyTexture(mOverlayTexture);
	SDL_DestroyTexture(mRenderTarget);

	for(auto& i : mMarker) {
		SDL_DestroyTexture(i);
	}

	for(auto& i : mBall)
		SDL_DestroyTexture(i);

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

	for(const auto& image : mImageMap) {
		SDL_DestroyTexture(image.second->sdlImage);
		delete image.second;
	}

#if !BLOBBY_FEATURE_HAS_BACKBUTTON
	SDL_DestroyTexture(mBackFlag);
#endif

	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
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
	catch (const FileLoadException&)
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
	SDL_UpdateTexture(handledBlobBlood->mSDLsf, nullptr, tempSurface->pixels, tempSurface->pitch);
	SDL_FreeSurface(tempSurface);
}


void RenderManagerSDL::colorizeBlobs(int player, int frame)
{
	std::vector<DynamicColoredTexture> *handledBlob = nullptr;
	std::vector<DynamicColoredTexture> *handledBlobShadow = nullptr;

	if (player == LEFT_PLAYER)
	{
		handledBlob = &mLeftBlob;
		handledBlobShadow = &mLeftBlobShadow;
	}
	if (player == RIGHT_PLAYER)
	{
		handledBlob = &mRightBlob;
		handledBlobShadow = &mRightBlobShadow;
	}

	if( (*handledBlob)[frame].mColor != mBlobColor[player])
	{
		SDL_Surface* tempSurface = colorSurface(mStandardBlob[frame], mBlobColor[player]);
		SDL_UpdateTexture((*handledBlob)[frame].mSDLsf, nullptr, tempSurface->pixels, tempSurface->pitch);
		SDL_FreeSurface(tempSurface);

		SDL_Surface* tempSurface2 = colorSurface(mStandardBlobShadow[frame], mBlobColor[player]);
		SDL_UpdateTexture((*handledBlobShadow)[frame].mSDLsf, nullptr, tempSurface2->pixels, tempSurface2->pitch);
		SDL_FreeSurface(tempSurface2);

		(*handledBlob)[frame].mColor = mBlobColor[player];
	}
}


void RenderManagerSDL::showShadow(bool shadow)
{
	mShowShadow = shadow;
}

void RenderManagerSDL::drawText(const std::string& text, Vector2 position, unsigned int flags)
{
	drawTextImpl(text, position, flags);
}

void RenderManagerSDL::drawTextImpl(const std::string& text, Vector2 position, unsigned int flags)
{
	int FontSize = (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL);
	int length = 0;

	for (auto iter = text.cbegin(); iter != text.cend(); )
	{
		int index = getNextFontIndex(iter);

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
				SDL_RenderCopy(mRenderer, mHighlightFont[index], nullptr, &charRect);
			}
			else
			{
				SDL_RenderCopy(mRenderer,mFont[index], nullptr, &charRect);
			}
		}
		else
		{
			if (flags & TF_HIGHLIGHT)
			{
				SDL_QueryTexture(mHighlightFont[index], nullptr, nullptr, &charRect.w, &charRect.h);
				SDL_RenderCopy(mRenderer, mHighlightFont[index], nullptr, &charRect);
			}
			else
			{
				SDL_QueryTexture(mHighlightFont[index], nullptr, nullptr, &charRect.w, &charRect.h);
				SDL_RenderCopy(mRenderer,mFont[index], nullptr, &charRect);
			}
		}

		length += FontSize;
	}
}

void RenderManagerSDL::drawImage(const std::string& filename, Vector2 position, Vector2 size)
{
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

	if (size == Vector2(0,0))
	{
		// No scaling
		const SDL_Rect blitRect = {
			(short)lround(position.x - float(imageBuffer->w) / 2.0),
			(short)lround(position.y - float(imageBuffer->h) / 2.0),
			(short)imageBuffer->w,
			(short)imageBuffer->h
		};
		SDL_RenderCopy(mRenderer, imageBuffer->sdlImage, nullptr, &blitRect);
	}
	else
	{
		// Scaling
		const SDL_Rect blitRect = {
			(short)lround(position.x - float(size.x) / 2.0),
			(short)lround(position.y - float(size.y) / 2.0),
			(short)size.x,
			(short)size.y
		};
		SDL_RenderCopy(mRenderer, imageBuffer->sdlImage, nullptr, &blitRect);
	}

}

void RenderManagerSDL::drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col)
{
	SDL_Rect ovRect;
	ovRect.x = (int)lround(pos1.x);
	ovRect.y = (int)lround(pos1.y);
	ovRect.w = (int)lround(pos2.x - pos1.x);
	ovRect.h = (int)lround(pos2.y - pos1.y);
	SDL_SetTextureAlphaMod(mOverlayTexture, lround(opacity * 255));
	SDL_SetTextureColorMod(mOverlayTexture, col.r, col.g, col.b);
	SDL_RenderCopy(mRenderer, mOverlayTexture, nullptr, &ovRect);
}

void RenderManagerSDL::drawBlob(const Vector2& pos, const Color& col)
{
	SDL_Rect position;
	position.x = (int)lround(pos.x);
	position.y = (int)lround(pos.y);

	static int toDraw = 0;

	setBlobColor(toDraw, col);
	/// \todo this recolores the current frame (0)
	/// + shadows; that's not exactly what we want
	colorizeBlobs(toDraw, 0);


	//  Second dirty workaround in the function to have the right position of blobs in the GUI
	position.x = position.x - (int)(75/2);
	position.y = position.y - (int)(89/2);

	if(toDraw == 1)
	{
		SDL_QueryTexture(mRightBlob[0].mSDLsf, nullptr, nullptr, &position.w, &position.h);
		SDL_RenderCopy(mRenderer, mRightBlob[0].mSDLsf, nullptr, &position);
		toDraw = 0;
	}
	else
	{
		SDL_QueryTexture(mLeftBlob[0].mSDLsf, nullptr, nullptr, &position.w, &position.h);
		SDL_RenderCopy(mRenderer, mLeftBlob[0].mSDLsf, nullptr, &position);
		toDraw = 1;
	}
}

void RenderManagerSDL::drawParticle(const Vector2& pos, int player)
{
	SDL_Rect blitRect = {
		(short)lround(pos.x - float(9) / 2.0),
		(short)lround(pos.y - float(9) / 2.0),
		(short)9,
		(short)9,
	};

	DynamicColoredTexture blood = player == LEFT_PLAYER ? mLeftBlobBlood : mRightBlobBlood;

	SDL_RenderCopy(mRenderer, blood.mSDLsf, nullptr, &blitRect);
}

void RenderManagerSDL::refresh()
{
	SDL_SetRenderTarget(mRenderer, nullptr);

	// We have a resizeable window
	// Resize renderer if needed
	// TODO: We should catch the resize event
	SDL_Rect renderRect;
	int windowX;
	int windowY;
	SDL_RenderGetViewport(mRenderer, &renderRect);
	SDL_GetWindowSize(mWindow, &windowX, &windowY);
	if (renderRect.w != windowX || renderRect.h != windowY)
	{
		renderRect.w = windowX;
		renderRect.h = windowY;
		SDL_RenderSetViewport(mRenderer, &renderRect);
	}

	SDL_RenderCopy(mRenderer, mRenderTarget, nullptr, nullptr);
	SDL_RenderPresent(mRenderer);
	SDL_SetRenderTarget(mRenderer, mRenderTarget);
}

void RenderManagerSDL::drawGame(const DuelMatchState& gameState)
{
	SDL_RenderCopy(mRenderer, mBackground, nullptr, nullptr);

	SDL_Rect position;

	// Ball marker
	position.y = 5;
	position.x = (int)lround(gameState.getBallPosition().x - 2.5);
	position.w = 5;
	position.h = 5;
	SDL_RenderCopy(mRenderer, mMarker[(int)SDL_GetTicks() % 1000 >= 500], nullptr, &position);

	// Mouse marker
	position.y = 590;
	position.x = (int)lround(mMouseMarkerPosition - 2.5);
	position.w = 5;
	position.h = 5;
	SDL_RenderCopy(mRenderer, mMarker[(int)SDL_GetTicks() % 1000 >= 500], nullptr, &position);

	if(mShowShadow)
	{
		// Ball Shadow
		position = ballShadowRect(ballShadowPosition(gameState.getBallPosition()));
		SDL_RenderCopy(mRenderer, mBallShadow, nullptr, &position);

		// Left blob shadow
		position = blobShadowRect(blobShadowPosition(gameState.getBlobPosition(LEFT_PLAYER)));
		int animationState = int(gameState.getBlobState(LEFT_PLAYER)) % 5;
		SDL_RenderCopy(mRenderer, mLeftBlobShadow[animationState].mSDLsf, nullptr, &position);

		// Right blob shadow
		position = blobShadowRect(blobShadowPosition(gameState.getBlobPosition(RIGHT_PLAYER)));
		animationState = int(gameState.getBlobState(RIGHT_PLAYER)) % 5;
		SDL_RenderCopy(mRenderer, mRightBlobShadow[animationState].mSDLsf, nullptr, &position);
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

#if !BLOBBY_FEATURE_HAS_BACKBUTTON
	position.x = 400 - 35;
	position.y = 70;
	position.w = 70;
	position.h = 82;
	SDL_RenderCopy(mRenderer, mBackFlag, 0, &position);
#endif

	// Drawing the Ball
	position = ballRect(gameState.getBallPosition());
	int animationState = int(gameState.getBallRotation() / M_PI / 2 * 16) % 16;
	SDL_RenderCopy(mRenderer, mBall[animationState], nullptr, &position);

	// update blob colors
	int leftFrame = int(gameState.getBlobState(LEFT_PLAYER)) % 5;
	int rightFrame = int(gameState.getBlobState(RIGHT_PLAYER)) % 5;
	colorizeBlobs(LEFT_PLAYER, leftFrame);
	colorizeBlobs(RIGHT_PLAYER, rightFrame);

	// Drawing left blob
	position = blobRect(gameState.getBlobPosition(LEFT_PLAYER));
	SDL_RenderCopy( mRenderer, mLeftBlob[leftFrame].mSDLsf, nullptr, &position);

	// Drawing right blob
	position = blobRect(gameState.getBlobPosition(RIGHT_PLAYER));
	SDL_RenderCopy(mRenderer, mRightBlob[rightFrame].mSDLsf, nullptr, &position);
}
