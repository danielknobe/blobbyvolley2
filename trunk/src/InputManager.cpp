#include <cassert>
#include <SDL/SDL.h>
#include "InputManager.h"

#include "SoundManager.h"

InputManager* InputManager::mSingleton = 0;

InputManager::InputManager()
{
	assert (mSingleton == 0);
	mSingleton = this;
	mRunning = true;
	mJoystick = 0;
#if defined(__arm__) && defined(linux)
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	mJoystick = SDL_JoystickOpen(0);
#endif
	// No key in the GUI is pressed
	mUp = false;
	mDown = false;
	mLeft = false;
	mRight = false;
	mSelect = false;
	mExit = false;
	mClick = false;
	
	// initialize the SDLKeys for ingame input
	configFileToCurrentConfig();
}

InputManager* InputManager::getSingleton()
{
	assert(mSingleton);
	return mSingleton;
}

PlayerInput InputManager::getGameInput(int player)
{
	assert (player >= 0 && player < 2);
	return mInput[player];
}

InputManager* InputManager::createInputManager()
{
	return new InputManager();
}

void InputManager::updateInput()
{
	static float volume = 1.0;
	SDL_PumpEvents();
	SDL_Event event;
	keyState = SDL_GetKeyState(0);

	if (mJoystick)
		SDL_JoystickUpdate();

	// AUSLAUFMODELLAUFBAU==================================================
	while (SDL_PollEvent(&event))
	switch (event.type)
	{
		case SDL_MOUSEBUTTONDOWN:
		case SDL_KEYDOWN:
			if (exit())
				mRunning = 0;
			break;		
		case SDL_QUIT:
			mRunning = 0;
			break;
#if defined(__arm__) && defined(linux)
		case SDL_JOYBUTTONDOWN:
			switch (event.jbutton.button)
			{
				case 17:
					volume -= 0.15;
				break;
				case 16:
					volume += 0.15;
				break;
			}
			SoundManager::getSingleton().setVolume(volume);
			break;
#endif
	}

#if defined(__arm__) && defined(linux)
	mInput[0] = PlayerInput(
		SDL_JoystickGetButton(mJoystick, 2) ||
			SDL_JoystickGetButton(mJoystick, 1),
		SDL_JoystickGetButton(mJoystick, 6) ||
			SDL_JoystickGetButton(mJoystick, 7),
		SDL_JoystickGetButton(mJoystick, 0) ||
			SDL_JoystickGetButton(mJoystick, 1) ||
			SDL_JoystickGetButton(mJoystick, 7));
	mInput[1] = PlayerInput(
		SDL_JoystickGetButton(mJoystick, 12),
		SDL_JoystickGetButton(mJoystick, 13),
		SDL_JoystickGetButton(mJoystick, 14)
	);
	if (
		SDL_JoystickGetButton(mJoystick, 10) &&
		SDL_JoystickGetButton(mJoystick, 11)
	)
		mRunning = false;
	// AUSLAUFMODELLAUFBAU==================================================
#else



	// Later in seperat method
	// Set Inputkeys
	mInput[0] = PlayerInput(keyState[mLeftBlobbyLeftMove], keyState[mLeftBlobbyRightMove], 
		keyState[mLeftBlobbyJump]);
	mInput[1] = PlayerInput(keyState[mRightBlobbyLeftMove],
			keyState[mRightBlobbyRightMove], keyState[mRightBlobbyJump]);
#endif
}

// GUI-Methods
bool InputManager::up()
{
	if(keyState[SDLK_UP])
	{
		if(mUp==false)
		{
			mUp=true;
			return true;
    	}
    	else
    		return false;
	}
	else
	{
		mUp=false;
		return false;	
	}
}

bool InputManager::down()
{
	if(keyState[SDLK_DOWN])
	{
		if(mDown==false)
		{
			mDown=true;
			return true;
    	}
    	else
    		return false;
	}
	else
	{
		mDown=false;
		return false;	
	}
}
     
bool InputManager::left()
{
	if(keyState[SDLK_LEFT])
	{
		if(mLeft==false)
		{
			mLeft=true;
			return true;
    	}
    	else
    		return false;
	}
	else
	{
		mLeft=false;
		return false;	
	}
}

bool InputManager::right()
{
	if(keyState[SDLK_RIGHT])
	{
		if(mRight==false)
		{
			mRight=true;
			return true;
    	}
    	else
    		return false;
	}
	else
	{
		mRight=false;
		return false;	
	}
}

