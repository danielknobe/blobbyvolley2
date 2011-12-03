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

#include <queue>
#include <cassert>

#include <SDL/SDL.h>

#include "IMGUI.h"

enum ObjectType
{
	IMAGE,
	OVERLAY,
	TEXT,
	BUTTON, // Unused!
	SCROLLBAR,
	ACTIVESCROLLBAR,
	EDITBOX,
	ACTIVEEDITBOX,
	SELECTBOX,
	ACTIVESELECTBOX,
	BLOB,
	CHAT
};

struct QueueObject
{
	ObjectType type;
	int id;
	Vector2 pos1;
	Vector2 pos2;
	Color col;
	std::string text;
	std::vector<std::string> entries;
	int selected;
	int length;
	unsigned int flags;
};

typedef std::queue<QueueObject> RenderQueue;

IMGUI* IMGUI::mSingleton = 0;
RenderQueue *mQueue;

IMGUI::IMGUI()
{
	mQueue = new RenderQueue;
	mActiveButton = -1;
	mHeldWidget = 0;
	mLastKeyAction = NONE;
	mLastWidget = 0;
	mButtonReset = false;
	mInactive = false;
}

IMGUI::~IMGUI()
{
	delete mQueue;
}

IMGUI& IMGUI::getSingleton()
{
	if (!mSingleton)
		mSingleton = new IMGUI;
	return *mSingleton;
}

void IMGUI::begin()
{
	mUsingCursor = false;
	mButtonReset = false;
	while (!mQueue->empty())
		mQueue->pop();
	
	mLastKeyAction = NONE;
	if (InputManager::getSingleton()->up())
		mLastKeyAction = UP;
	if (InputManager::getSingleton()->down())
		mLastKeyAction = DOWN;
	if (InputManager::getSingleton()->left())
		mLastKeyAction = LEFT;
	if (InputManager::getSingleton()->right())
		mLastKeyAction = RIGHT;
	if (InputManager::getSingleton()->select())
		mLastKeyAction = SELECT;
}

