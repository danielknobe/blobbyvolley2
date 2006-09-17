#include "RenderManager.h"

#include "RenderManagerGL2D.h"
#include <physfs.h>

#if HAVE_LIBGL

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
			0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
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
	SDL_WM_SetCaption("Blobby Volley 2 Alpha 5", "");
	SDL_SetVideoMode(xResolution, yResolution, 0, screenFlags);
	SDL_ShowCursor(0);
	glDisable(GL_MULTISAMPLE);
	
	mLeftBlobColor = Color(255, 0, 0);
	mRightBlobColor = Color(0, 255, 0);

	mBackground = loadTexture(loadSurface("gfx/strand2.bmp"), false);
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

	for (int i = 0; i <= 51; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/font%02d.bmp", i);
		GLuint newFont = loadTexture(loadSurface(filename), false);
		SDL_Surface* fontSurface = loadSurface(filename);
		SDL_Surface* highlight = highlightSurface(fontSurface, 60);
		SDL_FreeSurface(fontSurface);
		
		mHighlightFont.push_back(loadTexture(highlight, false));		
		mFont.push_back(newFont);
	}

	glViewport(0, 0, xResolution, yResolution);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 800, 600, 0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
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
	
	for (std::map<std::string, BufferedImage*>::iterator iter = mImageMap.begin();
		iter != mImageMap.end(); ++iter)
	{
		glDeleteTextures(1, &(*iter).second->glHandle);
		delete iter->second;
	}
}

void RenderManagerGL2D::draw()
{
	glClear(GL_DEPTH_BUFFER_BIT);
	if (!mDrawGame)
		return;
		
	// General object settings
	glEnable(GL_TEXTURE_2D);
	glAlphaFunc(GL_GREATER, 0.5);
	glEnable(GL_ALPHA_TEST);
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
		glLoadIdentity();
		glTranslatef(mBallPosition.x, mBallPosition.y, 0.5);

		drawQuad(64.0, 64.0);
/*
		opacity += 0.1;
	}
	if (mLastBallStates.size() > MotionBlurIterations)
			mLastBallStates.pop_back();
	glDisable(GL_BLEND);
*/	
	
	// left blob
	glLoadIdentity();
	glTranslatef(mLeftBlobPosition.x, mLeftBlobPosition.y, 0.6);
	glBindTexture(GL_TEXTURE_2D, mBlob[int(mLeftBlobAnimationState)  % 5]);
	glColor3ubv(mLeftBlobColor.val);
	drawQuad(128.0, 128.0);

	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, mBlobSpecular[int(mLeftBlobAnimationState)  % 5]);
	drawQuad(128.0, 128.0);
	glDisable(GL_BLEND);

	// right blob
	glLoadIdentity();
	glTranslatef(mRightBlobPosition.x, mRightBlobPosition.y, 0.6);
	glBindTexture(GL_TEXTURE_2D, mBlob[int(mRightBlobAnimationState)  % 5]);
	glColor3ubv(mRightBlobColor.val);
	drawQuad(128.0, 128.0);

	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, mBlobSpecular[int(mRightBlobAnimationState)  % 5]);
	drawQuad(128.0, 128.0);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	
	// Background
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, mBackground);
	glLoadIdentity();
	glTranslatef(400.0, 300.0, -0.5);
	drawQuad(1024, 1024);

	// Secure rod

	glLoadIdentity();
	glTranslatef(0.0, 0.0, 1.0);
	glColorMask(false, false, false, false);
	glBegin(GL_QUADS);
		glVertex2f(393.0, 300.0);
		glVertex2f(407.0, 300.0);
		glVertex2f(407.0, 600.0);
		glVertex2f(393.0, 600.0);
	glEnd();
	glColorMask(true, true, true, true);

	// Ball marker
	
	glDisable(GL_TEXTURE_2D);
	GLubyte markerColor = SDL_GetTicks() % 1000 >= 500 ? 255 : 0;
	glColor3ub(markerColor, markerColor, markerColor);
	glLoadIdentity();
	glTranslatef(mBallPosition.x, 7.5, -0.4);
	drawQuad(5.0, 5.0);

	// Mouse marker

	glLoadIdentity();
	glTranslatef(mMouseMarkerPosition, 592.5, -0.4);
	drawQuad(5.0, 5.0);

	// Generic shadow settings
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	// Blob shadows
	Vector2 pos;
	
	glLoadIdentity();
	pos = blobShadowPosition(mLeftBlobPosition);
	glTranslatef(pos.x, pos.y, 0.2);
	glColor4ub(mLeftBlobColor.r, mLeftBlobColor.g, mLeftBlobColor.b, 128);
	glBindTexture(GL_TEXTURE_2D, mBlobShadow[int(mLeftBlobAnimationState)  % 5]);
	drawQuad(128.0, 32.0);
	
	glLoadIdentity();
	pos = blobShadowPosition(mRightBlobPosition);
	glTranslatef(pos.x, pos.y, 0.2);
	glColor4ub(mRightBlobColor.r, mRightBlobColor.g, mRightBlobColor.b, 128);
	glBindTexture(GL_TEXTURE_2D, mBlobShadow[int(mRightBlobAnimationState)  % 5]);
	drawQuad(128.0, 32.0);

	// Ball shadow	
	glLoadIdentity();
	pos = ballShadowPosition(mBallPosition);
	glTranslatef(pos.x, pos.y, 0.2);
	glColor4f(1.0, 1.0, 1.0, 0.5);
	glBindTexture(GL_TEXTURE_2D, mBallShadow);
	drawQuad(128.0, 32.0);

	glDisable(GL_BLEND);
	
	// Scores
	char textBuffer[64];
	snprintf(textBuffer, 8, mLeftPlayerWarning ? "%02d!" : "%02d",
			mLeftPlayerScore);
	drawText(textBuffer, Vector2(24, 24), false);
	snprintf(textBuffer, 8, mRightPlayerWarning ? "%02d!" : "%02d",
			mRightPlayerScore);	
	drawText(textBuffer, Vector2(800 - 96, 24), false);

}

