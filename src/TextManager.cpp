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


#include "TextManager.h"

#include "Global.h"
#include "File.h"

#include "tinyxml/tinyxml.h"

#include <iostream>
#include <algorithm>

TextManager* TextManager::mSingleton = 0;


TextManager* TextManager::createTextManager(std::string langname){
	delete mSingleton;
	
	mSingleton = new TextManager(langname);
	
	std::string langfile = "lang_"+langname+".xml";
	
	bool loaded = false;
	try{
		loaded = mSingleton->loadFromXML(langfile);
	}catch(FileLoadException& fle){
		std::cerr << fle.what() << std::endl;
	};
	
	if(!loaded){
		std::cerr << "error loading language " << langfile << "!" << std::endl;
		std::cerr << "\tfalling back to english" << std::endl;
	}
}

const TextManager* TextManager::getSingleton(){
	return mSingleton;
}

TextManager::TextManager(std::string l):lang(l){
	mStrings.resize(COUNT);
	setDefault();
}

void TextManager::switchLanguage(std::string langname){
	// if old and new language are the same, nothing must be done 
	if(langname == mSingleton->lang)
		return;
		
	// otherwise, the old TextManager is destroyed and a new one is created
	createTextManager(langname);
}

const std::string& TextManager::getString(STRING str) const{
	return mStrings[str];
}
std::string TextManager::getLang() const{
	return lang;
}

/// \todo why no const std::string& ?
bool TextManager::loadFromXML(std::string filename){
	// create and load file
	File file(filename, File::OPEN_READ);

	int fileLength = file.length();
	boost::shared_array<char> fileBuffer(new char[fileLength + 1]);
	file.readRawBytes( fileBuffer.get(), fileLength );
	// null-terminate
	fileBuffer[fileLength] = 0;
	
	// parse file
	TiXmlDocument language_data;
	language_data.Parse(fileBuffer.get());
	fileBuffer.reset(0);
	file.close();

	if (language_data.Error())
	{
		std::cerr << "Warning: Parse error in " << filename;
		std::cerr << "!" << std::endl;
	}

	TiXmlElement* language = language_data.FirstChildElement("language");
	if (!language)
		return false;
	
	int num_strings = mStrings.size();
	int found_count = 0;
	
	// this loop assumes that the strings in the xml file are in the correct order
	//  in each step, it reads the next string element and writes it to the next position in mStrings
	for (	TiXmlElement* stringel = language->FirstChildElement("string"); 
			stringel; 
			stringel = stringel->NextSiblingElement("string"))
	
	{
		
		/// \todo we don't check for duplicate entries!
		const char* e = stringel->Attribute("english");
		const char* t = stringel->Attribute("translation");
		if (t && e){
			// search the english string and replace it with the translation
			std::vector<std::string>::iterator found = std::find(mStrings.begin(), mStrings.end(), e);
			if(found != mStrings.end())
			{
				found_count++;
				*found = t;
			}
			else
				std::cerr << "error in language file: entry " << e << " -> " << t << " invalid\n";
		
		} 
		else if(t)
		{
			std::cerr << "error in language file: english not found for " << t << std::endl;
		}
		else if(e)
		{
			std::cerr << "error in language file: translation not found for " << e << std::endl;
		}
		
	}
	
	// do we check if we got all?
	if(num_strings != found_count)
	{
		std::cerr << "missing translations: got " << found_count << 
					" out of " << num_strings << " translation entries" << std::endl; 
	}
	
	return true;
}

