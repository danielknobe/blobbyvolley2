#include <queue>
#include <cassert>

#include "InputManager.h"
#include "RenderManager.h"

#include "IMGUI.h"

enum ObjectType
{
	IMAGE,
	OVERLAY,
	TEXT,
	HIGHLIGHTTEXT,
	BUTTON
};

struct QueueObject
{
	ObjectType type;
	int id;
	Vector2 pos1;
	Vector2 pos2;
	std::string text;
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
	while (!mQueue->empty())
		mQueue->pop();
	
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
				rmanager.drawOverlay(0.5, obj.pos1, obj.pos2);
				break;
			case TEXT:
				rmanager.drawText(obj.text, obj.pos1, false);
				break;
			case HIGHLIGHTTEXT:
				rmanager.drawText(obj.text, obj.pos1, true);
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

void IMGUI::doOverlay(int id, const Vector2& pos1, const Vector2& pos2)
{
	QueueObject obj;
	obj.type = OVERLAY;
	obj.id = id;
	obj.pos1 = pos1;
	obj.pos2 = pos2;
	mQueue->push(obj);
	RenderManager::getSingleton().redraw();
}

bool IMGUI::doButton(int id, const Vector2& position, const std::string& text)
{
	if (mActiveButton == 0)
		mActiveButton = id;
			
	bool clicked = false;
	QueueObject obj;
	obj.id = id;
	obj.pos1 = position;
	obj.text = text;
	obj.type = TEXT;
	
	if (id == mActiveButton)
	{
		obj.type = HIGHLIGHTTEXT;
		if (InputManager::getSingleton()->select())
		clicked = true;
	
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
		}
	}

	mLastWidget = id;
	Vector2 mousepos = InputManager::getSingleton()->position();

	if (mousepos.x > position.x + 24.0 &&
		mousepos.y > position.y &&
		mousepos.x < position.x + text.length() * 24 + 24.0 &&
		mousepos.y < position.y + 24.0)
	{
		obj.type = HIGHLIGHTTEXT;
		if (InputManager::getSingleton()->click())
			clicked = true;
	}
	
	if (id == mActiveButton)
	{
		obj.type = HIGHLIGHTTEXT;
		if (InputManager::getSingleton()->select())
			clicked = true;
	}

	mQueue->push(obj);
	return clicked;
}

void IMGUI::doCursor()
{
	mDrawCursor = true;
}
