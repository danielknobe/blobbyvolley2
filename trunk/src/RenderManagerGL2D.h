#pragma once

#ifdef HAVE_LIBGL

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <vector>

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

	Color mLeftBlobColor;
	Color mRightBlobColor;

	void drawQuad(float x, float y);
	GLuint loadTexture(const std::string& filename, bool specular);
	int getNextPOT(int npot);
public:
	RenderManagerGL2D();

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
		       
	virtual void drawText(const std::string& text, Vector2 position);
};

#endif