void TextManager::setDefault()
{
	// Hardcoded default language
	mStrings[LBL_OK] = "ok";
	mStrings[LBL_CANCEL] = "cancel";
	mStrings[LBL_YES] = "yes";
	mStrings[LBL_NO] = "no";
	mStrings[LBL_CONF_QUIT] = "really quit?";
	mStrings[LBL_CONTINUE] = "continue";
	
	mStrings[MNU_LABEL_ONLINE] = "online game";
	mStrings[MNU_LABEL_LAN] = "lan game";
	mStrings[MNU_LABEL_START] = "start";
	mStrings[MNU_LABEL_OPTIONS] = "options";
	mStrings[MNU_LABEL_REPLAY] = "watch replay";
	mStrings[MNU_LABEL_CREDITS] = "credits";
	mStrings[MNU_LABEL_EXIT] = "exit";
	
	mStrings[CRD_PROGRAMMERS] = "programmers:";
	mStrings[CRD_GRAPHICS] = "graphics:";
	mStrings[CRD_THX] = "special thanks at:";
	
	mStrings[RP_SHOW_AGAIN] = "show again";
	mStrings[RP_PLAY] = "play";
	mStrings[RP_DELETE] = "delete";
	mStrings[RP_CHECKSUM] = "checksum error";
	mStrings[RP_FILE_CORRUPT] = "file is corrupt";
	mStrings[RP_VERSION] = "version error";
	mStrings[RP_FILE_OUTDATED] = "file is outdated";
	mStrings[RP_SAVE_NAME] = "name of the replay:";
	mStrings[RP_WAIT_REPLAY] = "receiving replay...";
	mStrings[RP_SAVE] = "save replay";
	
	mStrings[GAME_WIN] = "has won the game!";
	mStrings[GAME_TRY_AGAIN] = "try again";
	mStrings[GAME_WAITING] = "waiting for opponent...";
	mStrings[GAME_OPP_LEFT] = "opponent left the game";
	mStrings[GAME_PAUSED] = "game paused";
	mStrings[GAME_QUIT] = "quit";
	
	mStrings[NET_SERVER_SCAN] = "scan for servers";
	mStrings[NET_DIRECT_CONNECT] = "direct connect";
	mStrings[NET_SERVER_INFO] = "server info";
	mStrings[NET_ACTIVE_GAMES] = "active games: ";
	mStrings[NET_WAITING_PLAYER] = "waiting player: ";
	mStrings[NET_HOST_GAME] = "host game";
	mStrings[NET_CONNECTING] = "connecting to server ...";
	mStrings[NET_DISCONNECT] = "disconnected from server";
	mStrings[NET_CON_FAILED] = "connection failed";
	mStrings[NET_SERVER_FULL] = "server full";
	
	mStrings[OP_INPUT_OP] = "input options";
	mStrings[OP_GFX_OP] = "graphic options";
	mStrings[OP_MISC] = "misc options";
	mStrings[OP_VIDEO] = "video settings";
	mStrings[OP_FULLSCREEN] = "fullscreen mode";
	mStrings[OP_WINDOW] = "window mode";
	mStrings[OP_RENDER_DEVICE] = "render device";
	mStrings[OP_SHOW_SHADOW] = "show shadow";
	mStrings[OP_BLOB_COLORS] = "blob colors";
	mStrings[OP_LEFT_PLAYER] = "left player";
	mStrings[OP_RIGHT_PLAYER] = "right player";
	mStrings[OP_RED] = "red";
	mStrings[OP_GREEN] = "green";
	mStrings[OP_BLUE] = "blue";
	mStrings[OP_MORPHING] = "morphing blob?";
	mStrings[OP_KEYBOARD] = "keyboard";
	mStrings[OP_MOUSE] = "mouse";
	mStrings[OP_JOYSTICK] = "joystick";
	mStrings[OP_JUMP_BUTTON] = "jump button";
	mStrings[OP_SET_ALL] = "set all";
	mStrings[OP_LEFT_KEY] = "left key";
	mStrings[OP_RIGHT_KEY] = "right key";
	mStrings[OP_JUMP_KEY] = "jump key";
	mStrings[OP_LEFT_BUTTON] = "left button";
	mStrings[OP_RIGHT_BUTTON] = "right button";
	mStrings[OP_PRESS_MOUSE_BUTTON] = "press mouse button for";
	mStrings[OP_PRESS_KEY_FOR] = "press key for";
	mStrings[OP_MOVING_LEFT] = "moving left";
	mStrings[OP_MOVING_RIGHT] = "moving right";
	mStrings[OP_JUMPING] = "jumping";
	mStrings[OP_PRESS_BUTTON_FOR] = "press button for";
	mStrings[OP_BACKGROUND] = "background:";
	mStrings[OP_VOLUME] = "volume:";
	mStrings[OP_MUTE] = "mute";
	mStrings[OP_FPS] = "show fps";
	mStrings[OP_BLOOD] = "show blood";
	mStrings[OP_NETWORK_SIDE] = "network side:";
	mStrings[OP_LEFT] = "left";
	mStrings[OP_RIGHT] = "right";
	mStrings[OP_SPEED] = "gamespeed:";
	mStrings[OP_VSLOW] = "very slow";
	mStrings[OP_SLOW] = "slow";
	mStrings[OP_DEFAULT] = "default";
	mStrings[OP_FAST] = "fast";
	mStrings[OP_VFAST] = "very fast";
	mStrings[OP_LANGUAGE] = "language";
	mStrings[OP_DIFFICULTY] = "bot strength";
	mStrings[OP_WEAK] = "weak";
	mStrings[OP_MEDIUM] = "medium";
	mStrings[OP_STRONG] = "strong";
	
	mStrings[UPDATE_NOTIFICATION] = "please visit http://blobby.sourceforge.net/ for a new version of blobby volley";
}

std::map<std::string, std::string> TextManager::language_names;

struct lang_init{
	lang_init(){
		TextManager::language_names["de"] = "deutsch";
		TextManager::language_names["en"] = "english";
		TextManager::language_names["fr"] = "francais";
	}
};
static lang_init init;