void IMGUI::end()
{
	int FontSize;
	RenderManager& rmanager = RenderManager::getSingleton();
	while (!mQueue->empty())
	{
		QueueObject& obj = mQueue->front();
		switch (obj.type)
		{
			case IMAGE:
				rmanager.drawImage(obj.text, obj.pos1);
				break;
			case OVERLAY:
				rmanager.drawOverlay(0.6, obj.pos1, obj.pos2, obj.col);
				break;
			case TEXT:
				rmanager.drawText(obj.text, obj.pos1, obj.flags);
				break;
			case SCROLLBAR:
				rmanager.drawOverlay(0.4, obj.pos1, obj.pos1 + Vector2(210.0, 26.0));
				rmanager.drawImage("gfx/scrollbar.bmp",obj.pos1 + Vector2(obj.pos2.x * 200.0 + 5 , 13));
				break;
			case ACTIVESCROLLBAR:
				rmanager.drawOverlay(0.3, obj.pos1, obj.pos1 + Vector2(210.0, 26.0));
				rmanager.drawImage("gfx/scrollbar.bmp",obj.pos1 + Vector2(obj.pos2.x * 200.0 + 5 , 13));
				break;
			case EDITBOX:
				FontSize = (obj.flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL);
				rmanager.drawOverlay(0.4, obj.pos1, obj.pos1 + Vector2(10+obj.length*FontSize, 10+FontSize));
				rmanager.drawText(obj.text, obj.pos1+Vector2(5, 5), obj.flags);
				break;
			case ACTIVEEDITBOX:
				FontSize = (obj.flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL);
				rmanager.drawOverlay(0.2, obj.pos1, obj.pos1 + Vector2(10+obj.length*FontSize, 10+FontSize));
				rmanager.drawText(obj.text, obj.pos1+Vector2(5, 5), obj.flags);
				if (obj.pos2.x >= 0)
					rmanager.drawOverlay(1.0, Vector2((obj.pos2.x)*FontSize+obj.pos1.x+5, obj.pos1.y+5), Vector2((obj.pos2.x)*FontSize+obj.pos1.x+5+3, obj.pos1.y+5+FontSize), Color(255,255,255));
				break;
			case SELECTBOX:
				FontSize = (obj.flags & TF_SMALL_FONT ? (FONT_WIDTH_SMALL+LINE_SPACER_SMALL) : (FONT_WIDTH_NORMAL+LINE_SPACER_NORMAL));
				rmanager.drawOverlay(0.4, obj.pos1, obj.pos2);
				for (unsigned int c = 0; c < obj.entries.size(); c++)
				{
					if(c == obj.selected)
						rmanager.drawText(obj.entries[c], Vector2(obj.pos1.x+5, obj.pos1.y+(c*FontSize)+5), obj.flags | TF_HIGHLIGHT);
					else
						rmanager.drawText(obj.entries[c], Vector2(obj.pos1.x+5, obj.pos1.y+(c*FontSize)+5), obj.flags);
				}
				break;
			case ACTIVESELECTBOX:
				FontSize = (obj.flags & TF_SMALL_FONT ? (FONT_WIDTH_SMALL+LINE_SPACER_SMALL) : (FONT_WIDTH_NORMAL+LINE_SPACER_NORMAL));
				rmanager.drawOverlay(0.2, obj.pos1, obj.pos2);
				for (unsigned int c = 0; c < obj.entries.size(); c++)
				{
					if(c == obj.selected)
						rmanager.drawText(obj.entries[c], Vector2(obj.pos1.x+5, obj.pos1.y+(c*FontSize)+5), obj.flags | TF_HIGHLIGHT);
					else
						rmanager.drawText(obj.entries[c], Vector2(obj.pos1.x+5, obj.pos1.y+(c*FontSize)+5), obj.flags);
				}
				break;
			case CHAT:
				FontSize = (obj.flags & TF_SMALL_FONT ? (FONT_WIDTH_SMALL+LINE_SPACER_SMALL) : (FONT_WIDTH_NORMAL+LINE_SPACER_NORMAL));
				rmanager.drawOverlay(0.4, obj.pos1, obj.pos2);
				for (unsigned int c = 0; c < obj.entries.size(); c++)
				{
					if (obj.text[c] == 'R' )
						rmanager.drawText(obj.entries[c], Vector2(obj.pos1.x+5, obj.pos1.y+(c*FontSize)+5), obj.flags | TF_HIGHLIGHT);
					else
						rmanager.drawText(obj.entries[c], Vector2(obj.pos1.x+5, obj.pos1.y+(c*FontSize)+5), obj.flags);
				}
				break;
			case BLOB:
				rmanager.drawBlob(obj.pos1, obj.col);
				break;
			default:
				break;		
		}
		mQueue->pop();
	}
	if (mDrawCursor)
	{
		rmanager.drawImage("gfx/cursor.bmp", 
			InputManager::getSingleton()->
			position() + Vector2(24.0, 24.0));
		mDrawCursor = false;
	}
}

void IMGUI::doImage(int id, const Vector2& position, const std::string& name)
{
	QueueObject obj;
	obj.type = IMAGE;
	obj.id = id;
	obj.pos1 = position;
	obj.text = name;
	mQueue->push(obj);
}

void IMGUI::doText(int id, const Vector2& position, const std::string& text, unsigned int flags)
{
	QueueObject obj;
	obj.type = TEXT;
	obj.id = id;
	obj.pos1 = position;
	obj.text = text;
	obj.flags = flags;
	mQueue->push(obj);
}

void IMGUI::doOverlay(int id, const Vector2& pos1, const Vector2& pos2, const Color& col)
{
	QueueObject obj;
	obj.type = OVERLAY;
	obj.id = id;
	obj.pos1 = pos1;
	obj.pos2 = pos2;
	obj.col = col;
	mQueue->push(obj);
	RenderManager::getSingleton().redraw();
}

