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

#include "RenderManager.h"

#include "RenderManagerGL2D.h"
#include <physfs.h>

#if HAVE_LIBGL

// Standard values:
//		GL_DEPTH_TEST: false
//		GL_TEXTURE_2D: true
//		GL_ALPHA_TEST: true

// wrapper functions for debugging purposes
void RenderManagerGL2D::glEnable(unsigned int flag) {
	//mDebugStateChanges++;
	::glEnable(flag);
}

void RenderManagerGL2D::glDisable(unsigned int flag) {
	//mDebugStateChanges++;
	::glDisable(flag);
}

int RenderManagerGL2D::getNextPOT(int npot)
{
	int pot = 1;
	while (pot < npot)
		pot *= 2;
	return pot;
}

GLuint RenderManagerGL2D::loadTexture(SDL_Surface *surface, 
	bool specular)
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

	SDL_SetColorKey(textureSurface, SDL_SRCCOLORKEY, 
			SDL_MapRGB(textureSurface->format, 0, 0, 0));
	convertedTexture = 
		SDL_CreateRGBSurface(SDL_SWSURFACE,
			paddedX, paddedY, 32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
			0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif
	SDL_BlitSurface(textureSurface, 0, convertedTexture, &targetRect);

	if (specular)
	{
		for (int y = 0; y < convertedTexture->h; ++y)
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

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			convertedTexture->w, convertedTexture->h, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, convertedTexture->pixels);
	SDL_FreeSurface(textureSurface);
	SDL_FreeSurface(convertedTexture);
	
	return texture;
}

void RenderManagerGL2D::drawQuad(float x, float y)
{
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex2f(0.0 - x / 2.0, 0.0 - y / 2.0);
		glTexCoord2f(1.0, 0.0);
		glVertex2f(x / 2.0, 0.0 - y / 2.0);
		glTexCoord2f(1.0, 1.0);
		glVertex2f(x / 2.0, y / 2.0);
		glTexCoord2f(0.0, 1.0);
		glVertex2f(0.0 - x / 2.0, y / 2.0);
	glEnd();
}

void RenderManagerGL2D::drawQuad2(float x, float y, float w, float h)
{
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex2f(x - w / 2.0, y - h / 2.0);
		glTexCoord2f(1.0, 0.0);
		glVertex2f(x + w / 2.0, y - h / 2.0);
		glTexCoord2f(1.0, 1.0);
		glVertex2f(x + w / 2.0, y + h / 2.0);
		glTexCoord2f(0.0, 1.0);
		glVertex2f(x - w / 2.0, y + h / 2.0);
	glEnd();
}

RenderManagerGL2D::RenderManagerGL2D()
	: RenderManager()
{
}

RenderManager* RenderManager::createRenderManagerGL2D()
{
	return new RenderManagerGL2D();
}

