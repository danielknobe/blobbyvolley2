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
#include "RenderManagerGL2D.h"

#if HAVE_LIBGL

/* includes */
#include "FileExceptions.h"
#include "Color.h"
#include "Global.h"
#include "DuelMatchState.h"

/* implementation */
RenderManagerGL2D::Texture::Texture( GLuint tex, int x, int y, int width, int height, int tw, int th ) :
		w(width), h(height), texture(tex)
{
	assert(x + w <= tw);
	assert(y + h <= th);
	indices[0] = x / (float)tw;
	indices[1] = y / (float)th;
	indices[2] = (x + w) / (float)tw;
	indices[3] = y / (float)th;
	indices[4] = (x + w) / (float)tw;
	indices[5] = (y + h) / (float)th;
	indices[6] = x / (float)tw;
	indices[7] = (y + h) / (float)th;
}
int debugStateChanges = 0;
int debugBindTextureCount = 0;

// wrapper functions for debugging purposes
void RenderManagerGL2D::glEnable(unsigned int flag)
{
	if(mCurrentFlags.find(flag) != mCurrentFlags.end())
		return;

	debugStateChanges++;
	::glEnable(flag);
	mCurrentFlags.insert(flag);
}

void RenderManagerGL2D::glDisable(unsigned int flag)
{
	if( mCurrentFlags.find(flag) == mCurrentFlags.end() )
		return;

	debugStateChanges++;
	::glDisable(flag);
	mCurrentFlags.erase( mCurrentFlags.find(flag) );
}

void RenderManagerGL2D::glBindTexture(GLuint texture)
{
	if(mCurrentTexture == texture)
		return;

	debugBindTextureCount++;
	::glBindTexture(GL_TEXTURE_2D, texture);
	mCurrentTexture = texture;
}



int RenderManagerGL2D::getNextPOT(int npot)
{
	int pot = 1;
	while (pot < npot)
		pot *= 2;

	return pot;
}