bool IMGUI::doButton(int id, const Vector2& position, const std::string& text, unsigned int flags)
{
	bool clicked = false;
	QueueObject obj;
	obj.id = id;
	obj.pos1 = position;
	obj.text = text;
	obj.type = TEXT;
	obj.flags = flags;
	
	if (!mInactive)
	{
		// M.W. : Activate cursorless object-highlighting once the up or down key is pressed.
		if (mActiveButton == -1)
		{
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
		}
		
		// Highlight first menu object for arrow key navigation.
		if (mActiveButton == 0 && !mButtonReset)
			mActiveButton = id;

		// React to keyboard input.
		if (id == mActiveButton)
		{
			obj.flags = obj.flags | TF_HIGHLIGHT;
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				case SELECT:
					clicked = true;
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
		}

		// React to mouse input.
		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x > position.x &&
			mousepos.y > position.y &&
			mousepos.x < position.x + text.length() * (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL) &&
			mousepos.y < position.y + (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL))
		{
			obj.flags = obj.flags | TF_HIGHLIGHT;
			if (InputManager::getSingleton()->click())
			{
				clicked = true;
				mActiveButton = id;
			}
		}
	}

	mLastWidget = id;
	mQueue->push(obj);
	return clicked;
}

bool IMGUI::doScrollbar(int id, const Vector2& position, float& value)
{
	QueueObject obj;
	obj.id = id;
	obj.pos1 = position;
	obj.type = SCROLLBAR;
	
	bool deselected = false;
	
	if (InputManager::getSingleton()->unclick())
	{
		if (id == mHeldWidget)
			deselected = true;
		mHeldWidget = 0;
	}
	
	if (!mInactive)
	{
		// M.W. : Activate cursorless object-highlighting once the up or down key is pressed.
		if (mActiveButton == -1)
		{
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
		}
		
		// Highlight first menu object for arrow key navigation.
		if (mActiveButton == 0 && !mButtonReset)
			mActiveButton = id;
		
		// React to keyboard input.
		if (id == mActiveButton)
		{
			obj.type = ACTIVESCROLLBAR;
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				case LEFT:
					value -= 0.1;
					mLastKeyAction = NONE;
					break;
				case RIGHT:
					value += 0.1;
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
		}
		
		// React to mouse input.
		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x + 5 > position.x &&
			mousepos.y > position.y &&
			mousepos.x < position.x + 205 &&
			mousepos.y < position.y + 24.0)
		{
			obj.type = ACTIVESCROLLBAR;

			if (InputManager::getSingleton()->click())
			{
				mHeldWidget = id;
			}
			if (mHeldWidget == id)
			{
				value = (mousepos.x - position.x) / 200.0;
				mActiveButton = id;
			}

			if(InputManager::getSingleton()->mouseWheelUp())
				value += 0.1;
			if(InputManager::getSingleton()->mouseWheelDown())
				value -= 0.1;
		}
	}

	value = value > 0.0 ? (value < 1.0 ? value : 1.0) : 0.0;
	obj.pos2.x = value;

	mLastWidget = id;
	mQueue->push(obj);

	return deselected;
}

void IMGUI::resetSelection()
{
	mInactive = false;
	mActiveButton = -1;
	mButtonReset = true;
}

