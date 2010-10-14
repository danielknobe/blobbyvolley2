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

#include <iostream>
#include <vector>
#include <string>

#include "Vector.h"
#include "Global.h"

#include "InputManager.h"
#include "RenderManager.h"

// Warning: This may explode if we use the GUI from several files

#define GEN_ID (__LINE__)

enum KeyAction
{
	UP,
	DOWN,
	LEFT,
	RIGHT,
	SELECT,
	NONE
};

class IMGUI
{
public:
	static IMGUI& getSingleton();
	
	void begin();
	void end();
	void resetSelection();
	
	void doImage(int id, const Vector2& position, const std::string& name);
	void doText(int id, const Vector2& position, const std::string& text, unsigned int flags = TF_NORMAL);
	void doOverlay(int id, const Vector2& pos1, const Vector2& pos2, const Color& col = Color(0, 0, 0));
	void doCursor(bool draw = true) { mDrawCursor = draw; 	mUsingCursor = true; }

	bool doButton(int id, const Vector2& position, const std::string& text, unsigned int flags = TF_NORMAL);
	bool doScrollbar(int id, const Vector2& position, float& value);
	bool doEditbox(int id, const Vector2& position, int length, std::string& text, unsigned& cpos, unsigned int flags = TF_NORMAL);
	bool doSelectbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, int& selected, unsigned int flags = TF_NORMAL);
	bool doBlob(int id, const Vector2& position, const Color& col);

	bool usingCursor() const;
	void doInactiveMode(bool inactive) { mInactive = inactive; }
private:
	IMGUI();
	~IMGUI();

	static IMGUI* mSingleton;

	int mActiveButton;
	int mHeldWidget;
	KeyAction mLastKeyAction;
	int mLastWidget;
	bool mDrawCursor;
	bool mButtonReset;
	bool mInactive;
	bool mUsingCursor;
};

