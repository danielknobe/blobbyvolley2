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

#pragma once

#if HAVE_LIBGL

#include <SDL/SDL.h>


#if defined(WIN32)
#include <windows.h>
#endif

#if __MACOSX__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <vector>
#include <list>

#include "RenderManager.h"

class RenderManagerGL2D : public RenderManager
{
private:
	GLuint mBackground;
	GLuint mBallShadow;

	std::vector<GLuint> mBall;
	std::vector<GLuint> mBlob;
	std::vector<GLuint> mBlobSpecular;
	std::vector<GLuint> mBlobShadow;
	std::vector<GLuint> mFont;
	std::vector<GLuint> mHighlightFont;
	std::vector<GLuint> mSmallFont;
	std::vector<GLuint> mHighlightSmallFont;
	GLuint mParticle;
	GLuint mScroll;
	
	std::list<Vector2> mLastBallStates;

	Vector2 mBallPosition;
	float mBallRotation;
	Vector2 mLeftBlobPosition;
	float mLeftBlobAnimationState;
	Vector2 mRightBlobPosition;
	float mRightBlobAnimationState;

	bool mShowShadow;

	int mLeftPlayerScore;
	int mRightPlayerScore;
	bool mLeftPlayerWarning;
	bool mRightPlayerWarning;

	std::string mLeftPlayerName;
	std::string mRightPlayerName;
	std::string mTime;

	Color mLeftBlobColor;
	Color mRightBlobColor;

	void drawQuad(float x, float y);
	void drawQuad2(float x, float y, float width, float height);
	GLuint loadTexture(SDL_Surface* surface, bool specular);
	int getNextPOT(int npot);
	
	void glEnable(unsigned int flag);
	void glDisable(unsigned int flag);
	void glBindTexture(GLuint texture);
	
	GLuint mCurrentTexture;
public:
	RenderManagerGL2D();

	virtual void init(int xResolution, int yResolution, bool fullscreen);
	virtual void deinit();
	virtual void draw();
	virtual void refresh();

	virtual bool setBackground(const std::string& filename);
	virtual void setBlobColor(int player, Color color);
	virtual void showShadow(bool shadow);
	
	virtual void setBall(const Vector2& position, float rotation);
	virtual void setBlob(int player, const Vector2& position,
			float animationState);
		
	virtual void setScore(int leftScore, int rightScore,
		       bool leftWarning, bool rightWarning);

	virtual void setPlayernames(std::string leftName, std::string rightName);
	virtual void setTime(const std::string& t);

	virtual void drawText(const std::string& text, Vector2 position, unsigned int flags = TF_NORMAL);
	virtual void drawImage(const std::string& filename, Vector2 position);
	virtual void drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col);
	virtual void drawBlob(const Vector2& pos, const Color& col);
	virtual void startDrawParticles();
	virtual void drawParticle(const Vector2& pos, int player);
	virtual void endDrawParticles();
};


#endif
