#pragma once

#include <iostream>
#include <vector>
#include "Vector.h"

struct GUIObject
{
	GUIObject()
		: floatValue(0.0),  type(GUI_UNDEFINED), selected(false) {} 
	enum
	{
		GUI_UNDEFINED,
		GUI_OVERLAY,
		GUI_TEXT,
		GUI_TEXTBUTTON,
		GUI_IMAGE
	} type;
	Vector2 pos1;
	Vector2 pos2;
	std::string stringValue;
	float floatValue;
	bool selected;	
};

class GUIManager
{
private:
	static GUIManager* mSingleton;
	
	std::vector<GUIObject> mObjectMap;
	std::vector<int> mSelectableObjects;
	int  mSelectedObject;
	
	Vector2 mCursorPosition;
	
	
	GUIManager();
public:
	static GUIManager* createGUIManager();
	static GUIManager* getSingleton();

	// Deletes all GUI objects
	void clear();

	int createOverlay(float opacity, Vector2 point1, Vector2 point2);
	int createText(const std::string& text, Vector2 position);
	int createTextButton(const std::string& text, int length, Vector2 position);
	int createImage(const std::string& filename, Vector2 position);
	
	bool getClick(int object);

	// Called once per frame
	void processInput();
	void render();

};

