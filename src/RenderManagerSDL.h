#pragma once

#include <SDL/SDL.h>
#include <vector>

#include "RenderManager.h"

class RenderManagerSDL : public RenderManager
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
	
	SDL_Surface *mOverlaySurface;
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

	SDL_Surface* colorSurface(SDL_Surface *surface, Color color);
	
public:
	RenderManagerSDL();

	virtual void init(int xResolution, int yResolution, bool fullscreen);
	virtual void deinit();
	virtual void drawBlob(int x, int y, Color col);
	virtual void draw();
	virtual void refresh();

	virtual bool setBackground(const std::string& filename);
	virtual void setBlobColor(int player, Color color);
	
	virtual void setBall(const Vector2& position, float rotation);
	virtual void setBlob(int player, const Vector2& position,
			float animationState);
		
	virtual void setScore(int leftScore, int rightScore,
		       bool leftWarning, bool rightWarning);
		       
	virtual void setMouseMarker(float position);
			       
	virtual void drawText(const std::string& text, Vector2 position, bool highlight);
	virtual void drawImage(const std::string& filename, Vector2 position);
	virtual void drawOverlay(float opacity, Vector2 pos1, Vector2 pos2);
};

