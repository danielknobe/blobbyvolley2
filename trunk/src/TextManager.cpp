#include "TextManager.h"

TextManager* TextManager::mSingleton = 0;


TextManager* TextManager::createTextManager(LANGUAGE l){
	if(mSingleton)
		delete mSingleton;
	mSingleton = new TextManager();
	
	switch(l){
		case ENGLISH:
			mSingleton->mStrings[LBL_OK] = "ok";
			mSingleton->mStrings[LBL_CANCEL] = "cancel";
			mSingleton->mStrings[LBL_YES] = "yes";
			mSingleton->mStrings[LBL_NO] = "no";
			mSingleton->mStrings[LBL_MAINMENU] = "back to main menu";
			mSingleton->mStrings[LBL_CONF_QUIT] = "really quit?";
			mSingleton->mStrings[LBL_CONTINUE] = "continue";
			
			mSingleton->mStrings[MNU_LABEL_NETWORK] = "network game";
			mSingleton->mStrings[MNU_LABEL_START] = "start";
			mSingleton->mStrings[MNU_LABEL_OPTIONS] = "options";
			mSingleton->mStrings[MNU_LABEL_REPLAY] = "watch replay";
			mSingleton->mStrings[MNU_LABEL_CREDITS] = "credits";
			mSingleton->mStrings[MNU_LABEL_EXIT] = "exit";
			
			mSingleton->mStrings[CRD_PROGRAMMERS] = "programmers:";
			mSingleton->mStrings[CRD_GRAPHICS] = "graphics:";
			mSingleton->mStrings[CRD_THX] = "special thanks at:";
			
			mSingleton->mStrings[RP_SHOW_AGAIN] = "show again";
			mSingleton->mStrings[RP_PLAY] = "play";
			mSingleton->mStrings[RP_DELETE] = "delete";
			mSingleton->mStrings[RP_CHECKSUM] = "checksum error";
			mSingleton->mStrings[RP_FILE_CORRUPT] = "file is corrupt";
			mSingleton->mStrings[RP_SAVE_NAME] = "name of the replay:";
			mSingleton->mStrings[RP_SAVE] = "save replay";
			
			mSingleton->mStrings[GAME_WIN] = "has won the game!";
			mSingleton->mStrings[GAME_TRY_AGAIN] = "try again";
			mSingleton->mStrings[GAME_WAITING] = "waiting for opponent...";
			mSingleton->mStrings[GAME_OPP_LEFT] = "opponent left the game";
			mSingleton->mStrings[GAME_PAUSED] = "game paused";
			mSingleton->mStrings[GAME_QUIT] = "quit";
			
			mSingleton->mStrings[NET_SERVER_SCAN] = "scan for servers";
			mSingleton->mStrings[NET_DIRECT_CONNECT] = "direct connect";
			mSingleton->mStrings[NET_SERVER_INFO] = "server info";
			mSingleton->mStrings[NET_ACTIVE_GAMES] = "active games: ";
			mSingleton->mStrings[NET_WAITING_PLAYER] = "waiting player: ";
			mSingleton->mStrings[NET_HOST_GAME] = "host game";
			mSingleton->mStrings[NET_CONNECTING] = "connecting to server ...";
			mSingleton->mStrings[NET_DISCONNECT] = "disconnected from server";
			mSingleton->mStrings[NET_CON_FAILED] = "connection failed";
			mSingleton->mStrings[NET_SERVER_FULL] = "server full";
			
			mSingleton->mStrings[OP_INPUT_OP] = "input options";
			mSingleton->mStrings[OP_GFX_OP] = "graphic options";
			mSingleton->mStrings[OP_MISC] = "misc options";
			mSingleton->mStrings[OP_VIDEO] = "video settings";
			mSingleton->mStrings[OP_FULLSCREEN] = "fullscreen mode";
			mSingleton->mStrings[OP_WINDOW] = "window mode";
			mSingleton->mStrings[OP_RENDER_DEVICE] = "render device";
			mSingleton->mStrings[OP_BLOB_COLORS] = "blob colors";
			mSingleton->mStrings[OP_LEFT_PLAYER] = "left player";
			mSingleton->mStrings[OP_RIGHT_PLAYER] = "right player";
			mSingleton->mStrings[OP_RED] = "red";
			mSingleton->mStrings[OP_GREEN] = "green";
			mSingleton->mStrings[OP_BLUE] = "blue";
			mSingleton->mStrings[OP_MORPHING] = "morphing blob?";
			mSingleton->mStrings[OP_JUMP_BUTTON] = "jump button";
			mSingleton->mStrings[OP_SET_ALL] = "set all";
			mSingleton->mStrings[OP_LEFT_KEY] = "left key";
			mSingleton->mStrings[OP_RIGHT_KEY] = "right key";
			mSingleton->mStrings[OP_JUMP_KEY] = "jump key";
			mSingleton->mStrings[OP_LEFT_BUTTON] = "left button";
			mSingleton->mStrings[OP_RIGHT_BUTTON] = "right_button";
			mSingleton->mStrings[OP_PRESS_MOUSE_BUTTON] = "press mouse button for";
			mSingleton->mStrings[OP_PRESS_KEY_FOR] = "press key for";
			mSingleton->mStrings[OP_MOVING_LEFT] = "moving left";
			mSingleton->mStrings[OP_MOVING_RIGHT] = "moving right";
			mSingleton->mStrings[OP_JUMPING] = "jumping";
			mSingleton->mStrings[OP_PRESS_BUTTON_FOR] = "press button for";
			mSingleton->mStrings[OP_BACKGROUND] = "background:";
			mSingleton->mStrings[OP_VOLUME] = "volume:";
			mSingleton->mStrings[OP_MUTE] = "mute";
			mSingleton->mStrings[OP_FPS] = "show fps";
			mSingleton->mStrings[OP_BLOOD] = "show blood";
			mSingleton->mStrings[OP_NETWORK_SIDE] = "network side:";
			mSingleton->mStrings[OP_LEFT] = "left";
			mSingleton->mStrings[OP_RIGHT] = "right";
			mSingleton->mStrings[OP_SPEED] = "gamespeed:";
			mSingleton->mStrings[OP_VSLOW] = "very slow";
			mSingleton->mStrings[OP_SLOW] = "slow";
			mSingleton->mStrings[OP_DEFAULT] = "default";
			mSingleton->mStrings[OP_FAST] = "fast";
			mSingleton->mStrings[OP_VFAST] = "very fast";
			
			break;
		case GERMAN:
			mSingleton->mStrings[LBL_OK] = "ok";
			mSingleton->mStrings[LBL_CANCEL] = "abbrechen";
			mSingleton->mStrings[LBL_YES] = "ja";
			mSingleton->mStrings[LBL_NO] = "nein";
			mSingleton->mStrings[LBL_MAINMENU] = "hauptmenü";
			mSingleton->mStrings[LBL_CONF_QUIT] = "wirklich beenden?";
			mSingleton->mStrings[LBL_CONTINUE] = "fortsetzen";
			
			mSingleton->mStrings[MNU_LABEL_NETWORK] = "netzwerkspiel";
			mSingleton->mStrings[MNU_LABEL_START] = "start";
			mSingleton->mStrings[MNU_LABEL_OPTIONS] = "optionen";
			mSingleton->mStrings[MNU_LABEL_REPLAY] = "spiel ansehen";
			mSingleton->mStrings[MNU_LABEL_CREDITS] = "credits";
			mSingleton->mStrings[MNU_LABEL_EXIT] = "beenden";
			
			mSingleton->mStrings[CRD_PROGRAMMERS] = "programmierer:";
			mSingleton->mStrings[CRD_GRAPHICS] = "grafik:";
			mSingleton->mStrings[CRD_THX] = "besonderer dank an:";
			
			mSingleton->mStrings[RP_SHOW_AGAIN] = "nochmal ansehen";
			mSingleton->mStrings[RP_PLAY] = "abspielen";
			mSingleton->mStrings[RP_DELETE] = "löschen";
			mSingleton->mStrings[RP_CHECKSUM] = "checksum error";
			mSingleton->mStrings[RP_FILE_CORRUPT] = "datei beschädigt";
			mSingleton->mStrings[RP_SAVE_NAME] = "name des replays:";
			mSingleton->mStrings[RP_SAVE] = "replay speichern";
			
			
			mSingleton->mStrings[GAME_WIN] = "hat gewonnen!";
			mSingleton->mStrings[GAME_TRY_AGAIN] = "nochmal";
			mSingleton->mStrings[GAME_WAITING] = "warte auf mitspieler";
			mSingleton->mStrings[GAME_OPP_LEFT] = "gegner hat das spiel verlassen";
			mSingleton->mStrings[GAME_PAUSED] = "spiel pausiert";
			mSingleton->mStrings[GAME_QUIT] = "verlassen";
			
			mSingleton->mStrings[NET_SERVER_SCAN] = "server suchen";
			mSingleton->mStrings[NET_DIRECT_CONNECT] = "direktverbindung";
			mSingleton->mStrings[NET_SERVER_INFO] = "server info";
			mSingleton->mStrings[NET_ACTIVE_GAMES] = "aktive spiel: ";
			mSingleton->mStrings[NET_WAITING_PLAYER] = "wartender spieler: ";
			mSingleton->mStrings[NET_HOST_GAME] = "spiel hosten";
			mSingleton->mStrings[NET_CONNECTING] = "verbindungsaufbau...";
			mSingleton->mStrings[NET_DISCONNECT] = "verbindung unterbrochen";
			mSingleton->mStrings[NET_CON_FAILED] = "verbindungsfehler";
			mSingleton->mStrings[NET_SERVER_FULL] = "server voll";
			
			mSingleton->mStrings[OP_INPUT_OP] = "eingabeoptionen";
			mSingleton->mStrings[OP_GFX_OP] = "grafikoptionen";
			mSingleton->mStrings[OP_MISC] = "weitere optionen";
			mSingleton->mStrings[OP_VIDEO] = "video";
			mSingleton->mStrings[OP_FULLSCREEN] = "vollbild";
			mSingleton->mStrings[OP_WINDOW] = "fenster";
			mSingleton->mStrings[OP_RENDER_DEVICE] = "render device";
			mSingleton->mStrings[OP_BLOB_COLORS] = "blobbyfarben";
			mSingleton->mStrings[OP_LEFT_PLAYER] = "linker spieler";
			mSingleton->mStrings[OP_RIGHT_PLAYER] = "rechter spieler";
			mSingleton->mStrings[OP_RED] = "rot";
			mSingleton->mStrings[OP_GREEN] = "grün";
			mSingleton->mStrings[OP_BLUE] = "blau";
			mSingleton->mStrings[OP_MORPHING] = "morphing blob?";
			mSingleton->mStrings[OP_JUMP_BUTTON] = "springen";
			mSingleton->mStrings[OP_SET_ALL] = "alle setzen";
			mSingleton->mStrings[OP_LEFT_KEY] = "taste links";
			mSingleton->mStrings[OP_RIGHT_KEY] = "taste rechts";
			mSingleton->mStrings[OP_JUMP_KEY] = "taste sprung";
			mSingleton->mStrings[OP_LEFT_BUTTON] = "button links";
			mSingleton->mStrings[OP_RIGHT_BUTTON] = "button rechts";
			mSingleton->mStrings[OP_PRESS_MOUSE_BUTTON] = "maustaste drücken für";
			mSingleton->mStrings[OP_PRESS_KEY_FOR] = "taste drücken für";
			mSingleton->mStrings[OP_MOVING_LEFT] = "nach links";
			mSingleton->mStrings[OP_MOVING_RIGHT] = "nach rechts";
			mSingleton->mStrings[OP_JUMPING] = "springen";
			mSingleton->mStrings[OP_PRESS_BUTTON_FOR] = "taste drücken für";
			mSingleton->mStrings[OP_BACKGROUND] = "hintergrund:";
			mSingleton->mStrings[OP_VOLUME] = "lautstärke:";
			mSingleton->mStrings[OP_MUTE] = "stumm";
			mSingleton->mStrings[OP_FPS] = "fps";
			mSingleton->mStrings[OP_BLOOD] = "zeige blut";
			mSingleton->mStrings[OP_NETWORK_SIDE] = "netzwerkseite:";
			mSingleton->mStrings[OP_LEFT] = "links";
			mSingleton->mStrings[OP_RIGHT] = "rechts";
			mSingleton->mStrings[OP_SPEED] = "Geschwindigkeit:";
			mSingleton->mStrings[OP_VSLOW] = "sehr langsam";
			mSingleton->mStrings[OP_SLOW] = "langsam";
			mSingleton->mStrings[OP_DEFAULT] = "normal";
			mSingleton->mStrings[OP_FAST] = "schnell";
			mSingleton->mStrings[OP_VFAST] = "sehr schnell";
			break;
	}
}

const TextManager* TextManager::getSingleton(){
	return mSingleton;
}

TextManager::TextManager(){
	mStrings.resize(COUNT);
}

const std::string& TextManager::getString(STRING str) const{
	return mStrings[str];
}