GLuint RenderManagerGL2D::loadTexture(SDL_Surface *surface, bool specular)
{
	SDL_Surface* textureSurface;
	SDL_Surface* convertedTexture;

	textureSurface = surface;

	// Determine size of padding for 2^n format
	int oldX = textureSurface->w;
	int oldY = textureSurface->h;
	int paddedX = getNextPOT(textureSurface->w);
	int paddedY = getNextPOT(textureSurface->h);

	SDL_Rect targetRect;
	targetRect.w = oldX;
	targetRect.h = oldY;
	targetRect.x = (paddedX - oldX) / 2;
	targetRect.y = (paddedY - oldY) / 2;

	SDL_SetColorKey(textureSurface, SDL_TRUE,
	                SDL_MapRGB(textureSurface->format, 0, 0, 0));
	convertedTexture =
	        SDL_CreateRGBSurface(SDL_SWSURFACE,
	                             paddedX, paddedY, 32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	                0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
	                0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif
	SDL_BlitSurface(textureSurface, nullptr, convertedTexture, &targetRect);

	if (specular)
	{
		for (int y = 0; y < convertedTexture->h; ++y)
		{
			for (int x = 0; x < convertedTexture->w; ++x)
			{
				SDL_Color* pixel =
				        &(((SDL_Color*)convertedTexture->pixels)
				        [y * convertedTexture->w +x]);
				int luminance = int(pixel->r) * 5 - 4 * 256 - 138;
				luminance = luminance > 0 ? luminance : 0;
				luminance = luminance < 255 ? luminance : 255;
				pixel->r = luminance;
				pixel->g = luminance;
				pixel->b = luminance;
			}
		}
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
	             convertedTexture->w, convertedTexture->h, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, convertedTexture->pixels);
	SDL_FreeSurface(textureSurface);
	SDL_FreeSurface(convertedTexture);

	return texture;
}

void RenderManagerGL2D::drawQuad(float x, float y, float w, float h)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GLfloat vertices[] = {x - w / 2.f, y - h / 2.f,
	                      x + w / 2.f, y - h / 2.f,
	                      x + w / 2.f, y + h / 2.f,
	                      x - w / 2.f, y + h / 2.f};

	GLfloat texCoords[] = {0.f, 0.f,
	                       1.f, 0.f,
	                       1.f, 1.f,
	                       0.f, 1.f};

	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void RenderManagerGL2D::drawQuad(float x, float y, const Texture& tex) {
	glBindTexture(tex.texture);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	float w = tex.w;
	float h = tex.h;

	GLfloat vertices[] = {x - w / 2.f, y - h / 2.f,
	                      x + w / 2.f, y - h / 2.f,
	                      x + w / 2.f, y + h / 2.f,
	                      x - w / 2.f, y + h / 2.f};

	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, tex.indices);
	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

RenderManagerGL2D::RenderManagerGL2D() = default;

std::unique_ptr<RenderManager> RenderManager::createRenderManagerGL2D()
{
	return std::unique_ptr<RenderManager>{new RenderManagerGL2D()};
}

void RenderManagerGL2D::init(int xResolution, int yResolution, bool fullscreen)
{
	glDisable(GL_DEPTH_TEST);
#if !(defined _MSC_VER)
	mCurrentFlags.insert(GL_MULTISAMPLE);
#endif
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Set modesetting
	Uint32 screenFlags = SDL_WINDOW_OPENGL;
	if (fullscreen)
		screenFlags |= SDL_WINDOW_FULLSCREEN;

	// Create window
	mWindow = SDL_CreateWindow(AppTitle,
	                           SDL_WINDOWPOS_UNDEFINED,
	                           SDL_WINDOWPOS_UNDEFINED,
	                           xResolution, yResolution,
	                           screenFlags);

	// Set icon
#if !(defined BUILD_MACOS_BUNDLE)
	SDL_Surface* icon = loadSurface("Icon.bmp");
	SDL_SetColorKey(icon, SDL_TRUE,
	                SDL_MapRGB(icon->format, 0, 0, 0));
	SDL_SetWindowIcon(mWindow, icon);
	SDL_FreeSurface(icon);
#endif

	// Create gl context
	mGlContext = SDL_GL_CreateContext(mWindow);

	SDL_ShowCursor(0);
#if !(defined _MSC_VER)
	glDisable(GL_MULTISAMPLE);
#endif

	mLeftBlobColor = Color(255, 0, 0);
	mRightBlobColor = Color(0, 255, 0);
	glEnable(GL_TEXTURE_2D);

	// Load background
	SDL_Surface* bgSurface = loadSurface("backgrounds/strand2.bmp");
	BufferedImage* bgBufImage = new BufferedImage;
	bgBufImage->w = getNextPOT(bgSurface->w);
	bgBufImage->h = getNextPOT(bgSurface->h);
	bgBufImage->glHandle = loadTexture(bgSurface, false);
	mBackground = bgBufImage->glHandle;
	mImageMap["background"] = bgBufImage;

	mBallShadow = loadTexture(loadSurface("gfx/schball.bmp"), false);

	for (int i = 1; i <= 16; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/ball%02d.bmp", i);
		GLuint ballImage = loadTexture(loadSurface(filename), false);
		mBall.push_back(ballImage);
	}

	for (int i = 1; i <= 5; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/blobbym%d.bmp", i);
		GLuint blobImage = loadTexture(loadSurface(filename), false);
		mBlob.push_back(blobImage);
		sprintf(filename, "gfx/blobbym%d.bmp", i);
		GLuint blobSpecular = loadTexture(loadSurface(filename), true);
		mBlobSpecular.push_back(blobSpecular);
		sprintf(filename, "gfx/sch1%d.bmp", i);
		GLuint blobShadow = loadTexture(loadSurface(filename), false);
		mBlobShadow.push_back(blobShadow);
	}

	// create text base textures
	SDL_Surface* textbase = createEmptySurface(2048, 32);
	SDL_Surface* hltextbase = createEmptySurface(2048, 32);

	int x = 0;
	int sx = 0;

	for (int i = 0; i <= 58; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/font%02d.bmp", i);
		SDL_Surface* fontSurface = loadSurface(filename);

		SDL_Surface* highlight = highlightSurface(fontSurface, 60);

		SDL_Rect r = {(Uint16)x, 0, (Uint16)fontSurface->w, (Uint16)fontSurface->h};
		SDL_BlitSurface(fontSurface, nullptr, textbase, &r);
		SDL_BlitSurface(highlight, nullptr, hltextbase, &r);
		r.x = sx;
		r.y = 0;
		Texture s = Texture(0, x, 0, fontSurface->w, fontSurface->h, 2048, 32);
		mFont.push_back(s);
		mHighlightFont.push_back(s);

		x += fontSurface->w;

		SDL_FreeSurface(fontSurface);
		SDL_FreeSurface(highlight);
	}

	GLuint texture =  loadTexture(textbase, false);
	GLuint hltexture =  loadTexture(hltextbase, false);
	for (unsigned int i = 0; i < mFont.size(); ++i)
	{
		mFont[i].texture = texture;
		mHighlightFont[i].texture = hltexture;
	}

	mParticle = loadTexture(loadSurface("gfx/blood.bmp"), false);

	glViewport(0, 0, xResolution, yResolution);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 800, 600, 0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);

	glAlphaFunc(GL_GREATER, 0.5);
	glEnable(GL_ALPHA_TEST);
}

