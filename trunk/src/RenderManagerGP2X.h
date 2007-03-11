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

class RenderManagerGP2X : public RenderManager
{
	SDL_Surface* mBackground;
	SDL_Surface* mBallShadow;

	std::vector<SDL_Surface*> mBall;
	std::vector<SDL_Surface*> mStandardBlob;
	std::vector<SDL_Surface*> mStandardBlobShadow;
	std::vector<SDL_Surface*> mLeftBlob;
	std::vector<SDL_Surface*> mLeftBlobShadow;
	std::vector<SDL_Surface*> mRightBlob;
	std::vector<SDL_Surface*> mRightBlobShadow;

	std::vector<SDL_Surface*> mFont;
	std::vector<SDL_Surface*> mHighlightFont;

	SDL_Surface *mScreen;

	Vector2 mBallPosition;
	float mBallRotation;
	Vector2 mLeftBlobPosition;
	float mLeftBlobAnimationState;
	Vector2 mRightBlobPosition;
	float mRightBlobAnimationState;

	int mLeftPlayerScore;
	int mRightPlayerScore;
	bool mLeftPlayerWarning;
	bool mRightPlayerWarning;

	std::string mLeftPlayerName;
	std::string mRightPlayerName;

	SDL_Surface* colorSurface(SDL_Surface *surface, Color color);

public:
	RenderManagerGP2X();

	virtual void init(int xResolution, int yResolution, bool fullscreen);
	virtual void deinit();
	virtual void draw();
	virtual void refresh();

	virtual bool setBackground(const std::string& filename);
	virtual void setBlobColor(int player, Color color);

	virtual void setBall(const Vector2& position, float rotation);
	virtual void setBlob(int player, const Vector2& position,
			float animationState);

	virtual void setScore(int leftScore, int rightScore,
		       bool leftWarning, bool rightWarning);

	virtual void drawText(const std::string& text, Vector2 position, bool highlight);
	virtual void drawImage(const std::string& filename, Vector2 position) {};

};