bool RenderManagerGL2D::setBackground(const std::string& filename)
{
	GLuint newBackground;
	try
	{
		newBackground = loadTexture(loadSurface(filename), false);
	}
	catch (FileLoadException)
	{
		return false;
	}
	glDeleteTextures(1, &mBackground);
	mBackground = newBackground;
		
	return true;
}

void RenderManagerGL2D::setBlobColor(int player, Color color)
{
	if (player == 0)
		mLeftBlobColor = color;
	if (player == 1)
		mRightBlobColor = color;
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

void RenderManagerGL2D::setScore(int leftScore, int rightScore,
	       bool leftWarning, bool rightWarning)
{
	mLeftPlayerScore = leftScore;
	mRightPlayerScore = rightScore;
	mLeftPlayerWarning = leftWarning;
	mRightPlayerWarning = rightWarning;
}

void RenderManagerGL2D::drawText(const std::string& text, Vector2 position, bool highlight)
{
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glAlphaFunc(GL_GREATER, 0.5);
	glEnable(GL_ALPHA_TEST);
	int length = 0;
	std::string string = text;
	int index = getNextFontIndex(string);
	while (index != -1)
	{
		glLoadIdentity();
		glTranslatef(position.x + length + 12.0,
				position.y + 12.0, 0.0);
		if (highlight)
			glBindTexture(GL_TEXTURE_2D, mHighlightFont[index]);
		else
			glBindTexture(GL_TEXTURE_2D, mFont[index]);
		drawQuad(32.0, 32.0);
		index = getNextFontIndex(string);
		length += 24;
	}
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
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
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glAlphaFunc(GL_GREATER, 0.5);
	glEnable(GL_ALPHA_TEST);
	glLoadIdentity();
	glTranslatef(position.x , position.y, 0.0);
	glBindTexture(GL_TEXTURE_2D, imageBuffer->glHandle);
	drawQuad(imageBuffer->w, imageBuffer->h);

	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
}
void RenderManagerGL2D::drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	
	glColor4f(col.r, col.g, col.b, opacity);
	glLoadIdentity();
	glBegin(GL_QUADS);
		glVertex2f(pos1.x, pos1.y);
		glVertex2f(pos1.x, pos2.y);
		glVertex2f(pos2.x, pos2.y);
		glVertex2f(pos2.x, pos1.y);
	glEnd();
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void RenderManagerGL2D::drawBlob(const Vector2& pos, const Color& col)
{
	glLoadIdentity();
	glTranslatef(pos.x, pos.y, 0.6);
	glBindTexture(GL_TEXTURE_2D, mBlob[0]);
	glColor3ubv(col.val);
	drawQuad(128.0, 128.0);

	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, mBlobSpecular[0]);
	drawQuad(128.0, 128.0);
	glDisable(GL_BLEND);
}

void RenderManagerGL2D::refresh()
{
	SDL_GL_SwapBuffers();
}

#else

RenderManager* RenderManager::createRenderManagerGL2D()
{
	std::cerr << "OpenGL not available! Falling back to SDL renderer" <<
		std::endl;
	return RenderManager::createRenderManagerSDL();
}
        

#endif
