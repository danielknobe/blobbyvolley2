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

	int mLeftBlobbyInputDevice;
	int mRightBlobbyInputDevice;

	SDLKey mLeftBlobbyLeftKeyboardMove;
	SDLKey mLeftBlobbyRightKeyboardMove;
	SDLKey mLeftBlobbyKeyboardJump;
	SDLKey mRightBlobbyLeftKeyboardMove;
	SDLKey mRightBlobbyRightKeyboardMove;
	SDLKey mRightBlobbyKeyboardJump;

	// Joystick
	int mLeftBlobbyLeftJoystickMove;
	int mLeftBlobbyRightJoystickMove;
	int mLeftBlobbyJoystickJump;
	bool mLeftBlobbyJoystickDiagonal;
	int mLeftBlobbyJoystickLeftJump;
	int mLeftBlobbyJoystickRightJump;

	int mRightBlobbyLeftJoystickMove;
	int mRightBlobbyRightJoystickMove;
	int mRightBlobbyJoystickJump;
	bool mRightBlobbyJoystickDiagonal;
	int mRightBlobbyJoystickLeftJump;
	int mRightBlobbyJoystickRightJump;

	bool mIngameDevicesAvailable;

	// GUI storage (because in the gui we need a botton hit methods and not a button down methods)
	bool mUp;
	bool mDown;
	bool mLeft;
	bool mRight;
	bool mSelect;
	bool mExit;
	bool mClick;
	
	int mMouseX;
	int mMouseY;
	
	// Inputdevices
	SDL_Joystick* mJoystick;	// Gamepad or Joystick
	int mNumberOfJoysticks;		// How much Joysticks?
	
	UserConfig mConfigManager;
	PlayerInput mInput[2];	
	InputDevice *mInputDevice[2];	
	bool mRunning;
	
	InputManager();
	
public:
	static InputManager* createInputManager();
	static InputManager* getSingleton();
	~InputManager();

	bool createIngameInputDevices();
	bool deleteIngameInputDevices();

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
	SDLKey grabKey();

	// For GUI navigation (Mouse)
	Vector2 position();
	bool click();
	
	// Configmethods
	std::string keyToString(SDLKey key);
	SDLKey stringToKey(const std::string& keyname);
	std::string intToString(int input);
	int stringToInt(const std::string& inputname);

	void configFileToCurrentConfigForLeftKeyboard();
	void configFileToCurrentConfigForRightKeyboard();
	void currentConfigToConfigFileForLeftKeyboard();
	void currentConfigToConfigFileForRightKeyboard();

	void configFileToCurrentConfigForLeftJoystick();
	void configFileToCurrentConfigForRightJoystick();
	void currentConfigToConfigFileForLeftJoystick();
	void currentConfigToConfigFileForRightJoystick();
	
	void configFileToCurrentConfigForRightDevice();
	void configFileToCurrentConfigForLeftDevice();
	void currentConfigToConfigFileForLeftDevice();
	void currentConfigToConfigFileForRightDevice();



};