void RenderManagerGL2D::init(int xResolution, int yResolution, bool fullscreen)
{
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	Uint32 screenFlags = SDL_OPENGL;
	if (fullscreen)
		screenFlags |= SDL_FULLSCREEN;
	SDL_WM_SetCaption(AppTitle, "");
	SDL_WM_SetIcon(SDL_LoadBMP("data/Icon.bmp"), NULL);
	SDL_SetVideoMode(xResolution, yResolution, 0, screenFlags);
	SDL_ShowCursor(0);
	glDisable(GL_MULTISAMPLE);
	
	mLeftBlobColor = Color(255, 0, 0);
	mRightBlobColor = Color(0, 255, 0);

	SDL_Surface* bgSurface = loadSurface("backgrounds/strand2.bmp");
	BufferedImage* bgBufImage = new BufferedImage;
	bgBufImage->w = getNextPOT(bgSurface->w);
	bgBufImage->h = getNextPOT(bgSurface->h);
	bgBufImage->glHandle = loadTexture(bgSurface, false);
	mBackground = bgBufImage->glHandle;
	mImageMap["background"] = bgBufImage;
	
	mBallShadow = loadTexture(loadSurface("gfx/schball.bmp"), false);	
	mScroll = loadTexture(loadSurface("gfx/scrollbar.bmp"), false);

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

	for (int i = 0; i <= 53; ++i)
	{
		char filename[64], filename2[64];
		sprintf(filename, "gfx/font%02d.bmp", i);
		sprintf(filename2, "gfx/font_small/font%02d.bmp", i);
		GLuint newFont = loadTexture(loadSurface(filename), false);
		GLuint newFont2 = loadTexture(loadSurface(filename2), false);
		SDL_Surface* fontSurface = loadSurface(filename);
		SDL_Surface* fontSurface2 = loadSurface(filename2);
		SDL_Surface* highlight = highlightSurface(fontSurface, 60);
		SDL_Surface* highlight2 = highlightSurface(fontSurface2, 60);
		SDL_FreeSurface(fontSurface);
		SDL_FreeSurface(fontSurface2);
		
		mFont.push_back(newFont);
		mHighlightFont.push_back(loadTexture(highlight, false));
		mSmallFont.push_back(newFont2);
		mHighlightSmallFont.push_back(loadTexture(highlight2, false));
	}

	mParticle = loadTexture(loadSurface("gfx/blood.bmp"), false);

	glViewport(0, 0, xResolution, yResolution);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 800, 600, 0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glAlphaFunc(GL_GREATER, 0.5);
	glEnable(GL_ALPHA_TEST);
}

void RenderManagerGL2D::deinit()
{
	glDeleteTextures(1, &mBackground);
	glDeleteTextures(mBall.size(), &mBall[0]);
	glDeleteTextures(1, &mBallShadow);
	glDeleteTextures(mBlob.size(), &mBlob[0]);
	glDeleteTextures(mBlobSpecular.size(), &mBlobSpecular[0]);
	glDeleteTextures(mBlobShadow.size(), &mBlobShadow[0]);
	glDeleteTextures(mFont.size(), &mFont[0]);
	glDeleteTextures(mHighlightFont.size(), &mHighlightFont[0]);
	glDeleteTextures(mSmallFont.size(), &mSmallFont[0]);
	glDeleteTextures(mHighlightSmallFont.size(), &mHighlightSmallFont[0]);
	glDeleteTextures(1, &mScroll);
	
	for (std::map<std::string, BufferedImage*>::iterator iter = mImageMap.begin();
		iter != mImageMap.end(); ++iter)
	{
		glDeleteTextures(1, &(*iter).second->glHandle);
		delete iter->second;
	}
	
	glDeleteTextures(1, &mParticle);
}

void RenderManagerGL2D::draw()
{
	glClear(GL_DEPTH_BUFFER_BIT);
	if (!mDrawGame)
		return;
		
	// Background
	glDisable(GL_ALPHA_TEST);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, mBackground);
	glLoadIdentity();
	drawQuad2(400.0, 300.0, 1024.0, 1024.0);
	
	
	if(mShowShadow)
	{
		// Generic shadow settings
		
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		// Blob shadows
		Vector2 pos;
	
		pos = blobShadowPosition(mLeftBlobPosition);
		glColor4ub(mLeftBlobColor.r, mLeftBlobColor.g, mLeftBlobColor.b, 128);
		glBindTexture(GL_TEXTURE_2D, mBlobShadow[int(mLeftBlobAnimationState)  % 5]);
		drawQuad2(pos.x, pos.y, 128.0, 32.0);
	
		pos = blobShadowPosition(mRightBlobPosition);
		glColor4ub(mRightBlobColor.r, mRightBlobColor.g, mRightBlobColor.b, 128);
		glBindTexture(GL_TEXTURE_2D, mBlobShadow[int(mRightBlobAnimationState)  % 5]);
		drawQuad2(pos.x, pos.y, 128.0, 32.0);

		// Ball shadow	
		pos = ballShadowPosition(mBallPosition);
		glColor4f(1.0, 1.0, 1.0, 0.5);
		glBindTexture(GL_TEXTURE_2D, mBallShadow);
		drawQuad2(pos.x, pos.y, 128.0, 32.0);

		glDisable(GL_BLEND);
	}
	
	glEnable(GL_ALPHA_TEST);
		
	// General object settings
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	// The Ball
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, mBall[int(mBallRotation / M_PI / 2 * 16) % 16]);
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
		drawQuad2(mBallPosition.x, mBallPosition.y, 64.0, 64.0);

