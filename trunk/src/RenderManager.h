#pragma once

#include <iostream>
#include <list>
#include <SDL/SDL.h>

#include "Vector.h"

struct Color
{
	Color(int red, int green, int blue)
		: r(red), g(green), b(blue) {}
	Color() {}
	union
	{
		struct
		{
			Uint8 r;
			Uint8 g;
			Uint8 b;
		};
		Uint8 val[3];
	};
};

// This rendering class reduces all drawing stuff to a few calls
// to refresh the objects states. It also abstracts from specific
// graphics APIs.
// The following implementations are planned (ordered by importance)
//
// RenderManagerSDL:
// 	Uses standard SDL blits for drawing. It depends on precomputed
// 	rotated sprites and colors the blobs manually.
// 	Its fixed to the traditional resolution 800x600.
// RenderManagerGL2D:
// 	This manager relies on OpenGL to accelerate 2D drawing on systems
// 	like Linux/X11 where SDL acceleration is difficult. It rotates and
// 	colors its sprites in realtime, but still uses 2D graphics.
// RenderManagerGL3D:
// 	The GL3D is the top-end RenderManager. It uses newly created meshes
// 	and therefore supports vertex morphing for the blobs. It makes use 
// 	of OpenGL to present special effects like per-pixel-lighting, 
// 	stencil shadows, motion blur, and much more. It will requiere
// 	OpenGL 2.0 compliant graphics hardware.
// RenderManagerGP2X:
// 	This manager is used to port Blobby Volley to the GP2X handheld.
// 	It makes use of a fixed resolution at 320x240 and others for TV-Out.
// 	It also uses highly optimised loading routines with raw image data.
// 	In all other terms its similar to the RenderManagerSDL

class RenderManager
{

private:
	static RenderManager *mSingleton;

protected:
	RenderManager()
	{
		assert(!mSingleton);
		mSingleton = this;
	}
	
public:
	static RenderManager* createRenderManagerSDL();
	static RenderManager* createRenderManagerGP2X();
	static RenderManager* createRenderManagerGL2D();

	static RenderManager& getSingleton()
	{
		return *mSingleton;
	}

	// Draws the stuff
	virtual void draw() = 0;
	
	// This swaps the screen buffers and should be called
	// after all draw calls
	virtual void refresh() = 0;

	// Init with the desired Resolution.
	// Note: It is not guaranteed that this resolution will be selected
	virtual void init(int xResolution, int yResolution,
		bool fullscreen) = 0;

	// Frees all internal data
	virtual void deinit() = 0;

	// Set a background image by filename
	// Note: There is a default, you dont need to do this
	// Returns true on success
	virtual bool setBackground(const std::string& filename) = 0;

	// Colors the standard blob image, which are red and green by default
	virtual void setBlobColor(int player, Color color) = 0;
	
	// Takes the new balls position and its rotation in radians
	virtual void setBall(const Vector2& position, float rotation) = 0;

	// Takes the new position and the animation state as a float,
	// because some renderers may interpolate the animation
	virtual void setBlob(int player, const Vector2& position, 
			float animationState) = 0;
	
	// Set the displayed score values and the serve notifications
	virtual void setScore(int leftScore, int rightScore,
		bool leftWarning, bool rightWarning) = 0;
		
	// This simply draws the given text with its top left corner at the
	// given position and doesn't care about line feeds.
	virtual void drawText(const std::string& text, Vector2 position) = 0;
};
