#pragma once

#include <iostream>
#include <list>
#include <map>
#include <SDL/SDL.h>

#include "Vector.h"
#include "Global.h"

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
	RenderManager();
	// Returns -1 on EOF
	// Returns index for ? on unknown char
	int getNextFontIndex(std::string& string);
	SDL_Surface* highlightSurface(SDL_Surface* surface, int luminance);
	SDL_Surface* loadSurface(std::string filename);
	
	Vector2 blobShadowPosition(const Vector2& position);
	Vector2 ballShadowPosition(const Vector2& position);
	
	SDL_Rect blobRect(const Vector2& position);
	SDL_Rect blobShadowRect(const Vector2& position);
	SDL_Rect ballRect(const Vector2& position);
	SDL_Rect ballShadowRect(const Vector2& position);
	
	bool mDrawGame;
	
	std::map<std::string, BufferedImage*> mImageMap;

	float mMouseMarkerPosition;
	bool mNeedRedraw;
	
public:
	virtual ~RenderManager(){};

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
	virtual void refresh() {};

	// Init with the desired Resolution.
	// Note: It is not guaranteed that this resolution will be selected
	virtual void init(int xResolution, int yResolution,
		bool fullscreen) {};

	// Frees all internal data
	virtual void deinit() {};

	// Set a background image by filename
	// Note: There is a default, you dont need to do this
	// Returns true on success
	virtual bool setBackground(const std::string& filename) { return true; };

	// Colors the standard blob image, which are red and green by default
	virtual void setBlobColor(int player, Color color) {};
	
	// Takes the new balls position and its rotation in radians
	virtual void setBall(const Vector2& position, float rotation) {};

	// Takes the new position and the animation state as a float,
	// because some renderers may interpolate the animation
	virtual void setBlob(int player, const Vector2& position, 
			float animationState) {};

	virtual void setMouseMarker(float position);
	
	// Set the displayed score values and the serve notifications
	virtual void setScore(int leftScore, int rightScore,
		bool leftWarning, bool rightWarning) {};
		
	// This simply draws the given text with its top left corner at the
	// given position and doesn't care about line feeds.
	virtual void drawText(const std::string& text, Vector2 position, bool highlight) {};
	
	// This loads and draws an image by name
	// The according Surface is automatically colorkeyed
	// The image is centered around position
	virtual void drawImage(const std::string& filename, Vector2 position) {};
	
	// This draws a greyed-out area
	virtual void drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col = Color(0,0,0)) {}
	
	//Draws a blob 
	virtual void drawBlob(const Vector2& pos, const Color& col){};

	// This forces a redraw of the background, for example
	// when the windows was minimized
	void redraw();
	
	
	// This can disable the rendering of ingame graphics, for example for
	// the main menu
	void drawGame(bool draw);
};
