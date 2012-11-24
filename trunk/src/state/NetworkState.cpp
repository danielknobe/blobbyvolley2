/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

/* header include */

/* includes */
#include <algorithm>
#include <iostream>
#include <ctime>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/make_shared.hpp>

#include "raknet/RakClient.h"
#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include "NetworkState.h"
#include "NetworkMessage.h"
#include "NetworkGame.h"
#include "RakNetPacket.h"
#include "TextManager.h"
#include "ReplayRecorder.h"
#include "IMGUI.h"
#include "SoundManager.h"
#include "LocalInputSource.h"
#include "UserConfig.h"
#include "FileExceptions.h"
#include "GenericIO.h"
#include "FileRead.h"
#include "FileWrite.h"
#include "MatchEvents.h"


/* implementation */
NetworkGameState::NetworkGameState(const std::string& servername, Uint16 port):
	mClient(new RakClient()),
	mFakeMatch(new DuelMatch(true, "rules.lua")),
	mServerAddress(servername), mPort(port)
{	
	IMGUI::getSingleton().resetSelection();
	mWinningPlayer = NO_PLAYER;
	/// \todo we need read-only access here!
	UserConfig config;
	config.loadFile("config.xml");
	mOwnSide = (PlayerSide)config.getInteger("network_side");
	mUseRemoteColor = config.getBool("use_remote_color");
	mLocalInput.reset(new LocalInputSource(mOwnSide));
	mSaveReplay = false;
	mWaitingForReplay = false;
	mErrorMessage = "";

	RenderManager::getSingleton().redraw();

	if (mClient->Connect(servername.c_str(), port, 0, 0, RAKNET_THREAD_SLEEP_TIME))
		mNetworkState = CONNECTING;
	else
		mNetworkState = CONNECTION_FAILED;

	
	// game is not started until two players are connected 
	mFakeMatch->pause();
	
	// load/init players
	
	if(mOwnSide == LEFT_PLAYER)
	{
		PlayerIdentity localplayer = config.loadPlayerIdentity(LEFT_PLAYER, true);
		PlayerIdentity remoteplayer = config.loadPlayerIdentity(RIGHT_PLAYER, true);
		mLocalPlayer = &mFakeMatch->getPlayer( LEFT_PLAYER );
		mRemotePlayer = &mFakeMatch->getPlayer( RIGHT_PLAYER );
		mFakeMatch->setPlayers( localplayer, remoteplayer );
	}
	 else
	{
		PlayerIdentity localplayer = config.loadPlayerIdentity(RIGHT_PLAYER, true);
		PlayerIdentity remoteplayer = config.loadPlayerIdentity(LEFT_PLAYER, true);
		mLocalPlayer = &mFakeMatch->getPlayer( RIGHT_PLAYER );
		mRemotePlayer = &mFakeMatch->getPlayer( LEFT_PLAYER );
		mFakeMatch->setPlayers( remoteplayer, localplayer );
	}
	
	mRemotePlayer->setName("");
	
	RenderManager::getSingleton().setScore(0, 0, false, false);
	RenderManager::getSingleton().setPlayernames(mFakeMatch->getPlayer(LEFT_PLAYER).getName(), mFakeMatch->getPlayer(RIGHT_PLAYER).getName());

	mSelectedChatmessage = 0;
	mChatCursorPosition = 0;
	mChattext = "";
}

NetworkGameState::~NetworkGameState()
{
	mClient->Disconnect(50);
}

