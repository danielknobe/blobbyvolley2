#pragma once

#include <SDL/SDL.h>

#include "InputSource.h"
#include "InputDevice.h"
#include "UserConfig.h"
#include "Vector.h"

struct InputKeyMap
{
        const char *keyname;
        SDLKey key;
};

class InputManager
{
private:
	static InputManager* mSingleton;
	
	// Keyboard
	static InputKeyMap mKeyMap[];	// Type for String <-convert-> SDLKey

	// GUI storage (because we need event based input for the GUI)
	bool mUp;
	bool mDown;
	bool mLeft;
	bool mRight;
	bool mSelect;
	bool mExit;
	bool mClick;
	
	int mMouseX;
	int mMouseY;
	
	SDLKey mLastTextInputKey;

	PlayerInput mInput[MAX_PLAYERS];	
	InputDevice *mInputDevice[MAX_PLAYERS];	
	bool mRunning;
	
	InputManager();
	
public:
	static InputManager* createInputManager();
	static InputManager* getSingleton();
	~InputManager();

	void beginGame(PlayerSide side);
	void endGame();

	bool running();
	PlayerInput getGameInput(int player);
	void updateInput();

	// For GUI navigation (Gamepad, Joystick or Keyboard)
	bool up();
	bool down();
	bool left();
	bool right();
	bool select();
	bool exit(); // extention for mouse included, so that right click = exit

	std::string getLastTextKey();

	// For GUI navigation (Mouse)
	Vector2 position();
	bool click();
	
	// config conversion methods
	std::string keyToString(SDLKey key);
	SDLKey stringToKey(const std::string& keyname);
};
