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
};

typedef std::queue<QueueObject> RenderQueue;

IMGUI* IMGUI::mSingleton = 0;
RenderQueue *mQueue;

IMGUI::IMGUI()
{
	mQueue = new RenderQueue;
	mActiveButton = 0;
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
	mInactive = false;
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
				rmanager.drawOverlay(0.5, obj.pos1, obj.pos2, obj.col);
				break;
			case TEXT:
				rmanager.drawText(obj.text, obj.pos1, false);
				break;
			case HIGHLIGHTTEXT:
				rmanager.drawText(obj.text, obj.pos1, true);
				break;
			case SCROLLBAR:
				rmanager.drawOverlay(0.5, obj.pos1, obj.pos1 + Vector2(210.0, 25.0));
				rmanager.drawOverlay(0.7, obj.pos1 + Vector2(obj.pos2.x * 200.0, 0.0), obj.pos1 + Vector2(obj.pos2.x * 200 + 10, 25.0), Color(255, 255, 255));
				break;
			case ACTIVESCROLLBAR:
				rmanager.drawOverlay(0.5, obj.pos1, obj.pos1 + Vector2(210.0, 25.0));
				rmanager.drawOverlay(1.0, obj.pos1 + Vector2(obj.pos2.x * 200.0, 0.0), obj.pos1 + Vector2(obj.pos2.x * 200 + 10, 25.0), Color(255, 255, 255));
				break;
			case EDITBOX:
				rmanager.drawOverlay(0.4, obj.pos1, obj.pos1 + Vector2(10.0+obj.text.length()*24.0, 10.0+24.0));
				rmanager.drawText(obj.text, obj.pos1+Vector2(5.0, 5.0), false);
				if (obj.pos2.x >= 0)
					rmanager.drawOverlay(1.0, Vector2((obj.pos2.x)*24.0+obj.pos1.x+5.0, obj.pos1.y+5.0), Vector2((obj.pos2.x)*24.0+obj.pos1.x+5.0+3.0, obj.pos1.y+5.0+24.0), Color(255,255,255));
				break;
			case ACTIVEEDITBOX:
				rmanager.drawOverlay(0.5, obj.pos1, obj.pos1 + Vector2(10.0+obj.text.length()*24.0, 10.0+24.0));
				rmanager.drawText(obj.text, obj.pos1+Vector2(5.0, 5.0), true);
				if (obj.pos2.x >= 0)
					rmanager.drawOverlay(1.0, Vector2((obj.pos2.x)*24.0+obj.pos1.x+5.0, obj.pos1.y+5.0), Vector2((obj.pos2.x)*24.0+obj.pos1.x+5.0+3.0, obj.pos1.y+5.0+24.0), Color(255,255,255));
				break;
			case SELECTBOX:
				rmanager.drawOverlay(0.5, obj.pos1, obj.pos2);
				for (int c = 0; c < obj.entries.size(); c++)
					rmanager.drawText(obj.entries[c], Vector2(obj.pos1.x+5, obj.pos1.y+(c*24)+5), (c == obj.selected));
				break;
			case ACTIVESELECTBOX:
				rmanager.drawOverlay(0.4, obj.pos1, obj.pos2);
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
				value = (mousepos.x - position.x) / 200.0;
				mActiveButton = id;
			}
		}
	}

	value = value > 0.0 ? (value < 1.0 ? value : 1.0) : 0.0;
	obj.pos2.x = value;

	mLastWidget = id;
	mQueue->push(obj);

	return oldvalue != value;
}

void IMGUI::resetSelection()
{
	mActiveButton = 0;
	mButtonReset = true;
}

bool IMGUI::doEditbox(int id, const Vector2& position, int length, std::string& text, unsigned& cpos)
{
	bool changed = false;
	QueueObject obj;
	obj.id = id;
	obj.pos1 = position;
	obj.type = EDITBOX;

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
			// UTF-8 class can tell the real length.
			else if (input.length() == 1)
			{
				text.insert(cpos, input);
				cpos++;
				changed = true;
			}
		}
		
		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x > position.x &&
			mousepos.y > position.y &&
			mousepos.x < position.x + length * 24.0 + 10.0 &&
			mousepos.y < position.y + 24.0 + 10.0)
		{
			obj.type = ACTIVEEDITBOX;
			if (InputManager::getSingleton()->click())
			{
				cpos = (int)(mousepos.x-position.x-5.0)/24;
				mActiveButton = id;
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