bool IMGUI::doEditbox(int id, const Vector2& position, int length, std::string& text, unsigned& cpos, unsigned int flags, bool force_active)
{
	int FontSize = (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL);
	bool changed = false;
	QueueObject obj;
	obj.id = id;
	obj.pos1 = position;
	obj.type = EDITBOX;
	obj.length = length; // lenght does not actually work!
	obj.flags = flags;

	// React to mouse input.
	Vector2 mousepos = InputManager::getSingleton()->position();
	if (mousepos.x > position.x &&
		mousepos.y > position.y &&
		mousepos.x < position.x + length * FontSize + 10 &&
		mousepos.y < position.y + FontSize + 10)
	{
		obj.flags = obj.flags | TF_HIGHLIGHT;
		if (InputManager::getSingleton()->click())
		{
			// Handle click on the text.
			if (mousepos.x < position.x + text.length() * FontSize)
				cpos = (int) ((mousepos.x-position.x-5+(FontSize/2)) / FontSize);
			// Handle click behind the text.
			else if (mousepos.x < position.x + length * FontSize + 10)
				cpos = (int) text.length();
			mActiveButton = id;
		}
	}

	if (!mInactive)
	{
		// M.W. : Activate cursorless object-highlighting once the up or down key is pressed.
		if (mActiveButton == -1)
		{
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
			
			// M.W. : Initialize the cursor position at the end of the string.
			// IMPORTANT: If you make changes to EditBox text that alter the length
			//				of the text, either call resetSelection() to come back
			//				to this area of code or update cpos manually to prevent
			//				crashes due to a misplaced cursor.
			cpos = text.length();
		}
		
		// Highlight first menu object for arrow key navigation.
		if (mActiveButton == 0 && !mButtonReset)
			mActiveButton = id;
		
		// React to keyboard input.
		if (id == mActiveButton || force_active)
		{
			obj.type = ACTIVEEDITBOX;
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				case LEFT:
					if (cpos > 0)
						cpos--;
					mLastKeyAction = NONE;
					break;
				case RIGHT:
					if (cpos < text.length())
						cpos++;
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
			std::string input = InputManager::getSingleton()->getLastTextKey();
			if (input == "backspace" && text.length() > 0 && cpos > 0)
			{
				text.erase(cpos - 1, 1);
				cpos--;
			}
			else if (input == "del" && text.length() > cpos)
			{
				text.erase(cpos, 1);
			}
			else if (input == "return")
			{
				// Workarround for chatwindow! Delete this after GUI-Rework
				changed = true;
			}
			// This is a temporary solution until the new
			// UTF-8 class can tell the real length!!!
			else if (text.length() < length)
			{
				if (input == "space")
				{
					text.insert(cpos, " ");
					cpos++;
					changed = true;
				}
				if (input == "keypad0")
				{
					text.insert(cpos, "0");
					cpos++;
					changed = true;
				}
				else if (input == "keypad1")
				{
					text.insert(cpos, "1");
					cpos++;
					changed = true;
				}
				else if (input == "keypad2")
				{
					text.insert(cpos, "2");
					cpos++;
					changed = true;
				}
				else if (input == "keypad3")
				{
					text.insert(cpos, "3");
					cpos++;
					changed = true;
				}
				else if (input == "keypad4")
				{
					text.insert(cpos, "4");
					cpos++;
					changed = true;
				}
				else if (input == "keypad5")
				{
					text.insert(cpos, "5");
					cpos++;
					changed = true;
				}
				else if (input == "keypad6")
				{
					text.insert(cpos, "6");
					cpos++;
					changed = true;
				}
				else if (input == "keypad7")
				{
					text.insert(cpos, "7");
					cpos++;
					changed = true;
				}
				else if (input == "keypad8")
				{
					text.insert(cpos, "8");
					cpos++;
					changed = true;
				}
				else if (input == "keypad9")
				{
					text.insert(cpos, "9");
					cpos++;
					changed = true;
				}
				else if (input.length() == 1)
				{
					text.insert(cpos, input);
					cpos++;
					changed = true;
				}
			}
		}
	}

	obj.pos2.x = SDL_GetTicks() % 1000 >= 500 ? cpos : -1.0;
	obj.text = text;

	mLastWidget = id;
	mQueue->push(obj);

	// when content changed, it is active
	// part of chat window hack
	if( changed && force_active )
		mActiveButton = id;

	return changed;
}

