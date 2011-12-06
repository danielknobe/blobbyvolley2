/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/


#pragma once

#include <vector>
#include <string>
#include <map>

/// \brief class for managing the text
/// \details multilanguage support
/// the string can be loaded from a xml file
/// <string english="english" translation="translation />

class TextManager{
	public:
		/// enumeration for strings
		enum STRING{
			// common labels
			LBL_OK,
			LBL_CANCEL,
			LBL_YES,
			LBL_NO,
			LBL_CONF_QUIT,
			LBL_CONTINUE,
			
			// labels for main menu
			MNU_LABEL_ONLINE,
			MNU_LABEL_LAN,
			MNU_LABEL_START,
			MNU_LABEL_OPTIONS,
			MNU_LABEL_REPLAY,
			MNU_LABEL_CREDITS,
			MNU_LABEL_EXIT,
			
			// credits
			CRD_PROGRAMMERS,
			CRD_GRAPHICS,
			CRD_THX,
	
			// replays
			RP_SHOW_AGAIN,
			RP_PLAY,
			RP_DELETE,
			RP_CHECKSUM,
			RP_VERSION,
			RP_FILE_OUTDATED,
			RP_FILE_CORRUPT,
			RP_SAVE_NAME,
			RP_WAIT_REPLAY,
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
			OP_SHOW_SHADOW,
			OP_BLOB_COLORS,
			OP_LEFT_PLAYER,
			OP_RIGHT_PLAYER,
			OP_RED,
			OP_GREEN,
			OP_BLUE,
			OP_MORPHING,
			OP_KEYBOARD,
			OP_MOUSE,
			OP_JOYSTICK,
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
			OP_LANGUAGE,
			OP_DIFFICULTY,
			OP_WEAK,
			OP_MEDIUM,
			OP_STRONG,
			
			UPDATE_NOTIFICATION,
			
			COUNT
		};
		
		/// returns the string identified by str
		const std::string& getString(STRING str) const;
		
		std::string getLang() const;
		
		/// returns the mSingleton
		static const TextManager* getSingleton();
		
		/// creates a textmanager for a particular language
		static TextManager* createTextManager(std::string langname);
	
		/// switches the language
		static void switchLanguage(std::string langname);
		
		/// map to map abbreviations to full name (e.g. de to deutsch)
		static std::map<std::string, std::string> language_names;
		
	private:
		/// private construktor, use createTextManager
		TextManager(std::string l);
		
		/// Singleton
		static TextManager* mSingleton;
		
		/// vector with all strings
		std::vector<std::string> mStrings;
		
		/// string with language name
		std::string lang;
		
		/// loads the language data from an xml file
		bool loadFromXML(std::string file);
		
		/// sets the strings to the default values
		void setDefault();
};
