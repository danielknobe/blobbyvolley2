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

#include <algorithm>
#include <iostream>
#include <ctime>

#include "NetworkState.h"
#include "NetworkMessage.h"
#include "NetworkGame.h"
#include "TextManager.h"
#include "FileWrite.h"

#include "raknet/RakClient.h"
#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include <boost/lexical_cast.hpp>

#include "IMGUI.h"
#include "SoundManager.h"
#include "LocalInputSource.h"
#include "RakNetPacket.h"
#include "UserConfig.h"
// We don't need the stringcompressor

NetworkGameState::NetworkGameState(const std::string& servername, Uint16 port):
	mLeftPlayer(LEFT_PLAYER),
	mRightPlayer(RIGHT_PLAYER)
{	
	IMGUI::getSingleton().resetSelection();
	mWinningPlayer = NO_PLAYER;
	/// \todo we need read-only access here!
	UserConfig config;
	config.loadFile("config.xml");
	mOwnSide = (PlayerSide)config.getInteger("network_side");
	mUseRemoteColor = config.getBool("use_remote_color");
	mLocalInput = new LocalInputSource(mOwnSide);
	mSaveReplay = false;
	mWaitingForReplay = false;
	mFilename = boost::lexical_cast<std::string> (std::time(0));

	RenderManager::getSingleton().redraw();

	mClient = new RakClient();
	if (mClient->Connect(servername.c_str(), port, 0, 0, RAKNET_THREAD_SLEEP_TIME))
		mNetworkState = CONNECTING;
	else
		mNetworkState = CONNECTION_FAILED;

	mFakeMatch = new DuelMatch(0, 0, true, true);
	// game is not started until two players are connected 
	mFakeMatch->pause();
	
	// load/init players
	mLeftPlayer.loadFromConfig("left", false);
	mRightPlayer.loadFromConfig("right", false);
	if(mOwnSide == LEFT_PLAYER){
		mRightPlayer.setName("");
		mLocalPlayer = &mLeftPlayer;
		mRemotePlayer = &mRightPlayer;
	}else{
		mLeftPlayer.setName("");
		mLocalPlayer = &mRightPlayer;
		mRemotePlayer = &mLeftPlayer;
	}
	
	RenderManager::getSingleton().setScore(0, 0, false, false);
	RenderManager::getSingleton().setPlayernames(mLeftPlayer.getName(), mRightPlayer.getName());

	mSelectedChatmessage = 0;
	mChatCursorPosition = 0;
	mChattext = "";
}

NetworkGameState::~NetworkGameState()
{
	mClient->Disconnect(50);
	delete mLocalInput;
	delete mClient;
	delete mFakeMatch;
}

