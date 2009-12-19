#pragma once

#include <vector>
#include <string>

// class for managing the text
// multilanguage support
class TextManager{
	public:
		// enumeration for languages
		enum LANGUAGE{
			ENGLISH,
			GERMAN
		};
		
		// enumeration for strings
		enum STRING{
			// common labels
			LBL_OK,
			LBL_CANCEL,
			LBL_MAINMENU, 
			LBL_YES,
			LBL_NO,
			LBL_CONF_QUIT,
			LBL_CONTINUE,
			
			// labels for main menu
			MNU_LABEL_START,
			MNU_LABEL_OPTIONS,
			MNU_LABEL_REPLAY,
			MNU_LABEL_CREDITS,
			MNU_LABEL_EXIT,
			MNU_LABEL_NETWORK,
			
			// credits
			CRD_PROGRAMMERS,
			CRD_GRAPHICS,
			CRD_THX,
	
			// replays
			RP_SHOW_AGAIN,
			RP_PLAY,
			RP_DELETE,
			RP_CHECKSUM,
			RP_FILE_CORRUPT,
			RP_SAVE_NAME,
			RP_SAVE,
			
			// game texts
			GAME_WIN,
			GAME_TRY_AGAIN,
			GAME_WAITING,
			GAME_OPP_LEFT,
			GAME_PAUSED,
			GAME_QUIT,
			
			// network texts
			NET_SERVER_SCAN,
			NET_DIRECT_CONNECT,
			NET_SERVER_INFO,
			NET_ACTIVE_GAMES,
			NET_WAITING_PLAYER,
			NET_HOST_GAME,
			NET_CONNECTING,
			NET_DISCONNECT,
			NET_CON_FAILED,
			NET_SERVER_FULL,
			
			// options 
			OP_INPUT_OP,
			OP_GFX_OP,
			OP_MISC,
			OP_VIDEO,
			OP_FULLSCREEN,
			OP_WINDOW,
			OP_RENDER_DEVICE,
			OP_BLOB_COLORS,
			OP_LEFT_PLAYER,
			OP_RIGHT_PLAYER,
			OP_RED,
			OP_GREEN,
			OP_BLUE,
			OP_MORPHING,
			OP_JUMP_BUTTON,
			OP_SET_ALL,
			OP_LEFT_KEY,
			OP_RIGHT_KEY,
			OP_JUMP_KEY,
			OP_LEFT_BUTTON,
			OP_RIGHT_BUTTON,
			OP_PRESS_MOUSE_BUTTON,
			OP_PRESS_KEY_FOR,
			OP_MOVING_LEFT,
			OP_MOVING_RIGHT,
			OP_JUMPING,
			OP_PRESS_BUTTON_FOR,
			OP_BACKGROUND,
			OP_VOLUME,
			OP_MUTE,
			OP_FPS,
			OP_BLOOD,
			OP_NETWORK_SIDE,
			OP_LEFT,
			OP_RIGHT,
			OP_SPEED,
			OP_VSLOW,
			OP_SLOW,
			OP_DEFAULT,
			OP_FAST,
			OP_VFAST,
			
			COUNT
		};
		
		// returns the string identified by str
		const std::string& getString(STRING str) const;
		
		// retruns the mSingleton
		static const TextManager* getSingleton();
		
		// creates a textmanager for a particular language
		static TextManager* createTextManager(LANGUAGE);
	
	private:
		// private construktor, use createTextManager
		TextManager();
		
		// Singleton
		static TextManager* mSingleton;
		
		// vector with all strings
		std::vector<std::string> mStrings;
};