/*
		opacity += 0.1;
	}
	if (mLastBallStates.size() > MotionBlurIterations)
			mLastBallStates.pop_back();
	glDisable(GL_BLEND);
*/	
	
	// blob normal
	// left blob
	glBindTexture(GL_TEXTURE_2D, mBlob[int(mLeftBlobAnimationState)  % 5]);
	glColor3ubv(mLeftBlobColor.val);
	drawQuad2(mLeftBlobPosition.x, mLeftBlobPosition.y, 128.0, 128.0);
	
	// right blob
	glBindTexture(GL_TEXTURE_2D, mBlob[int(mRightBlobAnimationState)  % 5]);
	glColor3ubv(mRightBlobColor.val);
	drawQuad2(mRightBlobPosition.x, mRightBlobPosition.y, 128.0, 128.0);

	// blob specular
	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	// left blob
	glBindTexture(GL_TEXTURE_2D, mBlobSpecular[int(mLeftBlobAnimationState)  % 5]);
	drawQuad2(mLeftBlobPosition.x, mLeftBlobPosition.y, 128.0, 128.0);

	// right blob
	glBindTexture(GL_TEXTURE_2D, mBlobSpecular[int(mRightBlobAnimationState)  % 5]);
	drawQuad2(mRightBlobPosition.x, mRightBlobPosition.y, 128.0, 128.0);
	
	glDisable(GL_BLEND);
	

	// Ball marker
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	GLubyte markerColor = SDL_GetTicks() % 1000 >= 500 ? 255 : 0;
	glColor3ub(markerColor, markerColor, markerColor);
	drawQuad2(mBallPosition.x, 7.5, 5.0, 5.0);

	// Mouse marker

	// Position relativ zu BallMarker
	drawQuad2(mMouseMarkerPosition, 592.5, 5.0, 5.0);
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	
	// Scores
	char textBuffer[64];
	snprintf(textBuffer, 8, mLeftPlayerWarning ? "%02d!" : "%02d",
			mLeftPlayerScore);
	drawText(textBuffer, Vector2(24, 24), false);
	snprintf(textBuffer, 8, mRightPlayerWarning ? "%02d!" : "%02d",
			mRightPlayerScore);	
	drawText(textBuffer, Vector2(728, 24), false);

	// Drawing the names
	drawText(mLeftPlayerName, Vector2(12, 550), false);

	drawText(mRightPlayerName, Vector2(788-(24*mRightPlayerName.length()), 550), false);
	
	// Drawing the clock
	drawText(mTime, Vector2(400 - mTime.length()*12, 24), false);
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
	catch (FileLoadException)
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

void RenderManagerGL2D::setBall(const Vector2& position, float rotation)
{
	mBallPosition = position;
	mBallRotation = rotation;
	
	static int mbCounter = 0;
	mbCounter++;
	if (mbCounter > 1)
	{
		mLastBallStates.push_front(position);
		mbCounter = 0;
	}
}

