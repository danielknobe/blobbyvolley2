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
		GUI_IMAGE,
		GUI_SLIDER
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
	bool mDrawCursor;
	
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
// This is set to a fixed length because of the huge additional work which  had
// to be done in the RenderManager. With a fixed length, we can just take a 
// slider image and blit a positioner around it.
	int createSlider(Vector2 position float setting);
	
	void drawCursor(bool draw);
	
	bool getClick(int object);
	float getFloat(int object);

	// Called once per frame
	void processInput();
	void render();

};

