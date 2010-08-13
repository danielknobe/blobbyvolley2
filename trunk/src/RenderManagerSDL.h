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

#include <SDL/SDL.h>
#include <vector>

#include "RenderManager.h"

class RenderManagerSDL : public RenderManager
{
	SDL_Surface* mBackground;
	SDL_Surface* mBallShadow;
	SDL_Surface* mScroll;
	
	std::vector<SDL_Surface*> mBall;
	std::vector<SDL_Surface*> mStandardBlob;
	std::vector<SDL_Surface*> mStandardBlobShadow;
	SDL_Surface*			  mStandardBlobBlood;
	std::vector<SDL_Surface*> mLeftBlob;
	std::vector<SDL_Surface*> mLeftBlobShadow;
	SDL_Surface*			  mLeftBlobBlood;
	std::vector<SDL_Surface*> mRightBlob;
	std::vector<SDL_Surface*> mRightBlobShadow;
	SDL_Surface*			  mRightBlobBlood;

	std::vector<SDL_Surface*> mFont;
	std::vector<SDL_Surface*> mHighlightFont;
	std::vector<SDL_Surface*> mSmallFont;
	std::vector<SDL_Surface*> mHighlightSmallFont;
	
	SDL_Surface *mOverlaySurface;
	SDL_Surface *mScreen;

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
	
	// Store color for caching
	Color mBlobColor[MAX_PLAYERS];

	// colors a surface
	// the returned SDL_Surface* is already converted into DisplayFormat
	SDL_Surface* colorSurface(SDL_Surface *surface, Color color);
	
public:
	RenderManagerSDL();

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

	virtual void setMouseMarker(float position);
			       
	virtual void drawText(const std::string& text, Vector2 position, unsigned int flags = TF_NORMAL);
	virtual void drawImage(const std::string& filename, Vector2 position);
	virtual void drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col);
	virtual void drawBlob(const Vector2& pos, const Color& col);
	virtual void drawParticle(const Vector2& pos, int player);
};