RenderManagerGL2D::~RenderManagerGL2D()
{
	glDeleteTextures(1, &mBackground);
	glDeleteTextures(mBall.size(), &mBall[0]);
	glDeleteTextures(1, &mBallShadow);
	glDeleteTextures(mBlob.size(), &mBlob[0]);
	glDeleteTextures(mBlobSpecular.size(), &mBlobSpecular[0]);
	glDeleteTextures(mBlobShadow.size(), &mBlobShadow[0]);
	glDeleteTextures(1/*mFont.size()*/, &mFont[0].texture);
	glDeleteTextures(/*mHighlightFont.size()*/1, &mHighlightFont[0].texture);

	for (auto& iter : mImageMap) {
		glDeleteTextures(1, &iter.second->glHandle);
		delete iter.second;
	}

	glDeleteTextures(1, &mParticle);

	SDL_GL_DeleteContext(mGlContext);
	SDL_DestroyWindow(mWindow);
}

bool RenderManagerGL2D::setBackground(const std::string& filename)
{
	try
	{
		SDL_Surface* newSurface = loadSurface(filename);
		glDeleteTextures(1, &mBackground);
		delete mImageMap["background"];
		BufferedImage *imgBuffer = new BufferedImage;
		imgBuffer->w = getNextPOT(newSurface->w);
		imgBuffer->h = getNextPOT(newSurface->h);
		imgBuffer->glHandle = loadTexture(newSurface, false);
		mBackground = imgBuffer->glHandle;
		mImageMap["background"] = imgBuffer;
	}
	catch (const FileLoadException&)
	{
		return false;
	}
	return true;
}

void RenderManagerGL2D::setBlobColor(int player, Color color)
{
	if (player == LEFT_PLAYER)
		mLeftBlobColor = color;

	if (player == RIGHT_PLAYER)
		mRightBlobColor = color;
}

void RenderManagerGL2D::showShadow(bool shadow)
{
	mShowShadow = shadow;
}

void RenderManagerGL2D::drawText(const std::string& text, Vector2 position, unsigned int flags)
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	glColor4f(1.0, 1.0, 1.0, 1.0);
	int FontSize = (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL);

	float x = position.x - (FontSize / 2);
	float y = position.y + (FontSize / 2);

	for (auto iter = text.cbegin(); iter != text.cend(); )

	{
		int index = getNextFontIndex(iter);

		if (flags & TF_OBFUSCATE)
			index = FONT_INDEX_ASTERISK;

		x += FontSize;
		if (flags & TF_SMALL_FONT)
		{
			if (flags & TF_HIGHLIGHT)
			{
				int charWidth = mHighlightFont[index].w;
				int charHeight = mHighlightFont[index].h;
				mHighlightFont[index].w = FONT_WIDTH_SMALL;
				mHighlightFont[index].h = FONT_WIDTH_SMALL;
				drawQuad(x, y, mHighlightFont[index]);
				mHighlightFont[index].w = charWidth;
				mHighlightFont[index].h = charHeight;
			}
			else
			{
				int charWidth = mFont[index].w;
				int charHeight = mFont[index].h;
				mFont[index].w = FONT_WIDTH_SMALL;
				mFont[index].h = FONT_WIDTH_SMALL;
				drawQuad(x, y, mFont[index]);
				mFont[index].w = charWidth;
				mFont[index].h = charHeight;
			}
		}
		else
		{
			if (flags & TF_HIGHLIGHT)
				drawQuad(x, y, mHighlightFont[index]);
			else
				drawQuad(x, y, mFont[index]);
		}
	}
}