SelectBoxAction IMGUI::doSelectbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, int& selected, unsigned int flags)
{
	int FontSize = (flags & TF_SMALL_FONT ? (FONT_WIDTH_SMALL+LINE_SPACER_SMALL) : (FONT_WIDTH_NORMAL+LINE_SPACER_NORMAL));
	SelectBoxAction changed = SBA_NONE;
	QueueObject obj;
	obj.id = id;
	obj.pos1 = pos1;
	obj.pos2 = pos2;
	obj.type = SELECTBOX;
	obj.flags = flags;

	const int itemsPerPage = int(pos2.y - pos1.y - 10) / FontSize;
	int first = (int)(selected / itemsPerPage)*itemsPerPage; //the first visible element in the list

	if (!mInactive)
	{
		// M.W. : Activate cursorless object-highlighting once the up or down key is pressed.
		if (mActiveButton == -1)
		{
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
		}
		
		// Highlight first menu object for arrow key navigation.
		if (mActiveButton == 0 && !mButtonReset)
			mActiveButton = id;
		
		// React to keyboard input.
		if (id == mActiveButton)
		{
			obj.type = ACTIVESELECTBOX;
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				case LEFT:
					if (selected > 0)
					{
						selected--;
						changed = SBA_SELECT;
					}
					mLastKeyAction = NONE;
					break;
				case RIGHT:
					if (selected < entries.size()-1)
					{
						selected++;
						changed = SBA_SELECT;
					}
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
		}
		
		// React to mouse input.
		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x > pos1.x && mousepos.y > pos1.y && mousepos.x < pos2.x && mousepos.y < pos2.y)
		{
			obj.type = ACTIVESELECTBOX;
			if (InputManager::getSingleton()->click())
				mActiveButton = id;
		}
		//entries mouseclick:
		if (mousepos.x > pos1.x &&
			mousepos.y > pos1.y+5 &&
			mousepos.x < pos2.x-35 &&
			mousepos.y < pos1.y+5+FontSize*itemsPerPage)
		{
			if (InputManager::getSingleton()->click())
			{
				int tmp = (int)((mousepos.y-pos1.y-5) / FontSize)+first;
				/// \todo well, it's not really a doulbe click...
				/// we need to do this in inputmanager
				if( selected == tmp && InputManager::getSingleton()->doubleClick() )
					changed = SBA_DBL_CLICK;
				
				if (tmp < entries.size())
					selected = tmp;
				mActiveButton = id;
			}
			if ((InputManager::getSingleton()->mouseWheelUp()) && (selected > 0))
			{
				selected--;
				changed = SBA_SELECT;
			}
			if ((InputManager::getSingleton()->mouseWheelDown()) && (selected < entries.size()-1))
			{
				selected++;
				changed = SBA_SELECT;
			}
		}
		//arrows mouseclick:
		if (mousepos.x > pos2.x-30 && mousepos.x < pos2.x-30+24 && InputManager::getSingleton()->click())
		{
			if (mousepos.y > pos1.y+3 && mousepos.y < pos1.y+3+24 && selected > 0)
			{
				selected--;
				changed = SBA_SELECT;
			}
			if (mousepos.y > pos2.y-27 && mousepos.y < pos2.y-27+24 && selected < entries.size()-1)
			{
				selected++;
				changed = SBA_SELECT;
			}
		}
	}
	doImage(GEN_ID, Vector2(pos2.x-15, pos1.y+15), "gfx/pfeil_oben.bmp");
	doImage(GEN_ID, Vector2(pos2.x-15, pos2.y-15), "gfx/pfeil_unten.bmp");

	first = (selected / itemsPerPage)*itemsPerPage; //recalc first
	if ( !entries.empty() )
	{
		int last = first + itemsPerPage;
		if (last > entries.size())
			last = entries.size();
		obj.entries = std::vector<std::string>(entries.begin()+first, entries.begin()+last);
	}
	else
		obj.entries = std::vector<std::string>();
	obj.selected = selected-first;

	mLastWidget = id;
	mQueue->push(obj);

	return changed;
}