void NetworkGameState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	RenderManager* rmanager = &RenderManager::getSingleton();

	packet_ptr packet;
	while (packet = receivePacket(mClient))
	{
		switch(packet->data[0])
		{
			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_ENTER_GAME);
				stream.Write(mOwnSide);

				// Send playername
				char myname[16];
				strncpy(myname, mLocalPlayer->getName().c_str(), sizeof(myname));
				stream.Write(myname, sizeof(myname));

				// send color settings
				stream.Write(mLocalPlayer->getColor().toInt());

				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);

				mNetworkState = WAITING_FOR_OPPONENT;
				break;
			}
			case ID_PHYSIC_UPDATE:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				int ival;
				stream.IgnoreBytes(1);	//ID_PHYSIC_UPDATE
				stream.IgnoreBytes(1);	//ID_TIMESTAMP
				stream.Read(ival);	//TODO: un-lag based on timestamp delta
				//printf("Physic packet received. Time: %d\n", ival);
				mFakeMatch->setState(&stream);
				break;
			}
			case ID_WIN_NOTIFICATION:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_WIN_NOTIFICATION
				stream.Read((int&)mWinningPlayer);
				
				// last point must not be added anymore, because
				// the score is also simulated local so it is already
				// right. under strange circumstances this need not
				// be true, but then the score is set to the correy value
				// by ID_BALL_RESET
					
				mNetworkState = PLAYER_WON;
				break;
			}
			case ID_OPPONENT_DISCONNECTED:
			{
				// In this state, a leaving opponent would not be very surprising
				if (mNetworkState != PLAYER_WON)
					mNetworkState = OPPONENT_DISCONNECTED;
				break;
			}
			case ID_BALL_RESET:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_BALL_RESET
				stream.Read((int&)mServingPlayer);
				
				// read and set new score
				int nLeftScore;
				int nRightScore;
				int time;
				stream.Read(nLeftScore);
				stream.Read(nRightScore);
				stream.Read(time);
				mFakeMatch->setScore(nLeftScore, nRightScore);
				mFakeMatch->setServingPlayer(mServingPlayer);
				// sync the clocks... normally, they should not differ
				mFakeMatch->getClock().setTime(time);
				
				/// \attention
				/// we can get a problem here:
				/// assume the packet informing about the game event which lead to this
				///	either BALL_GROUND_COLLISION or BALL_PLAYER_COLLISION got stalled
				/// and arrives at the same time time as this packet. Then we get the following behaviour:
				/// we set the score to the right value... the event causing the score to happen gets processed
				///  -> that player scores -> score is off!
				///
				/// i don't have a clean fix for this right now, so we'll have to live with a workaround for now
				/// we just order the game to reset all triggered events.
				mFakeMatch->resetTriggeredEvents();
				/// \todo a good fix would involve ensuring we process all events in the right order
				
			
				break;
			}
			case ID_BALL_GROUND_COLLISION:
			{
				int side;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_BALL_GROUND_COLLISION
				stream.Read(side);
				switch((PlayerSide)side){
					case LEFT_PLAYER:
						mFakeMatch->trigger(DuelMatch::EVENT_BALL_HIT_LEFT_GROUND);
						break;
					case RIGHT_PLAYER:
						mFakeMatch->trigger(DuelMatch::EVENT_BALL_HIT_RIGHT_GROUND);
						break;
					default:
						assert(0);
				}
				break;
			}
			case ID_BALL_PLAYER_COLLISION:
			{
				float intensity;
				int side;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_PLAYER_BALL_COLLISION
				// FIXME ensure that this intensity is used
				stream.Read(intensity);
				stream.Read(side);
				switch((PlayerSide)side){
					case LEFT_PLAYER:
						mFakeMatch->trigger(DuelMatch::EVENT_LEFT_BLOBBY_HIT);
						break;
					case RIGHT_PLAYER:
						mFakeMatch->trigger(DuelMatch::EVENT_RIGHT_BLOBBY_HIT);
						break;
					default:
						assert(0);
				}
				break;
			}
			case ID_PAUSE:
				if (mNetworkState == PLAYING)
				{
					mNetworkState = PAUSING;
					mFakeMatch->pause();
				}
				break;
			case ID_UNPAUSE:
				if (mNetworkState == PAUSING)
				{
					mNetworkState = PLAYING;
					mFakeMatch->unpause();
				}
				break;
			case ID_GAME_READY:
			{
				char charName[16];
				RakNet::BitStream stream((char*)packet->data, packet->length, false);

				stream.IgnoreBytes(1);	// ignore ID_GAME_READY

				// read gamespeed
				int speed;
				stream.Read(speed);
				SpeedController::getMainInstance()->setGameSpeed(speed);
				
				// read playername
				stream.Read(charName, sizeof(charName));

				// ensures that charName is null terminated
				charName[sizeof(charName)-1] = '\0';

				// read colors
				int temp;
				stream.Read(temp);
				Color ncolor = temp;

				mRemotePlayer->setName(std::string(charName));
				
				// set names in render manager
				RenderManager::getSingleton().setPlayernames(mLeftPlayer.getName(), mRightPlayer.getName());
				
				// check whether to use remote player color
				if(mUseRemoteColor){
					mRemotePlayer->setColor(ncolor);
					RenderManager::getSingleton().setBlobColor(LEFT_PLAYER, mLeftPlayer.getColor());
					RenderManager::getSingleton().setBlobColor(RIGHT_PLAYER, mRightPlayer.getColor());
					RenderManager::getSingleton().redraw();
				}
				
				// Workarround for SDL-Renderer
				// Hides the GUI when networkgame starts
				rmanager->redraw();	

				mNetworkState = PLAYING;
				// start game
				mFakeMatch->unpause();
				
				// game ready whistle
				SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
				break;
			}
			case ID_CONNECTION_ATTEMPT_FAILED:
				mNetworkState = CONNECTION_FAILED;
				break;
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			case ID_REMOTE_CONNECTION_LOST:
				break;
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_CONNECTION_LOST:
				if (mNetworkState != PLAYER_WON)
					mNetworkState = DISCONNECTED;
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				mNetworkState = SERVER_FULL;
				break;
			case ID_RECEIVED_STATIC_DATA:
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				break;
			case ID_REMOTE_EXISTING_CONNECTION:
				break;
			case ID_CHAT_MESSAGE:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	// ID_CHAT_MESSAGE
				// Insert Message in the log and focus the last element
				char message[31];
				stream.Read(message, sizeof(message));
				message[30] = '\0';

				// Insert Message in the log and focus the last element
				mChatlog.push_back((std::string) message);
				mChatOrigin.push_back(false);
				mSelectedChatmessage = mChatlog.size() - 1;
				SoundManager::getSingleton().playSound("sounds/chat.wav", ROUND_START_SOUND_VOLUME);
				break;
			}
			case ID_REPLAY:
			{
				/// \todo we should take more action if server sends replay
				///		even if not requested!
				if(!mWaitingForReplay)
					break;
				
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	// ID_REPLAY
				int length;
				stream.Read(length);
				char* data = new char[length];
				stream.Read(data, length);
				// may throw!
				FileWrite file((std::string("replays/") + mFilename + std::string(".bvr")));
				file.write(data, length);
				file.close();
				mWaitingForReplay = false;
				break;
			}
			default:
				printf("Received unknown Packet %d\n", packet->data[0]);
				std::cout<<packet->data<<"\n";
				break;
		}
	}

	PlayerInput input = mNetworkState == PLAYING ?
		mLocalInput->getInput() : PlayerInput();

	presentGame(*mFakeMatch);
	rmanager->setBlobColor(LEFT_PLAYER, mLeftPlayer.getColor());
	rmanager->setBlobColor(RIGHT_PLAYER, mRightPlayer.getColor());

	if (InputManager::getSingleton()->exit() && mNetworkState != PLAYING)
	{
		if(mNetworkState == PAUSING)
		{
			// end pause
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_UNPAUSE);
			mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
		} 
		else
		{
			deleteCurrentState();
			setCurrentState(new MainMenuState);
		}
	}
	else if (InputManager::getSingleton()->exit() && mSaveReplay)
	{
		mSaveReplay = false;
		IMGUI::getSingleton().resetSelection();
	}
	else if (mSaveReplay)
	{
		imgui.doOverlay(GEN_ID, Vector2(150, 200), Vector2(650, 400));
		imgui.doText(GEN_ID, Vector2(190, 220), TextManager::getSingleton()->getString(TextManager::RP_SAVE_NAME));
		static unsigned cpos;
		imgui.doEditbox(GEN_ID, Vector2(180, 270), 18, mFilename, cpos);
		if (imgui.doButton(GEN_ID, Vector2(220, 330), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
		{
			if (mFilename != "")
			{
				// request replay from server
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_REPLAY);
				mClient->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0);
			}
			mSaveReplay = false;
			mWaitingForReplay = true;
			imgui.resetSelection();
		}
		if (imgui.doButton(GEN_ID, Vector2(440, 330), TextManager::getSingleton()->getString(TextManager::LBL_CANCEL)))
		{
			mSaveReplay = false;
			imgui.resetSelection();
		}
		imgui.doCursor();
	} 
	else if (mWaitingForReplay)
	{
		imgui.doOverlay(GEN_ID, Vector2(150, 200), Vector2(650, 400));
		imgui.doText(GEN_ID, Vector2(190, 220), TextManager::getSingleton()->getString(TextManager::RP_WAIT_REPLAY));
		if (imgui.doButton(GEN_ID, Vector2(440, 330), TextManager::getSingleton()->getString(TextManager::LBL_CANCEL)))
		{
			mSaveReplay = false;
			mWaitingForReplay = false;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else switch (mNetworkState)
	{
		case CONNECTING:
		{
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 310.0));
			imgui.doText(GEN_ID, Vector2(150.0, 250.0),
					TextManager::getSingleton()->getString(TextManager::NET_CONNECTING));
			break;
		}
		case WAITING_FOR_OPPONENT:
		{
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 310.0));
			imgui.doText(GEN_ID, Vector2(150.0, 250.0),
					TextManager::getSingleton()->getString(TextManager::GAME_WAITING));
			break;
		}
		case OPPONENT_DISCONNECTED:
		{
			imgui.doCursor();
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 370.0));
			imgui.doText(GEN_ID, Vector2(140.0, 250.0),
					TextManager::getSingleton()->getString(TextManager::GAME_OPP_LEFT));
			if (imgui.doButton(GEN_ID, Vector2(230.0, 300.0),
					TextManager::getSingleton()->getString(TextManager::LBL_OK)))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0), TextManager::getSingleton()->getString(TextManager::RP_SAVE)))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}
			break;
		}
		case DISCONNECTED:
		{
			imgui.doCursor();
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 370.0));
			imgui.doText(GEN_ID, Vector2(120.0, 250.0),
					TextManager::getSingleton()->getString(TextManager::NET_DISCONNECT));
			if (imgui.doButton(GEN_ID, Vector2(230.0, 320.0),
					TextManager::getSingleton()->getString(TextManager::LBL_OK)))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			if (imgui.doButton(GEN_ID, Vector2(350.0, 320.0), TextManager::getSingleton()->getString(TextManager::RP_SAVE)))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}
			break;
		}
		case CONNECTION_FAILED:
		{
			imgui.doCursor();
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 370.0));
			imgui.doText(GEN_ID, Vector2(200.0, 250.0),
					TextManager::getSingleton()->getString(TextManager::NET_CON_FAILED));
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0),
					TextManager::getSingleton()->getString(TextManager::LBL_OK)))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			break;
		}
		case SERVER_FULL:
		{
			imgui.doCursor();
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 370.0));
			imgui.doText(GEN_ID, Vector2(200.0, 250.0),
					TextManager::getSingleton()->getString(TextManager::NET_SERVER_FULL));
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0),
					TextManager::getSingleton()->getString(TextManager::LBL_OK)))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			break;
		}
		case PLAYING:
		{
			mFakeMatch->step();

			if (InputManager::getSingleton()->exit())
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_PAUSE);
				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
			}
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_INPUT_UPDATE);
			stream.Write((unsigned char)ID_TIMESTAMP);	///! \todo do we really need this time stamps?
			stream.Write(RakNet::GetTime());
			stream.Write(input.left);
			stream.Write(input.right);
			stream.Write(input.up);
			mClient->Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
			break;
		}
		case PLAYER_WON:
		{
			std::string tmp;
			if(mWinningPlayer==LEFT_PLAYER)
				tmp = mLeftPlayer.getName();
			else
				tmp = mRightPlayer.getName();
			imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(700, 450));
			imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
			imgui.doText(GEN_ID, Vector2(274, 240), tmp);
			imgui.doText(GEN_ID, Vector2(274, 300), TextManager::getSingleton()->getString(TextManager::GAME_WIN));
			if (imgui.doButton(GEN_ID, Vector2(290, 360), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState());
			}
			if (imgui.doButton(GEN_ID, Vector2(380, 360), TextManager::getSingleton()->getString(TextManager::RP_SAVE)))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}
			imgui.doCursor();
			break;
		}
		case PAUSING:
		{
			imgui.doOverlay(GEN_ID, Vector2(175, 20), Vector2(625, 175));
			imgui.doText(GEN_ID, Vector2(275, 35), TextManager::getSingleton()->getString(TextManager::GAME_PAUSED));
			if (imgui.doButton(GEN_ID, Vector2(205, 95), TextManager::getSingleton()->getString(TextManager::LBL_CONTINUE)))
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_UNPAUSE);
				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
			}
			// Chat
			imgui.doChatbox(GEN_ID, Vector2(10, 190), Vector2(790, 450), mChatlog, mSelectedChatmessage, mChatOrigin);
			if (imgui.doEditbox(GEN_ID, Vector2(30, 460), 30, mChattext, mChatCursorPosition, 0, true))
			{

				// GUI-Hack, so that we can send messages
				if ((InputManager::getSingleton()->getLastActionKey() == "return") && (mChattext != ""))
				{
					RakNet::BitStream stream;
					char message[31];
					
					strncpy(message, mChattext.c_str(), sizeof(message));
					stream.Write((unsigned char)ID_CHAT_MESSAGE);
					stream.Write(message, sizeof(message));
					mClient->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0);
					mChatlog.push_back(mChattext);
					mChatOrigin.push_back(true);
					mSelectedChatmessage = mChatlog.size() - 1;
					mChattext = "";
					mChatCursorPosition = 0;
					SoundManager::getSingleton().playSound("sounds/chat.wav", ROUND_START_SOUND_VOLUME);
				}
			}
			if (imgui.doButton(GEN_ID, Vector2(500, 95), TextManager::getSingleton()->getString(TextManager::GAME_QUIT)))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			if (imgui.doButton(GEN_ID, Vector2(285, 125), TextManager::getSingleton()->getString(TextManager::RP_SAVE)))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}
			imgui.doCursor();
		}
	}
}