void RenderManagerGL2D::drawImage(const std::string& filename, Vector2 position, Vector2 size)
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	BufferedImage* imageBuffer = mImageMap[filename];
	if (!imageBuffer)
	{
		imageBuffer = new BufferedImage;
		SDL_Surface* newSurface = loadSurface(filename);
		imageBuffer->w = getNextPOT(newSurface->w);
		imageBuffer->h = getNextPOT(newSurface->h);
		imageBuffer->glHandle = loadTexture(newSurface, false);
		mImageMap[filename] = imageBuffer;
	}

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glDisable(GL_BLEND);
	//glLoadIdentity();
	//glTranslatef(position.x , position.y, 0.0);
	glBindTexture(imageBuffer->glHandle);
	drawQuad(position.x, position.y, imageBuffer->w, imageBuffer->h);
}

void RenderManagerGL2D::drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);

	glColor4f(col.r, col.g, col.b, opacity);
	//glLoadIdentity();
	glBegin(GL_QUADS);
	glVertex2f(pos1.x, pos1.y);
	glVertex2f(pos1.x, pos2.y);
	glVertex2f(pos2.x, pos2.y);
	glVertex2f(pos2.x, pos1.y);
	glEnd();
}

void RenderManagerGL2D::drawBlob(const Vector2& pos, const Color& col)
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	//glLoadIdentity();
	//glTranslatef(pos.x, pos.y, 0.6);
	glBindTexture(mBlob[0]);
	glColor3ub(col.r, col.g, col.b);
	drawQuad(pos.x, pos.y, 128.0, 128.0);

	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(mBlobSpecular[0]);
	drawQuad(pos.x, pos.y, 128.0, 128.0);
	glDisable(GL_BLEND);
}

void RenderManagerGL2D::startDrawParticles()
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBindTexture(mParticle);
	glBegin(GL_QUADS);
}

void RenderManagerGL2D::drawParticle(const Vector2& pos, int player)
{
	//glLoadIdentity();
	//glTranslatef(pos.x, pos.y, 0.6);

	if (player == LEFT_PLAYER)
		glColor3ub(mLeftBlobColor.r, mLeftBlobColor.g, mLeftBlobColor.b);
	if (player == RIGHT_PLAYER)
		glColor3ub(mRightBlobColor.r, mRightBlobColor.g, mRightBlobColor.b);
	if (player > 1)
		glColor3ub(255, 0, 0);

	float w = 16.0;
	float h = 16.0;
	glTexCoord2f(0.0, 0.0);
	glVertex2f(pos.x - w / 2.f, pos.y - h / 2.f);
	glTexCoord2f(1.0, 0.0);
	glVertex2f(pos.x + w / 2.f, pos.y - h / 2.f);
	glTexCoord2f(1.0, 1.0);
	glVertex2f(pos.x + w / 2.f, pos.y + h / 2.f);
	glTexCoord2f(0.0, 1.0);
	glVertex2f(pos.x - w / 2.f, pos.y + h / 2.f);
}

void RenderManagerGL2D::endDrawParticles()
{
	glEnd();
}

void RenderManagerGL2D::refresh()
{
	//std::cout << debugStateChanges << "\n";
	SDL_GL_SwapWindow(mWindow);
	debugStateChanges = 0;
	//std::cerr << debugBindTextureCount << "\n";
	debugBindTextureCount = 0;

}

