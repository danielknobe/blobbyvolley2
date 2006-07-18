#include "GUIManager.h"
#include "RenderManager.h"
#include "InputManager.h"

GUIManager* GUIManager::mSingleton= 0;

GUIManager* GUIManager::getSingleton()
{
	assert(mSingleton);
	return mSingleton;
}	

void GUIManager::clear()
{
	mObjectMap.clear();
	mSelectableObjects.clear();
	mSelectedObject = -1;
}

GUIManager::GUIManager()
{
	mCursorPosition = Vector2(-100.0, -100.0);
	mSingleton = this;
}

GUIManager* GUIManager::createGUIManager()
{
	return new GUIManager();
}

int GUIManager::createOverlay(float opacity, Vector2 point1, Vector2 point2)
{
	GUIObject overlayObject;
	overlayObject.type = GUIObject::GUI_OVERLAY;
	overlayObject.floatValue = opacity;
	overlayObject.pos1 = point1;
	overlayObject.pos2 = point2;
	mObjectMap.push_back(overlayObject);
	return mObjectMap.size() - 1;
}

int GUIManager::createImage(const std::string& filename, Vector2 position)
{
	GUIObject imageObject;
	imageObject.type = GUIObject::GUI_IMAGE;
	imageObject.pos1 = position;
	imageObject.stringValue = filename;
	mObjectMap.push_back(imageObject);
	return mObjectMap.size() - 1;	
}

int GUIManager::createTextButton(const std::string& text, int length, Vector2 position)
{
	GUIObject buttonObject;
	buttonObject.type = GUIObject::GUI_TEXTBUTTON;
	buttonObject.pos1 = position;
	buttonObject.pos2 = Vector2(position.x + length * 24, position.y + 24);
	buttonObject.stringValue = text;
	mObjectMap.push_back(buttonObject);
	int index = mObjectMap.size() - 1;
	if (mSelectableObjects.empty())
		mSelectedObject = -1;
	mSelectableObjects.push_back(index);
	return index;
	
}

int GUIManager::createText(const std::string& text, Vector2 position)
{
	GUIObject textObject;
	textObject.type = GUIObject::GUI_TEXT;
	textObject.pos1 = position;
	textObject.stringValue = text;
	mObjectMap.push_back(textObject);
	return mObjectMap.size() - 1;
}

int GUIManager::createSlider(Vector2 position, float setting)
{
	GUIObject sliderObject;
	sliderObject.type = GUIObject::GUI_SLIDER;
	sliderObject.pos1 = position;
	sliderObject.floatValue = setting;	
}

void GUIManager::drawCursor(bool draw)
{
	mDrawCursor = draw;
}
	
void GUIManager::processInput()
{
	InputManager* inputmgr = InputManager::getSingleton();
	Vector2 oldCursorPosition = mCursorPosition;
	mCursorPosition = inputmgr->position();
	
	for (int i = 0; i != mSelectableObjects.size(); ++i)
	{
		GUIObject& object = mObjectMap[mSelectableObjects[i]];
		object.selected = false;
		if (mCursorPosition.x > object.pos1.x + 24.0 &&
			mCursorPosition.y > object.pos1.y &&
			mCursorPosition.x < object.pos2.x + 24.0 &&
			mCursorPosition.y < object.pos2.y)
		{
			if (mCursorPosition != oldCursorPosition)
			{
				mSelectedObject = i;
			}
			object.selected = true;
		}
	}
	if (!mSelectableObjects.empty())
	{
		if (inputmgr->down())
			mSelectedObject = mSelectedObject + 1 < mSelectableObjects.size() ? 
				mSelectedObject + 1 : 0;
		if (inputmgr->up())
			mSelectedObject = mSelectedObject > 0 ? 
				mSelectedObject - 1 : mSelectableObjects.size() - 1;
	}
	
}
bool GUIManager::getClick(int object)
{
	InputManager* inputmgr = InputManager::getSingleton();
	bool mouseClick = mObjectMap.at(object).selected && inputmgr->click();
	bool keyClick = mSelectableObjects[mSelectedObject] == object && inputmgr->select();
	return mouseClick || keyClick;

}

float GUIManager::getFloat(int object)
{
	if (mObjectMap[object].type == GUIObject::GUI_SLIDER)
		return mObjectMap[object].floatValue;
}

void GUIManager::render()
{
	RenderManager* rmanager = &RenderManager::getSingleton();
	for (int i = 0; i < mObjectMap.size(); ++i)
	{
		GUIObject& object = mObjectMap[i];
		if (mObjectMap[i].type == GUIObject::GUI_OVERLAY)
		{
			rmanager->drawOverlay(object.floatValue, object.pos1, object.pos2);
		}
		if (mObjectMap[i].type == GUIObject::GUI_TEXT || 
			mObjectMap[i].type == GUIObject::GUI_TEXTBUTTON)
		{
			rmanager->drawText(object.stringValue, object.pos1,
				i == mSelectableObjects[mSelectedObject]);
		}
		if (mObjectMap[i].type == GUIObject::GUI_IMAGE)
		{
			rmanager->drawImage(object.stringValue, object.pos1);
		}
	}
	if (mDrawCursor)
		rmanager->drawImage("gfx/cursor.bmp", 
			mCursorPosition + Vector2(24.0, 24.0));
}
