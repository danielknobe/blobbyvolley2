#pragma once

#include <iostream>
#include <Vector.h>

class GUIManager
{
private:
	static GUIManager* mSingleton;

	GUIManager();
public:
	static GUIManager* createGUIManager();
	static GUIManager* getSingleton();

	// Deletes all GUI objects
	void clear();

	int createOverlay(float opacity, Vector2 point1, Vector2 point2);
	int createText(const std::string& text, Vector2 position);
	int createTextButton(const std::string& text, Vector2 position);
	int createImage(const std::string& filename, Vector2 position);

	// Called once per frame
	void processInput();

};