void NetworkGameState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	RenderManager* rmanager = &RenderManager::getSingleton();

	packet_ptr packet;
	while (packet = receivePacket(mClient.get()))
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
				stream.Write(mLocalPlayer->getStaticColor().toInt());

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
				DuelMatchState ms;
				boost::shared_ptr<GenericIn> in = createGenericReader(&stream);
				in->generic<DuelMatchState> (ms);
				mFakeMatch->setState(ms);
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
			case ID_UPDATE_SCORE:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_UPDATE_SCORE
				
				// read and set new score
				int nLeftScore;
				int nRightScore;
				int time;
				stream.Read(nLeftScore);
				stream.Read(nRightScore);
				stream.Read(time);
				mFakeMatch->setScore(nLeftScore, nRightScore);
				// sync the clocks... normally, they should not differ
				mFakeMatch->getClock().setTime(time);
				break;
			}
			case ID_BALL_RESET:
			{
				PlayerSide servingPlayer;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_BALL_RESET
				stream.Read((int&)servingPlayer);
				
				// read and set new score
				int nLeftScore;
				int nRightScore;
				int time;
				stream.Read(nLeftScore);
				stream.Read(nRightScore);
				stream.Read(time);
				mFakeMatch->setScore(nLeftScore, nRightScore);
				mFakeMatch->setServingPlayer(servingPlayer);
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
						mFakeMatch->trigger(EVENT_BALL_HIT_LEFT_GROUND);
						break;
					case RIGHT_PLAYER:
						mFakeMatch->trigger(EVENT_BALL_HIT_RIGHT_GROUND);
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
				stream.Read(intensity);
				mFakeMatch->setLastHitIntensity(intensity);
				stream.Read(side);
				switch((PlayerSide)side){
					case LEFT_PLAYER:
						mFakeMatch->trigger(EVENT_LEFT_BLOBBY_HIT);
						break;
					case RIGHT_PLAYER:
						mFakeMatch->trigger(EVENT_RIGHT_BLOBBY_HIT);
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

				mRemotePlayer->setName(charName);
				
				mFilename = mLocalPlayer->getName();
				if(mFilename.size() > 7)
					mFilename.resize(7);
				mFilename += " vs ";
				std::string oppname = mRemotePlayer->getName();
				if(oppname.size() > 7)
					oppname.resize(7);
				mFilename += oppname;
				
				// set names in render manager
				RenderManager::getSingleton().setPlayernames(mFakeMatch->getPlayer(LEFT_PLAYER).getName(), 
															mFakeMatch->getPlayer(RIGHT_PLAYER).getName());
				
				// check whether to use remote player color
				if(mUseRemoteColor)
				{
					mRemotePlayer->setStaticColor(ncolor);
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
			case ID_RULES_CHECKSUM:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				
				stream.IgnoreBytes(1);	// ignore ID_RULES_CHECKSUM
				
				int serverChecksum;
				stream.Read(serverChecksum);
				int ourChecksum = 0;
				if (serverChecksum != 0)
				{
					try
					{
						FileRead rulesFile("server_rules.lua");
						ourChecksum = rulesFile.calcChecksum(0);
						rulesFile.close();
					}
					catch( FileLoadException& ex )
					{
						// file doesn't exist - nothing to do here
					}
				}
				
				RakNet::BitStream stream2;
				stream2.Write((unsigned char)ID_RULES);
				stream2.Write(bool(serverChecksum != 0 && serverChecksum != ourChecksum));
				mClient->Send(&stream2, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
				
				break;
			}
			case ID_RULES:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				
				stream.IgnoreBytes(1);	// ignore ID_RULES
				
				int rulesLength;
				stream.Read(rulesLength);
				if (rulesLength)
				{
					boost::shared_array<char>  rulesString( new char[rulesLength + 1] );
					stream.Read(rulesString.get(), rulesLength);
					// null terminate
					rulesString[rulesLength] = 0;
					FileWrite rulesFile("server_rules.lua");
					rulesFile.write(rulesString.get(), rulesLength);
					rulesFile.close();
					mFakeMatch->setRules("server_rules.lua");
				} 
				else
				{
					// either old server, or we have to use fallback ruleset
					mFakeMatch->setRules( FALLBACK_RULES_NAME );
				}
				
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
				
				RakNet::BitStream stream = RakNet::BitStream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	// ID_REPLAY
				
				try 
				{
					boost::shared_ptr<GenericIn> reader = createGenericReader( &stream );
					ReplayRecorder dummyRec;
					dummyRec.receive( reader );
					
					boost::shared_ptr<FileWrite> fw = boost::make_shared<FileWrite>((std::string("replays/") + mFilename + std::string(".bvr")));
					dummyRec.save( fw );
				}
				 catch( FileLoadException& ex) 
				{
					mErrorMessage = std::string("Unable to create file:" + ex.getFileName());
					mSaveReplay = true;	// try again
				}
				 catch( FileAlreadyExistsException& ex) 
				{
					mErrorMessage = std::string("File already exists!:"+ ex.getFileName());
					mSaveReplay = true;
				}
				 catch( std::exception& ex) 
				{
					mErrorMessage = std::string("Could not save replay: ");
					// it is not expected to catch any exception here! save should only 
					// create FileLoad and FileAlreadyExists exceptions
					mSaveReplay = true;
				}
				
				// mWaitingForReplay will be set to false even if replay could not be saved because 
				// the server won't send it again.
				mWaitingForReplay = false;
				
				break;
			}
			default:
				printf("Received unknown Packet %d\n", packet->data[0]);
				std::cout<<packet->data<<"\n";
				break;
		}
	}

	presentGame(*mFakeMatch);

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
	else if (mErrorMessage != "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100, 200), Vector2(700, 360));
		size_t split = mErrorMessage.find(':');
		std::string mProblem = mErrorMessage.substr(0, split);
		std::string mInfo = mErrorMessage.substr(split+1);
		imgui.doText(GEN_ID, Vector2(120, 220), mProblem);
		imgui.doText(GEN_ID, Vector2(120, 260), mInfo);
		if(imgui.doButton(GEN_ID, Vector2(330, 320), TextManager::LBL_OK))
		{
			mErrorMessage = "";
		}
		imgui.doCursor();
	}
	else if (mSaveReplay)
	{
		imgui.doOverlay(GEN_ID, Vector2(150, 200), Vector2(650, 400));
		imgui.doText(GEN_ID, Vector2(190, 220), TextManager::RP_SAVE_NAME);
		static unsigned cpos;
		imgui.doEditbox(GEN_ID, Vector2(180, 270), 18, mFilename, cpos);
		if (imgui.doButton(GEN_ID, Vector2(220, 330), TextManager::LBL_OK))
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
		if (imgui.doButton(GEN_ID, Vector2(440, 330), TextManager::LBL_CANCEL))
		{
			mSaveReplay = false;
			imgui.resetSelection();
		}
		imgui.doCursor();
	} 
	else if (mWaitingForReplay)
	{
		imgui.doOverlay(GEN_ID, Vector2(150, 200), Vector2(650, 400));
		imgui.doText(GEN_ID, Vector2(190, 220), TextManager::RP_WAIT_REPLAY);
		if (imgui.doButton(GEN_ID, Vector2(440, 330), TextManager::LBL_CANCEL))
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
					TextManager::NET_CONNECTING);
			break;
		}
		case WAITING_FOR_OPPONENT:
		{
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 310.0));
			imgui.doText(GEN_ID, Vector2(150.0, 250.0),
					TextManager::GAME_WAITING);
			break;
		}
		case OPPONENT_DISCONNECTED:
		{
			imgui.doCursor();
			imgui.doOverlay(GEN_ID, Vector2(100.0, 205.0), Vector2(700.0, 380.0));
			imgui.doText(GEN_ID, Vector2(140.0, 245.0),	TextManager::GAME_OPP_LEFT);
			
			if (imgui.doButton(GEN_ID, Vector2(230.0, 295.0), TextManager::LBL_OK))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			
			if (imgui.doButton(GEN_ID, Vector2(350.0, 295.0), TextManager::RP_SAVE))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}
			
			if (imgui.doButton(GEN_ID, Vector2(250.0, 330.0), TextManager::NET_STAY_ON_SERVER))
			{
				// we need to make a copy here because this variables get deleted in destrutor
				// when deleteCurrentState runs
				std::string server = mServerAddress;
				uint16_t port = mPort;
				
				deleteCurrentState();
				setCurrentState(new NetworkGameState(server, port));
				return;
			}
			break;
		}
		case DISCONNECTED:
		{
			imgui.doCursor();
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 370.0));
			imgui.doText(GEN_ID, Vector2(120.0, 250.0),
					TextManager::NET_DISCONNECT);
			if (imgui.doButton(GEN_ID, Vector2(230.0, 320.0),
					TextManager::LBL_OK))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			if (imgui.doButton(GEN_ID, Vector2(350.0, 320.0), TextManager::RP_SAVE))
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
					TextManager::NET_CON_FAILED);
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0),
					TextManager::LBL_OK))
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
					TextManager::NET_SERVER_FULL);
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0),
					TextManager::LBL_OK))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			break;
		}
		case PLAYING:
		{
			mFakeMatch->step();

			PlayerInput input = mLocalInput->updateInput();

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
			std::string tmp = mFakeMatch->getPlayer(mWinningPlayer).getName();
			imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(700, 450));
			imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
			imgui.doText(GEN_ID, Vector2(274, 240), tmp);
			imgui.doText(GEN_ID, Vector2(274, 300), TextManager::GAME_WIN);
			if (imgui.doButton(GEN_ID, Vector2(290, 360), TextManager::LBL_OK))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState());
			}
			if (imgui.doButton(GEN_ID, Vector2(380, 360), TextManager::RP_SAVE))
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
			imgui.doText(GEN_ID, Vector2(275, 35), TextManager::GAME_PAUSED);
			if (imgui.doButton(GEN_ID, Vector2(205, 95), TextManager::LBL_CONTINUE))
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
			if (imgui.doButton(GEN_ID, Vector2(500, 95), TextManager::GAME_QUIT))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			if (imgui.doButton(GEN_ID, Vector2(285, 125), TextManager::RP_SAVE))
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
	mServer->Start(2, 0, BLOBBY_PORT);
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
						switchSide, "rules.lua");
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