void RenderManagerGL2D::setBlob(int player, 
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

void RenderManagerGL2D::setScore(int leftScore, int rightScore,
	       bool leftWarning, bool rightWarning)
{
	mLeftPlayerScore = leftScore;
	mRightPlayerScore = rightScore;
	mLeftPlayerWarning = leftWarning;
	mRightPlayerWarning = rightWarning;
}

void RenderManagerGL2D::setPlayernames(std::string leftName, std::string rightName)
{
	mLeftPlayerName = leftName;
	mRightPlayerName = rightName;
}

void RenderManagerGL2D::setTime(const std::string& t)
{
	mTime = t;
}

void RenderManagerGL2D::drawText(const std::string& text, Vector2 position, unsigned int flags)
{
	glColor4f(1.0, 1.0, 1.0, 1.0);
	int FontSize = (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL);
	std::string string = text;
	int index = getNextFontIndex(string);
	
	float x = position.x - (FontSize / 2);
	float y = position.y + (FontSize / 2);
	while (index != -1)
	{
		if (flags & TF_OBFUSCATE)
			index = FONT_INDEX_ASTERISK;
		
		x += FontSize;
		if (flags & TF_SMALL_FONT)
			if (flags & TF_HIGHLIGHT)
				glBindTexture(GL_TEXTURE_2D, mHighlightSmallFont[index]);
			else
				glBindTexture(GL_TEXTURE_2D, mSmallFont[index]);
		else
			if (flags & TF_HIGHLIGHT)
				glBindTexture(GL_TEXTURE_2D, mHighlightFont[index]);
			else
				glBindTexture(GL_TEXTURE_2D, mFont[index]);
		
		// Why does the quad for correctly displayed 24px symbols have to be 32x32?
		float width = (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL+8);
		float height = (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL+8);
		drawQuad2(x, y, width, height);
		index = getNextFontIndex(string);
	}
}

void RenderManagerGL2D::drawImage(const std::string& filename, Vector2 position)
{
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
	glBindTexture(GL_TEXTURE_2D, imageBuffer->glHandle);
	drawQuad2(position.x, position.y, imageBuffer->w, imageBuffer->h);
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
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
}

void RenderManagerGL2D::drawBlob(const Vector2& pos, const Color& col)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	//glLoadIdentity();
	//glTranslatef(pos.x, pos.y, 0.6);
	glBindTexture(GL_TEXTURE_2D, mBlob[0]);
	glColor3ubv(col.val);
	drawQuad2(pos.x, pos.y, 128.0, 128.0);

	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, mBlobSpecular[0]);
	drawQuad2(pos.x, pos.y, 128.0, 128.0);
	glDisable(GL_BLEND);
}

void RenderManagerGL2D::startDrawParticles()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBindTexture(GL_TEXTURE_2D, mParticle);
	glBegin(GL_QUADS);
}

void RenderManagerGL2D::drawParticle(const Vector2& pos, int player)
{
	//glLoadIdentity();
	//glTranslatef(pos.x, pos.y, 0.6);
	
	if (player == LEFT_PLAYER)
		glColor3ubv(mLeftBlobColor.val);
	if (player == RIGHT_PLAYER)
		glColor3ubv(mRightBlobColor.val);
	if (player > 1)
		glColor3ubv(Color(255, 0, 0).val);
	
	float w = 9.0;
	float h = 9.0;
	glTexCoord2f(0.0, 0.0);
	glVertex2f(pos.x - w / 2.0, pos.y - h / 2.0);
	glTexCoord2f(1.0, 0.0);
	glVertex2f(pos.x + w / 2.0, pos.y - h / 2.0);
	glTexCoord2f(1.0, 1.0);
	glVertex2f(pos.x + w / 2.0, pos.y + h / 2.0);
	glTexCoord2f(0.0, 1.0);
	glVertex2f(pos.x - w / 2.0, pos.y + h / 2.0);
}

void RenderManagerGL2D::endDrawParticles()
{
	glEnd();
}	

void RenderManagerGL2D::refresh()
{
	//std::cout << mDebugStateChanges << "\n";
	SDL_GL_SwapBuffers();
	//mDebugStateChanges = 0;
}

#else

RenderManager* RenderManager::createRenderManagerGL2D()
{
	std::cerr << "OpenGL not available! Falling back to SDL renderer" <<
		std::endl;
	return RenderManager::createRenderManagerSDL();
}
        

#endif
