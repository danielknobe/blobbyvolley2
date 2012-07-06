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

#include <list>
#include <map>
#include <SDL/SDL.h>

#include "Vector.h"
#include "Global.h"


// Text definitions
#define FONT_WIDTH_NORMAL	24	// Height and width of the normal font.
#define LINE_SPACER_NORMAL	6	// Extra space between 2 lines in a normal SelectBox.
#define FONT_WIDTH_SMALL	8	// Height and width of the small font.
#define LINE_SPACER_SMALL	2	// Extra space between 2 lines in a small SelectBox.

#define FONT_INDEX_ASTERISK	36 // M.W. : Currently a dot because there is no asterisk yet.


// Text flags (usable for the RenderManager::drawText() flag parameter)
// Just using one byte for now - up to 8 flags.
#define TF_NORMAL		0x00 // 0 == false (backward compatibility for state modules)
#define TF_HIGHLIGHT	0x01 // 1 == true (backward compatibility for state modules)
#define TF_SMALL_FONT	0x02 // Draw a smaller font. (8px instead of 24px)
#define TF_OBFUSCATE	0x04 // Obfuscate the text with asterisks. (for password Editboxes)

// Text Alignment Flags
#define TF_ALIGN_LEFT		0x00	// Text aligned to the left (default)
#define TF_ALIGN_CENTER		0x08	// Text centered
#define	TF_ALIGN_RIGHT		0x10	// Text aligned right		



struct SDL_Surface;
/*! \struct BufferedImage
	\brief image data
	\details couples the raw image data with its size in a way that is
			independend of the used renderer.
*/
struct BufferedImage
{
	int w;
	int h;
	union
	{
		SDL_Surface* sdlImage;
		unsigned glHandle;
	};
};


/*! \class RenderManager
	\brief class for managing rendering
	\details 
	This rendering class reduces all drawing stuff to a few calls
	to refresh the objects states. It also abstracts from specific
	graphics APIs.
	The following implementations are planned (ordered by importance)
	
	RenderManagerSDL:
	 Uses standard SDL blits for drawing. It depends on precomputed
	 rotated sprites and colors the blobs manually.
	 Its fixed to the traditional resolution 800x600.
	RenderManagerGL2D:
	 This manager relies on OpenGL to accelerate 2D drawing on systems
	 like Linux/X11 where SDL acceleration is difficult. It rotates and
	 colors its sprites in realtime, but still uses 2D graphics.
	RenderManagerGL3D:
     The GL3D is the top-end RenderManager. It uses newly created meshes
     and therefore supports vertex morphing for the blobs. It makes use 
     of OpenGL to present special effects like per-pixel-lighting, 
     stencil shadows, motion blur, and much more. It will requiere
     OpenGL 2.0 compliant graphics hardware.
	RenderManagerGP2X:
	 This manager is used to port Blobby Volley to the GP2X handheld.
	 It makes use of a fixed resolution at 320x240 and others for TV-Out.
	 It also uses highly optimised loading routines with raw image data.
	 In all other terms its similar to the RenderManagerSDL

	\todo This classes need a complete rework! They include far too much information about the actual game.
*/
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
	SDL_Surface* createEmptySurface(unsigned int width, unsigned int height);
	
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
	static RenderManager* createRenderManagerNull();

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

	virtual void showShadow(bool shadow) {};
	
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

	// Set the names
	virtual void setPlayernames(std::string leftName, std::string rightName) {};
	
	// Set the time
	virtual void setTime(const std::string& time) {};

	// This simply draws the given text with its top left corner at the
	// given position and doesn't care about line feeds.
	virtual void drawText(const std::string& text, Vector2 position, unsigned int flags = TF_NORMAL) {};
	
	// This loads and draws an image by name
	// The according Surface is automatically colorkeyed
	// The image is centered around position
	virtual void drawImage(const std::string& filename, Vector2 position) {};
	
	// This draws a greyed-out area
	virtual void drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col = Color(0,0,0)) {}
	
	//Draws a blob 
	virtual void drawBlob(const Vector2& pos, const Color& col){};

	// Enables particle drawing
	virtual void startDrawParticles() {};
	//Draw blood particle
	virtual void drawParticle(const Vector2& pos, int player){};
	// Finishes drawing particles
	virtual void endDrawParticles() {};

	// This forces a redraw of the background, for example
	// when the windows was minimized
	void redraw();
	
	// This can disable the rendering of ingame graphics, for example for
	// the main menu
	void drawGame(bool draw);
	
	// This function may be useful for displaying framerates
	void setTitle(const std::string& title);
};