bool InputManager::select()
{
	if(keyState[SDLK_RETURN] || keyState[SDLK_SPACE])
	{
		if(mSelect==false)
		{
			mSelect=true;
			return true;
    	}
    	else
    		return false;
	}
	else
	{
		mSelect=false;
		return false;	
	}
}

bool InputManager::exit()
{
	if(keyState[SDLK_ESCAPE] || keyState[SDLK_BACKSPACE] || SDL_GetMouseState(NULL,NULL)&SDL_BUTTON(3))
	{
		if(mExit==false)
		{
			mExit=true;
			return true;
    	}
    	else
    		return false;
	}
	else
	{
		mExit=false;
		return false;	
	}
}

Vector2 InputManager::position()
{
	SDL_GetMouseState(&mouseX,&mouseY);
	return Vector2(mouseX,mouseY);
}

bool InputManager::click()
{
	if(SDL_GetMouseState(NULL,NULL)&SDL_BUTTON(1))
	{
		if(mClick==false)
		{
			mClick=true;
			return true;
    	}
    	else
    		return false;
	}
	else
	{
		mClick=false;
		return false;	
	}
}

bool InputManager::running()
{
	return mRunning;
}




// =================================EXPERIMENTAL!!!

// declaration of the keymap
InputKeyMap InputManager::mKeyMap[] = {
	"",SDLK_UNKNOWN,
	"",SDLK_FIRST,
	"backspace",SDLK_BACKSPACE,
	"tabulator",SDLK_TAB,
	"",SDLK_CLEAR,
	"return",SDLK_RETURN,
	"pause",SDLK_PAUSE,
	"",SDLK_ESCAPE,
	"space",SDLK_SPACE,
	"exclaim",SDLK_EXCLAIM,
	"quotedouble",SDLK_QUOTEDBL,
	"#",SDLK_HASH,
	"$",SDLK_DOLLAR,
	"&",SDLK_AMPERSAND,
	"'",SDLK_QUOTE,
	"(",SDLK_LEFTPAREN,
	")",SDLK_RIGHTPAREN,
	"*",SDLK_ASTERISK,
	"+",SDLK_PLUS,
	",",SDLK_COMMA,
	"-",SDLK_MINUS,
	".",SDLK_PERIOD,
	"/",SDLK_SLASH,
	"0",SDLK_0,
	"1",SDLK_1,
	"2",SDLK_2,
	"3",SDLK_3,
	"4",SDLK_4,
	"5",SDLK_5,
	"6",SDLK_6,
	"7",SDLK_7,
	"8",SDLK_8,
	"9",SDLK_9,
	":",SDLK_COLON,
	";",SDLK_SEMICOLON,
	"<",SDLK_LESS,
	"=",SDLK_EQUALS,
	">",SDLK_GREATER,
	"?",SDLK_QUESTION,
	"@",SDLK_AT,

	/* 
	   Skip uppercase letters
	 */
	"[",SDLK_LEFTBRACKET,
	"backslash",SDLK_BACKSLASH,
	"]",SDLK_RIGHTBRACKET,
	"^",SDLK_CARET,
	"_",SDLK_UNDERSCORE,
	"`",SDLK_BACKQUOTE,
	"a",SDLK_a,
	"b",SDLK_b,
	"c",SDLK_c,
	"d",SDLK_d,
	"e",SDLK_e,
	"f",SDLK_f,
	"g",SDLK_g,
	"h",SDLK_h,
	"i",SDLK_i,
	"j",SDLK_j,
	"k",SDLK_k,
	"l",SDLK_l,
	"m",SDLK_m,
	"n",SDLK_n,
	"o",SDLK_o,
	"p",SDLK_p,
	"q",SDLK_q,
	"r",SDLK_r,
	"s",SDLK_s,
	"t",SDLK_t,
	"u",SDLK_u,
	"v",SDLK_v,
	"w",SDLK_w,
	"x",SDLK_x,
	"y",SDLK_y,
	"z",SDLK_z,
	"del",SDLK_DELETE,
	/* End of ASCII mapped keysyms */

	/* International keyboard syms */
	"world0",SDLK_WORLD_0,
	"world1",SDLK_WORLD_1,
	"world2",SDLK_WORLD_2,
	"world3",SDLK_WORLD_3,
	"world4",SDLK_WORLD_4,
	"world5",SDLK_WORLD_5,
	"world6",SDLK_WORLD_6,
	"world7",SDLK_WORLD_7,
	"world8",SDLK_WORLD_8,
	"world9",SDLK_WORLD_9,
	"world10",SDLK_WORLD_10,
	"world11",SDLK_WORLD_11,
	"world12",SDLK_WORLD_12,
	"world13",SDLK_WORLD_13,
	"world14",SDLK_WORLD_14,
	"world15",SDLK_WORLD_15,
	"world16",SDLK_WORLD_16,
	"world17",SDLK_WORLD_17,
	"world18",SDLK_WORLD_18,
	"world19",SDLK_WORLD_19,
	"world20",SDLK_WORLD_20,
	"world21",SDLK_WORLD_21,
	"world22",SDLK_WORLD_22,
	"world23",SDLK_WORLD_23,
	"world24",SDLK_WORLD_24,
	"world25",SDLK_WORLD_25,
	"world26",SDLK_WORLD_26,
	"world27",SDLK_WORLD_27,
	"world28",SDLK_WORLD_28,
	"world29",SDLK_WORLD_29,
	"world30",SDLK_WORLD_30,
	"world31",SDLK_WORLD_31,
	"world32",SDLK_WORLD_32,
	"world33",SDLK_WORLD_33,
	"world34",SDLK_WORLD_34,
	"world35",SDLK_WORLD_35,
	"world36",SDLK_WORLD_36,
	"world37",SDLK_WORLD_37,
	"world38",SDLK_WORLD_38,
	"world39",SDLK_WORLD_39,
	"world40",SDLK_WORLD_40,
	"world41",SDLK_WORLD_41,
	"world42",SDLK_WORLD_42,
	"world43",SDLK_WORLD_43,
	"world44",SDLK_WORLD_44,
	"world45",SDLK_WORLD_45,
	"world46",SDLK_WORLD_46,
	"world47",SDLK_WORLD_47,
	"world48",SDLK_WORLD_48,
	"world49",SDLK_WORLD_49,
	"world50",SDLK_WORLD_50,
	"world51",SDLK_WORLD_51,
	"world52",SDLK_WORLD_52,
	"world53",SDLK_WORLD_53,
	"world54",SDLK_WORLD_54,
	"world55",SDLK_WORLD_55,
	"world56",SDLK_WORLD_56,
	"world57",SDLK_WORLD_57,
	"world58",SDLK_WORLD_58,
	"world69",SDLK_WORLD_59,
	"world60",SDLK_WORLD_60,
	"world61",SDLK_WORLD_61,
	"world62",SDLK_WORLD_62,
	"world63",SDLK_WORLD_63,
	"world64",SDLK_WORLD_64,
	"world65",SDLK_WORLD_65,
	"world66",SDLK_WORLD_66,
	"world67",SDLK_WORLD_67,
	"world68",SDLK_WORLD_68,
	"world69",SDLK_WORLD_69,
	"world70",SDLK_WORLD_70,
	"world71",SDLK_WORLD_71,
	"world72",SDLK_WORLD_72,
	"world73",SDLK_WORLD_73,
	"world74",SDLK_WORLD_74,
	"world75",SDLK_WORLD_75,
	"world76",SDLK_WORLD_76,
	"world77",SDLK_WORLD_77,
	"world78",SDLK_WORLD_78,
	"world79",SDLK_WORLD_79,
	"world80",SDLK_WORLD_80,
	"world81",SDLK_WORLD_81,
	"world82",SDLK_WORLD_82,
	"world83",SDLK_WORLD_83,
	"world84",SDLK_WORLD_84,
	"world85",SDLK_WORLD_85,
	"world86",SDLK_WORLD_86,
	"world87",SDLK_WORLD_87,
	"world88",SDLK_WORLD_88,
	"world89",SDLK_WORLD_89,
	"world90",SDLK_WORLD_90,
	"world91",SDLK_WORLD_91,
	"world92",SDLK_WORLD_92,
	"world93",SDLK_WORLD_93,
	"world94",SDLK_WORLD_94,
	"world95",SDLK_WORLD_95,

	/* Numeric keypad */
	"keypad0",SDLK_KP0,
	"keypad1",SDLK_KP1,
	"keypad2",SDLK_KP2,
	"keypad3",SDLK_KP3,
	"keypad4",SDLK_KP4,
	"keypad5",SDLK_KP5,
	"keypad6",SDLK_KP6,
	"keypad7",SDLK_KP7,
	"keypad8",SDLK_KP8,
	"keypad9",SDLK_KP9,
	"keypad.",SDLK_KP_PERIOD,
	"keypad/",SDLK_KP_DIVIDE,
	"keypad*",SDLK_KP_MULTIPLY,
	"keypad-",SDLK_KP_MINUS,
	"keypad+",SDLK_KP_PLUS,
	"keypadenter",SDLK_KP_ENTER,
	"keypad",SDLK_KP_EQUALS,

	/* Arrows + Home/End pad */
	"up",SDLK_UP,
	"down",SDLK_DOWN,
	"right",SDLK_RIGHT,
	"left",SDLK_LEFT,
	"insert",SDLK_INSERT,
	"home",SDLK_HOME,
	"end",SDLK_END,
	"pageup",SDLK_PAGEUP,
	"pagedown",SDLK_PAGEDOWN,

	/* Function keys */
	"f1",SDLK_F1,
	"f2",SDLK_F2,
	"f3",SDLK_F3,
	"f4",SDLK_F4,
	"f5",SDLK_F5,
	"f6",SDLK_F6,
	"f7",SDLK_F7,
	"f8",SDLK_F8,
	"f9",SDLK_F9,
	"f10",SDLK_F10,
	"f11",SDLK_F11,
	"f12",SDLK_F12,
	"f13",SDLK_F13,
	"f14",SDLK_F14,
	"f15",SDLK_F15,

	/* Key state modifier keys */
	"numlock",SDLK_NUMLOCK,
	"capslock",SDLK_CAPSLOCK,
	"scrollock",SDLK_SCROLLOCK,
	"rightshift",SDLK_RSHIFT,
	"leftshift",SDLK_LSHIFT,
	"rightcontrol",SDLK_RCTRL,
	"leftcontrol",SDLK_LCTRL,
	"rightalt",SDLK_RALT,
	"leftalt",SDLK_LALT,
	"rmeta",SDLK_RMETA,
	"lmeta",SDLK_LMETA,
	"leftwin",SDLK_LSUPER,
	"rightwin",SDLK_RSUPER,
	"mode",SDLK_MODE,		/* "Alt Gr" key */
	"compose",SDLK_COMPOSE,	/* Multi-key compose key */

	/* Miscellaneous function keys */
	"help",SDLK_HELP,
	"print",SDLK_PRINT,
	"sysreq",SDLK_SYSREQ,
	"break",SDLK_BREAK,
	"menu",SDLK_MENU,
	"power",SDLK_POWER,		/* Power Macintosh power key */
	"euro",SDLK_EURO,		/* Some european keyboards */
	"undo",SDLK_UNDO,		/* Atari keyboard has Undo */
	NULL					// end of the keymap
};	