void IMGUI::doChatbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, int& selected, const std::vector<bool>& local, unsigned int flags)
{
	assert( entries.size() == local.size() );
	int FontSize = (flags & TF_SMALL_FONT ? (FONT_WIDTH_SMALL+LINE_SPACER_SMALL) : (FONT_WIDTH_NORMAL+LINE_SPACER_NORMAL));
	SelectBoxAction changed = SBA_NONE;
	QueueObject obj;
	obj.id = id;
	obj.pos1 = pos1;
	obj.pos2 = pos2;
	obj.type = CHAT;
	obj.flags = flags;

	const int itemsPerPage = int(pos2.y - pos1.y - 10) / FontSize;
	int first = selected; //the first visible element in the list

	if (!mInactive)
	{
		// M.W. : Activate cursorless object-highlighting once the up or down key is pressed.
		if (mActiveButton == -1)
		{
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
		}
		
		// Highlight first menu object for arrow key navigation.
		if (mActiveButton == 0 && !mButtonReset)
			mActiveButton = id;
		
		// React to keyboard input.
		if (id == mActiveButton)
		{
			switch (mLastKeyAction)
			{
				case DOWN:
					mActiveButton = 0;
					mLastKeyAction = NONE;
					break;
				case UP:
					mActiveButton = mLastWidget;
					mLastKeyAction = NONE;
					break;
				case LEFT:
					if (selected > 0)
					{
						selected--;
					}
					mLastKeyAction = NONE;
					break;
				case RIGHT:
					if (selected < entries.size()-1)
					{
						selected++;
					}
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
		}
		
		// React to mouse input.
		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x > pos1.x && mousepos.y > pos1.y && mousepos.x < pos2.x && mousepos.y < pos2.y)
		{
			if (InputManager::getSingleton()->click())
				mActiveButton = id;
		}
		//entries mouseclick:
		if (mousepos.x > pos1.x &&
			mousepos.y > pos1.y+5 &&
			mousepos.x < pos2.x-35 &&
			mousepos.y < pos1.y+5+FontSize*itemsPerPage)
		{
			if ((InputManager::getSingleton()->mouseWheelUp()) && (selected > 0))
			{
				selected--;
			}
			if ((InputManager::getSingleton()->mouseWheelDown()) && (selected < entries.size()-1))
			{
				selected++;
			}
		}
		//arrows mouseclick:
		if (mousepos.x > pos2.x-30 && mousepos.x < pos2.x-30+24 && InputManager::getSingleton()->click())
		{
			if (mousepos.y > pos1.y+3 && mousepos.y < pos1.y+3+24 && selected > 0)
			{
				selected--;
			}
			if (mousepos.y > pos2.y-27 && mousepos.y < pos2.y-27+24 && selected < entries.size()-1)
			{
				selected++;
			}
		}
	}
	doImage(GEN_ID, Vector2(pos2.x-15, pos1.y+15), "gfx/pfeil_oben.bmp");
	doImage(GEN_ID, Vector2(pos2.x-15, pos2.y-15), "gfx/pfeil_unten.bmp");

	first = (selected / itemsPerPage)*itemsPerPage; //recalc first
	if ( !entries.empty() )
	{
		int last = selected + 1;
		first = last - itemsPerPage;
		if (first < 0)
			first = 0;
		obj.entries = std::vector<std::string>(entries.begin()+first, entries.begin()+last);
		// HACK: we use taxt to store information which text is from local player and which from
		//			remote player.
		obj.text = "";
		for(int i = first; i < last; ++i)
		{
			obj.text += local[i] ? 'L' : 'R';
		}
	}
	else
		obj.entries = std::vector<std::string>();
	obj.selected = selected-first;

	mLastWidget = id;
	mQueue->push(obj);
}


bool IMGUI::doBlob(int id, const Vector2& position, const Color& col)
{
	QueueObject obj;
	obj.id = id;
	obj.pos1 = position;
	obj.type = BLOB;
	obj.col = col;
	mQueue->push(obj);
	return false;
}

bool IMGUI::usingCursor() const
{
	return mUsingCursor;
}