const char* NetworkGameState::getStateName() const
{
	return "NetworkGameState";
}

NetworkHostState::NetworkHostState()
{
	mServer = new RakServer;
	mServer->Start(2, 0, 0, BLOBBY_PORT);
	mNetworkGame = 0;
	mGameState = new NetworkGameState("localhost", BLOBBY_PORT);
	mLocalPlayerSide = NO_PLAYER;
}

NetworkHostState::~NetworkHostState()
{
	delete mGameState;
	delete mNetworkGame;
	mServer->Disconnect(1);
	delete mServer;
}

void NetworkHostState::step()
{
	packet_ptr packet;
	while (packet = receivePacket(mServer))
	{
		switch (packet->data[0])
		{
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_CONNECTION_LOST:
			case ID_INPUT_UPDATE:
			case ID_CHAT_MESSAGE:
			case ID_PAUSE:
			case ID_UNPAUSE:
			case ID_REPLAY:
			{
				if (packet->playerId == mLocalPlayer ||
					packet->playerId == mRemotePlayer)
				{
					if (mNetworkGame)
						mNetworkGame->injectPacket(packet);
				}
				break;
			}
			case ID_BLOBBY_SERVER_PRESENT:
			{
				ServerInfo myinfo(mLocalPlayerSide == NO_PLAYER ? "somebody" : mLocalPlayerName.c_str());
				myinfo.activegames = mNetworkGame ? 1 : 0;
				if (mLocalPlayerSide == NO_PLAYER || mNetworkGame)
				{
					strncpy(myinfo.waitingplayer, "none",
						sizeof(myinfo.waitingplayer) - 1);
				}
				else
				{
					strncpy(myinfo.waitingplayer, mLocalPlayerName.c_str(),
						sizeof(myinfo.waitingplayer) - 1);
				}
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_BLOBBY_SERVER_PRESENT);
				myinfo.writeToBitstream(stream);
				mServer->Send(&stream, HIGH_PRIORITY,
						RELIABLE_ORDERED, 0,
						packet->playerId, false);
				break;
			}
			case ID_ENTER_GAME:
			{
				int ival;
				RakNet::BitStream stream((char*)packet->data,
						packet->length, false);

				stream.IgnoreBytes(1);	//ID_ENTER_GAME

				// read playername and side
				stream.Read(ival);

				char charName[16];
				stream.Read(charName, sizeof(charName));

				// read colour data
				int color;
				stream.Read(color);

				// ensures that charName is null terminated
				charName[sizeof(charName)-1] = '\0';

				std::string playerName(charName);
				PlayerSide newSide = (PlayerSide)ival;

				if (mLocalPlayerSide == NO_PLAYER)
				{ // First player is probably the local one
					mLocalPlayerSide = newSide;
					mLocalPlayer = packet->playerId;
					mLocalPlayerName = playerName;

					// set the color
					switch(newSide){
						case LEFT_PLAYER:
							mLeftColor = color;
							break;
						case RIGHT_PLAYER:
							mRightColor = color;
							break;
					}
				}
				else
				{
					mRemotePlayer = packet->playerId;
					mRemotePlayerName = playerName;

					PlayerID leftPlayer;
					PlayerID rightPlayer;
					std::string leftPlayerName;
					std::string rightPlayerName;

					if (LEFT_PLAYER == mLocalPlayerSide)
					{
						leftPlayer = mLocalPlayer;
						rightPlayer = packet->playerId;
						leftPlayerName = mLocalPlayerName;
						rightPlayerName = playerName;
						// set other color
						mRightColor = color;
					}
					else
					{
						leftPlayer = packet->playerId;
						rightPlayer = mLocalPlayer;
						leftPlayerName = playerName;
						rightPlayerName = mLocalPlayerName;
						// set other color
						mLeftColor = color;
					}

					PlayerSide switchSide = NO_PLAYER;

					if (newSide == mLocalPlayerSide)
					{
						if (newSide == LEFT_PLAYER)
							switchSide = RIGHT_PLAYER;
						if (newSide == RIGHT_PLAYER)
							switchSide = LEFT_PLAYER;
					}
					mNetworkGame = new NetworkGame(
						*mServer, leftPlayer, rightPlayer,
						leftPlayerName, rightPlayerName,
						mLeftColor, mRightColor,
						switchSide);
				}
			}
		}
	}
	mGameState->step();
	if (dynamic_cast<NetworkHostState*>(getCurrentState()) != 0)
	{
		if (mNetworkGame)
		{
				mNetworkGame->step();
		}
	}
}

const char* NetworkHostState::getStateName() const
{
	return "NetworkHostState";
}