std::string InputManager::keyToString (const SDLKey key)
{
	int i = 0;
	while (mKeyMap[i].keyname != NULL)
	{
		if (mKeyMap[i].key == key)
			return mKeyMap[i].keyname;
		++i;
	}
	return "";
}

SDLKey InputManager::stringToKey (const std::string& keyname)
{
	int i = 0;
	while (mKeyMap[i].keyname != NULL)
	{
		if (keyname == mKeyMap[i].keyname)
			return mKeyMap[i].key;
		i++;
	}
	return SDLK_UNKNOWN; // stringinformation = ""
}

void InputManager::configFileToCurrentConfig()
{
	mConfigManager.loadFile("inputconfig.xml");
	mLeftBlobbyLeftMove=stringToKey(mConfigManager.getString("left_blobby_left_move"));
	mLeftBlobbyRightMove=stringToKey(mConfigManager.getString("left_blobby_right_move"));
	mLeftBlobbyJump=stringToKey(mConfigManager.getString("left_blobby_jump"));
	mRightBlobbyLeftMove=stringToKey(mConfigManager.getString("right_blobby_left_move"));
	mRightBlobbyRightMove=stringToKey(mConfigManager.getString("right_blobby_right_move"));
	mRightBlobbyJump=stringToKey(mConfigManager.getString("right_blobby_jump"));
}

void InputManager::currentConfigToConfigFile()
{
	mConfigManager.loadFile("inputconfig.xml");
	mConfigManager.setString("left_blobby_left_move",keyToString(mLeftBlobbyLeftMove));
	mConfigManager.setString("left_blobby_right_move",keyToString(mLeftBlobbyRightMove));
	mConfigManager.setString("left_blobby_jump",keyToString(mLeftBlobbyJump));
	mConfigManager.setString("right_blobby_left_move",keyToString(mRightBlobbyLeftMove));
	mConfigManager.setString("right_blobby_right_move",keyToString(mRightBlobbyRightMove));
	mConfigManager.setString("right_blobby_jump",keyToString(mRightBlobbyJump));
	mConfigManager.saveFile("InputConfig.ini");
}
