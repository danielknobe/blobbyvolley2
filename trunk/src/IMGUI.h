#pragma once

#include <iostream>

#include "Vector.h"

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
	void doText(int id, const Vector2& position, const std::string& text);
	void doOverlay(int id, const Vector2& pos1, const Vector2& pos2, const Color& col = Color(0, 0, 0));
	void doCursor();

	bool doButton(int id, const Vector2& position, const std::string& text);
	bool doScrollbar(int id, const Vector2& position, float& value);
	bool doEditbox(int id, const Vector2& position, std::string& text, int& cpos);
	bool doBlob(int id, const Vector2& position, const Color& col);
private:
	IMGUI();
	~IMGUI();

	static IMGUI* mSingleton;

	int mActiveButton;
	KeyAction mLastKeyAction;
	int mLastWidget;
	bool mDrawCursor;
	bool mButtonReset;
};