void RenderManagerGL2D::drawGame(const DuelMatchState& gameState)
{
// Background
	glDisable(GL_ALPHA_TEST);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(mBackground);
	glLoadIdentity();
	drawQuad(400.0, 300.0, 1024.0, 1024.0);


	if(mShowShadow)
	{
		// Generic shadow settings

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		// Blob shadows
		Vector2 pos;

		pos = blobShadowPosition(gameState.getBlobPosition(LEFT_PLAYER));
		glColor4ub(mLeftBlobColor.r, mLeftBlobColor.g, mLeftBlobColor.b, 128);
		glBindTexture(mBlobShadow[int(gameState.getBlobState(LEFT_PLAYER))  % 5]);
		drawQuad(pos.x, pos.y, 128.0, 32.0);

		pos = blobShadowPosition(gameState.getBlobPosition(RIGHT_PLAYER));
		glColor4ub(mRightBlobColor.r, mRightBlobColor.g, mRightBlobColor.b, 128);
		glBindTexture(mBlobShadow[int(gameState.getBlobState(RIGHT_PLAYER))  % 5]);
		drawQuad(pos.x, pos.y, 128.0, 32.0);

		// Ball shadow
		pos = ballShadowPosition(gameState.getBallPosition());
		glColor4f(1.0, 1.0, 1.0, 0.5);
		glBindTexture(mBallShadow);
		drawQuad(pos.x, pos.y, 128.0, 32.0);

		glDisable(GL_BLEND);
	}

	glEnable(GL_ALPHA_TEST);

	// General object settings
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	// The Ball

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(mBall[int(gameState.getBallRotation() / M_PI / 2 * 16) % 16]);
/*
	float opacity = 0.0;
	for (std::list<Vector2>::iterator iter = mLastBallStates.begin();
		iter != mLastBallStates.end(); ++iter)
	{
//		glColor4f(1.0 / MotionBlurIterations,
//			1.0 / MotionBlurIterations, 1.0 / MotionBlurIterations, 1.0 - opacity);
		glColor4f(1.0, 1.0, 1.0, opacity);


		Vector2& ballPosition = *iter;
*/
	drawQuad(gameState.getBallPosition().x, gameState.getBallPosition().y, 64.0, 64.0);

/*
		opacity += 0.1;
	}
	if (mLastBallStates.size() > MotionBlurIterations)
			mLastBallStates.pop_back();
	glDisable(GL_BLEND);
*/

	// blob normal
	// left blob
	glBindTexture(mBlob[int(gameState.getBlobState(LEFT_PLAYER))  % 5]);
	glColor3ub(mLeftBlobColor.r, mLeftBlobColor.g, mLeftBlobColor.b);
	drawQuad(gameState.getBlobPosition(LEFT_PLAYER).x,gameState.getBlobPosition(LEFT_PLAYER).y, 128.0, 128.0);

	// right blob
	glBindTexture(mBlob[int(gameState.getBlobState(RIGHT_PLAYER))  % 5]);
	glColor3ub(mRightBlobColor.r, mRightBlobColor.g, mRightBlobColor.b);
	drawQuad(gameState.getBlobPosition(RIGHT_PLAYER).x, gameState.getBlobPosition(RIGHT_PLAYER).y, 128.0, 128.0);

	// blob specular
	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	// left blob
	glBindTexture(mBlobSpecular[int(gameState.getBlobState(LEFT_PLAYER))  % 5]);
	drawQuad(gameState.getBlobPosition(LEFT_PLAYER).x,gameState.getBlobPosition(LEFT_PLAYER).y, 128.0, 128.0);

	// right blob
	glBindTexture(mBlobSpecular[int(gameState.getBlobState(RIGHT_PLAYER))  % 5]);
	drawQuad(gameState.getBlobPosition(RIGHT_PLAYER).x, gameState.getBlobPosition(RIGHT_PLAYER).y, 128.0, 128.0);

	glDisable(GL_BLEND);


	// Ball marker
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	GLubyte markerColor = SDL_GetTicks() % 1000 >= 500 ? 255 : 0;
	glColor3ub(markerColor, markerColor, markerColor);
	drawQuad(gameState.getBallPosition().x, 7.5, 5.0, 5.0);

	// Mouse marker

	// Position relativ zu BallMarker
	drawQuad(mMouseMarkerPosition, 592.5, 5.0, 5.0);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
}

#else

#include "RenderManager.h"

std::unique_ptr<RenderManager> RenderManager::createRenderManagerGL2D()
{
	std::cerr << "OpenGL not available! Falling back to SDL renderer" <<
		std::endl;
	return RenderManager::createRenderManagerSDL();
}


#endif
