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

#include "InputManager.h"
#include "RenderManager.h"

#include "IMGUI.h"

enum ObjectType
{
	IMAGE,
	OVERLAY,
	TEXT,
	HIGHLIGHTTEXT,
	BUTTON,
	SCROLLBAR,
	ACTIVESCROLLBAR,
	EDITBOX,
	ACTIVEEDITBOX,
	HIGHLIGHTEDITBOX,
	SELECTBOX,
	ACTIVESELECTBOX,
	BLOB
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
	int lenght;
};

typedef std::queue<QueueObject> RenderQueue;

IMGUI* IMGUI::mSingleton = 0;
RenderQueue *mQueue;

IMGUI::IMGUI()
{
	mQueue = new RenderQueue;
	mActiveButton = 0;
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
				rmanager.drawText(obj.text, obj.pos1, false);
				break;
			case HIGHLIGHTTEXT:
				rmanager.drawText(obj.text, obj.pos1, true);
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
				rmanager.drawOverlay(0.4, obj.pos1, obj.pos1 + Vector2(10.0+obj.lenght*24.0, 10.0+24.0));
				rmanager.drawText(obj.text, obj.pos1+Vector2(5.0, 5.0), false);
				break;
			case ACTIVEEDITBOX:
				rmanager.drawOverlay(0.2, obj.pos1, obj.pos1 + Vector2(10.0+obj.lenght*24.0, 10.0+24.0));
				rmanager.drawText(obj.text, obj.pos1+Vector2(5.0, 5.0), true);
				if (obj.pos2.x >= 0)
					rmanager.drawOverlay(1.0, Vector2((obj.pos2.x)*24.0+obj.pos1.x+5.0, obj.pos1.y+5.0), Vector2((obj.pos2.x)*24.0+obj.pos1.x+5.0+3.0, obj.pos1.y+5.0+24.0), Color(255,255,255));
				break;
			case HIGHLIGHTEDITBOX:
				rmanager.drawOverlay(0.2, obj.pos1, obj.pos1 + Vector2(10.0+obj.lenght*24.0, 10.0+24.0));
				rmanager.drawText(obj.text, obj.pos1+Vector2(5.0, 5.0), true);
				break;
			case SELECTBOX:
				rmanager.drawOverlay(0.4, obj.pos1, obj.pos2);
				for (int c = 0; c < obj.entries.size(); c++)
					rmanager.drawText(obj.entries[c], Vector2(obj.pos1.x+5, obj.pos1.y+(c*24)+5), (c == obj.selected));
				break;
			case ACTIVESELECTBOX:
				rmanager.drawOverlay(0.2, obj.pos1, obj.pos2);
				for (int c = 0; c < obj.entries.size(); c++)
					rmanager.drawText(obj.entries[c], Vector2(obj.pos1.x+5, obj.pos1.y+(c*24)+5), (c == obj.selected));
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

void IMGUI::doText(int id, const Vector2& position, const std::string& text)
{
	QueueObject obj;
	obj.type = TEXT;
	obj.id = id;
	obj.pos1 = position;
	obj.text = text;
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

bool IMGUI::doButton(int id, const Vector2& position, const std::string& text)
{
	bool clicked = false;
	QueueObject obj;
	obj.id = id;
	obj.pos1 = position;
	obj.text = text;
	obj.type = TEXT;
	
	if (!mInactive)
	{
		if (mActiveButton == 0 && !mButtonReset)
			mActiveButton = id;

		if (id == mActiveButton)
		{
			obj.type = HIGHLIGHTTEXT;
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

		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x > position.x &&
			mousepos.y > position.y &&
			mousepos.x < position.x + text.length() * 24 &&
			mousepos.y < position.y + 24.0)
		{
			obj.type = HIGHLIGHTTEXT;
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
	float oldvalue = value;
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
		if (mActiveButton == 0 && !mButtonReset)
			mActiveButton = id;

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

		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x + 5 > position.x &&
			mousepos.y > position.y &&
			mousepos.x < position.x + 205 &&
			mousepos.y < position.y + 24.0)
		{
			if (InputManager::getSingleton()->click())
			{
				mHeldWidget = id;
			}
			if (mHeldWidget == id)
			{
				value = (mousepos.x - position.x) / 200.0;
				mActiveButton = id;
			}
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
	mActiveButton = 0;
	mButtonReset = true;
}

bool IMGUI::doEditbox(int id, const Vector2& position, int length, std::string& text, unsigned& cpos)
{
	// lenght does not actually work!
	bool changed = false;
	QueueObject obj;
	obj.id = id;
	obj.pos1 = position;
	obj.type = EDITBOX;
	obj.lenght = length;

		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x > position.x &&
			mousepos.y > position.y &&
			mousepos.x < position.x + length * 24.0 + 10.0 &&
			mousepos.y < position.y + 24.0 + 10.0)
		{
			obj.type = HIGHLIGHTEDITBOX;
			if (InputManager::getSingleton()->click())
			{
				if (mousepos.x < position.x + text.length() * 24.0)
					cpos = (int)(mousepos.x-position.x-5.0+12)/24;
				mActiveButton = id;
			}
		}

	if (!mInactive)
	{
		if (mActiveButton == 0 && !mButtonReset)
			mActiveButton = id;

		if (id == mActiveButton)
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

	return changed;
}

bool IMGUI::doSelectbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, int& selected)
{
	bool changed = false;
	QueueObject obj;
	obj.id = id;
	obj.pos1 = pos1;
	obj.pos2 = pos2;
	obj.type = SELECTBOX;

	const int itemsPerPage = int(pos2.y - pos1.y - 10) / 24;
	int first = (int)(selected / itemsPerPage)*itemsPerPage;	//the first visible element in the list

	if (!mInactive)
	{
		if (mActiveButton == 0 && !mButtonReset)
			mActiveButton = id;

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
						changed = true;
					}
					mLastKeyAction = NONE;
					break;
				case RIGHT:
					if (selected < entries.size()-1)
					{
						selected++;
						changed = true;
					}
					mLastKeyAction = NONE;
					break;
				default:
					break;
			}
		}
		
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
			mousepos.y < pos1.y+5+24*itemsPerPage)
		{
			if (InputManager::getSingleton()->click())
			{
				int tmp = (int)((mousepos.y-pos1.y-5) / 24)+first;
				if (tmp < entries.size())
					selected = tmp;
				mActiveButton = id;
			}
		}
		//arrows mouseclick:
		if (mousepos.x > pos2.x-30 && mousepos.x < pos2.x-30+24 && InputManager::getSingleton()->click())
		{
			if (mousepos.y > pos1.y+3 && mousepos.y < pos1.y+3+24 && selected > 0)
			{
				selected--;
				changed = true;
			}
			if (mousepos.y > pos2.y-27 && mousepos.y < pos2.y-27+24 && selected < entries.size()-1)
			{
				selected++;
				changed = true;
			}
		}
	}
	doImage(GEN_ID, Vector2(pos2.x-15, pos1.y+15), "gfx/pfeil_oben.bmp");
	doImage(GEN_ID, Vector2(pos2.x-15, pos2.y-15), "gfx/pfeil_unten.bmp");

	first = (selected / itemsPerPage)*itemsPerPage; //recalc first
	if (entries.size() != 0)
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

bool IMGUI::usingCursor()
{
	return mUsingCursor;
}
