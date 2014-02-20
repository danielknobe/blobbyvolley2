/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

#include <vector>
#include <string>

#include "Vector.h"
#include "Global.h"

#include "InputManager.h"
#include "RenderManager.h"
#include "TextManager.h" /// needed because we can't forward declare that enum
#include "BlobbyDebug.h"

// Warning: This may explode if we use the GUI from several files

#define GEN_ID (__LINE__)

enum KeyAction
{
	UP,
	DOWN,
	LEFT,
	RIGHT,
	SELECT,
	BACK,
	NONE
};

enum SelectBoxAction
{
	SBA_NONE = 0,
	SBA_SELECT,
	SBA_DBL_CLICK
};

/*! \class IMGUI
	\brief GUI Manager
	\details This class manages drawing and input handling of the blobby GUI.
			It is poorly designed, does not use OOP and makes extension difficult, so
			it needs a complete rewrite.
*/
class IMGUI : public ObjectCounter<IMGUI>
{
	public:
		static IMGUI& getSingleton();

		void begin();
		void end();
		void resetSelection();

		void doImage(int id, const Vector2& position, const std::string& name);
		void doText(int id, const Vector2& position, const std::string& text, unsigned int flags = TF_NORMAL);
		void doText(int id, const Vector2& position, TextManager::STRING text, unsigned int flags = TF_NORMAL);
		void doOverlay(int id, const Vector2& pos1, const Vector2& pos2, const Color& col = Color(0, 0, 0), float alpha = 0.65);
		void doCursor(bool draw = true) { mDrawCursor = draw; 	mUsingCursor = true; }

		bool doButton(int id, const Vector2& position, const std::string& text, unsigned int flags = TF_NORMAL);
		bool doButton(int id, const Vector2& position, TextManager::STRING text, unsigned int flags = TF_NORMAL);
		bool doScrollbar(int id, const Vector2& position, float& value);
		bool doEditbox(int id, const Vector2& position, unsigned length, std::string& text, unsigned& cpos, unsigned int flags = TF_NORMAL, bool force_active = false);
		SelectBoxAction doSelectbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, unsigned& selected, unsigned int flags = TF_NORMAL);
		void doChatbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, unsigned& selected, const std::vector<bool>& local, unsigned int flags = TF_NORMAL);
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

