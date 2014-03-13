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
#include "TextManager.h"
#include "ReplayRecorder.h"
#include "DuelMatch.h"
#include "IMGUI.h"
#include "SoundManager.h"
#include "LocalInputSource.h"
#include "UserConfig.h"
#include "FileExceptions.h"
#include "GenericIO.h"
#include "FileRead.h"
#include "FileWrite.h"
#include "MatchEvents.h"
#include "SpeedController.h"
#include "server/DedicatedServer.h"
#include "LobbyState.h"
#include "InputManager.h"


/* implementation */
NetworkGameState::NetworkGameState( boost::shared_ptr<RakClient> client):
	 GameState(new DuelMatch(true, DEFAULT_RULES_FILE)),
	 mClient( client ),
	 mWinningPlayer(NO_PLAYER),
	 mNetworkState(WAITING_FOR_OPPONENT),
	 mWaitingForReplay(false),
	 mSelectedChatmessage(0),
	 mChatCursorPosition(0),
	 mChattext("")
{
	IMGUI::getSingleton().resetSelection();
	/// \todo we need read-only access here!
	UserConfig config;
	config.loadFile("config.xml");
	mOwnSide = (PlayerSide)config.getInteger("network_side");
	mUseRemoteColor = config.getBool("use_remote_color");
	mLocalInput.reset(new LocalInputSource(mOwnSide));
	mLocalInput->setMatch(mMatch.get());

	/// \todo why do we need this here?
	RenderManager::getSingleton().redraw();

	// game is not started until two players are connected
	mMatch->pause();

	// load/init players

	if(mOwnSide == LEFT_PLAYER)
	{
		PlayerIdentity localplayer = config.loadPlayerIdentity(LEFT_PLAYER, true);
		PlayerIdentity remoteplayer = config.loadPlayerIdentity(RIGHT_PLAYER, true);
		mLocalPlayer = &mMatch->getPlayer( LEFT_PLAYER );
		mRemotePlayer = &mMatch->getPlayer( RIGHT_PLAYER );
		mMatch->setPlayers( localplayer, remoteplayer );
	}
	 else
	{
		PlayerIdentity localplayer = config.loadPlayerIdentity(RIGHT_PLAYER, true);
		PlayerIdentity remoteplayer = config.loadPlayerIdentity(LEFT_PLAYER, true);
		mLocalPlayer = &mMatch->getPlayer( RIGHT_PLAYER );
		mRemotePlayer = &mMatch->getPlayer( LEFT_PLAYER );
		mMatch->setPlayers( remoteplayer, localplayer );
	}

	mRemotePlayer->setName("");
}

NetworkGameState::~NetworkGameState()
{
	mClient->Disconnect(50);
}

void NetworkGameState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();
	RenderManager* rmanager = &RenderManager::getSingleton();

	packet_ptr packet;
	while (packet = mClient->Receive())
	{
		switch(packet->data[0])
		{
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
				mMatch->setState(ms);
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
				mMatch->setScore(nLeftScore, nRightScore);
				mMatch->setServingPlayer(servingPlayer);
				// sync the clocks... normally, they should not differ
				mMatch->getClock().setTime(time);

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
				mMatch->resetTriggeredEvents();
				/// \todo a good fix would involve ensuring we process all events in the right order


				break;
			}
			case ID_COLLISION:
			{
				int event;
				float intensity;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_COLLISION
				stream.Read(event);
				stream.Read(intensity);

				mMatch->setLastHitIntensity(intensity);
				mMatch->trigger( event );
				break;
			}
			case ID_PAUSE:
				if (mNetworkState == PLAYING)
				{
					mNetworkState = PAUSING;
					mMatch->pause();
				}
				break;
			case ID_UNPAUSE:
				if (mNetworkState == PAUSING)
				{
					mNetworkState = PLAYING;
					mMatch->unpause();
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

				setDefaultReplayName(mLocalPlayer->getName(), mRemotePlayer->getName());

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
				mMatch->unpause();

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
					mMatch->setRules("server_rules.lua");
				}
				else
				{
					// either old server, or we have to use fallback ruleset
					mMatch->setRules( FALLBACK_RULES_NAME );
				}

				break;
			}
			// status messages we don't care about
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			case ID_REMOTE_CONNECTION_LOST:
			case ID_SERVER_STATUS:
			case ID_CHALLENGE:
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
			case ID_REMOTE_EXISTING_CONNECTION:
				break;
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_CONNECTION_LOST:
				if (mNetworkState != PLAYER_WON)
					mNetworkState = DISCONNECTED;
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				mNetworkState = SERVER_FULL;
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

				// read stream into a dummy replay recorder
				boost::shared_ptr<GenericIn> reader = createGenericReader( &stream );
				ReplayRecorder dummyRec;
				dummyRec.receive( reader );
				// and save that
				saveReplay(dummyRec);

				// mWaitingForReplay will be set to false even if replay could not be saved because
				// the server won't send it again.
				mWaitingForReplay = false;

				break;
			}

			// we never do anything that should cause such a packet to be received!
			case ID_CONNECTION_REQUEST_ACCEPTED:
			case ID_CONNECTION_ATTEMPT_FAILED:
				assert( 0 );
				break;

			case ID_BLOBBY_SERVER_PRESENT:
			{
				// this should only be called if we use the stay on server option
				RakNet::BitStream stream( packet->getStream() );
				stream.IgnoreBytes(1);	//ID_BLOBBY_SERVER_PRESENT
				ServerInfo info(stream,	mClient->PlayerIDToDottedIP(packet->playerId), packet->playerId.port);

				if (packet->length == ServerInfo::BLOBBY_SERVER_PRESENT_PACKET_SIZE )
				{
					switchState(new LobbyState(info));
					return;
				}
				break;
			}
			default:
				printf("Received unknown Packet %d\n", packet->data[0]);
				std::cout<<packet->data<<"\n";
				break;
		}
	}

	// does this generate any problems if we pause at the exact moment an event is set ( i.e. the ball hit sound
	// could be played in a loop)?
	presentGame();
	presentGameUI();

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
			switchState(new MainMenuState);
			return;
		}
	}
	else if (InputManager::getSingleton()->exit() && mSaveReplay)
	{
		mSaveReplay = false;
		IMGUI::getSingleton().resetSelection();
	}
	else if (mErrorMessage != "")
	{
		displayErrorMessageBox();
	}
	else if (mSaveReplay)
	{
		if ( displaySaveReplayPrompt() )
		{

			// request replay from server
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_REPLAY);
			mClient->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0);

			mSaveReplay = false;
			mWaitingForReplay = true;
		}
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
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0), Vector2(700.0, 390.0));
			imgui.doText(GEN_ID, Vector2(140.0, 240.0),	TextManager::GAME_OPP_LEFT);

			if (imgui.doButton(GEN_ID, Vector2(230.0, 290.0), TextManager::LBL_OK))
			{
				switchState(new MainMenuState);
				return;
			}

			if (imgui.doButton(GEN_ID, Vector2(350.0, 290.0), TextManager::RP_SAVE))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}

			if (imgui.doButton(GEN_ID, Vector2(250.0, 340.0), TextManager::NET_STAY_ON_SERVER))
			{
				// Send a blobby server connection request
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_BLOBBY_SERVER_PRESENT);
				stream.Write(BLOBBY_VERSION_MAJOR);
				stream.Write(BLOBBY_VERSION_MINOR);
				mClient->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0);
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
				switchState(new MainMenuState);
				return;
			}
			if (imgui.doButton(GEN_ID, Vector2(350.0, 320.0), TextManager::RP_SAVE))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}
			break;
		}
		case SERVER_FULL:
		{
			imgui.doCursor();
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),Vector2(700.0, 370.0));
			imgui.doText(GEN_ID, Vector2(200.0, 250.0),	TextManager::NET_SERVER_FULL);
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0), TextManager::LBL_OK))
			{
				switchState(new MainMenuState);
				return;
			}
			break;
		}
		case PLAYING:
		{
			mMatch->step();

			mLocalInput->updateInput();
			PlayerInputAbs input = mLocalInput->getRealInput();

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
			input.writeTo(stream);
			mClient->Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
			break;
		}
		case PLAYER_WON:
		{
			displayWinningPlayerScreen(mWinningPlayer);
			if (imgui.doButton(GEN_ID, Vector2(290, 360), TextManager::LBL_OK))
			{
				switchState(new MainMenuState());
				return;
			}
			if (imgui.doButton(GEN_ID, Vector2(380, 360), TextManager::RP_SAVE))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}
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
				if ((InputManager::getSingleton()->getLastActionKey() == "Return") && (mChattext != ""))
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
				switchState(new MainMenuState);
				return;
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

// ---------------------------------------------------------------------------------------------------------------------------------
//	implementation of the local host state
// ----------------------------------------

NetworkHostState::NetworkHostState() : mServer(  ), mClient( new RakClient ), mGameState(nullptr)
{
	// read config
	/// \todo we need read-only access here!
	UserConfig config;
	config.loadFile("config.xml");
	PlayerSide localSide = (PlayerSide)config.getInteger("network_side");

	// load/init players
	if(localSide == LEFT_PLAYER)
	{
		mLocalPlayer = config.loadPlayerIdentity(LEFT_PLAYER, true);
	}
	 else
	{
		mLocalPlayer = config.loadPlayerIdentity(RIGHT_PLAYER, true);
	}

	ServerInfo info( mLocalPlayer.getName().c_str());
	std::string rulesfile = config.getString("rules");

	mServer.reset( new DedicatedServer(info, rulesfile, 4));

	// connect to server
	if (!mClient->Connect(info.hostname, info.port, 0, 0, RAKNET_THREAD_SLEEP_TIME))
		throw( std::runtime_error(std::string("Could not connect to server ") + info.hostname) );


}

NetworkHostState::~NetworkHostState()
{
	delete mGameState;
}

void NetworkHostState::step_impl()
{
	packet_ptr packet;
	if( mGameState == nullptr )
	{
		while (packet = mClient->Receive())
		{
			switch(packet->data[0])
			{
				// as soon as we are connected to the server
				case ID_CONNECTION_REQUEST_ACCEPTED:
				{
					// ----------------------------------------------------
					// Send ENTER SERVER packet
					RakNet::BitStream stream;
					stream.Write((unsigned char)ID_ENTER_SERVER);

					// Send preferred side
					stream.Write( mLocalPlayer.getPreferredSide() );

					// Send playername
					char myname[16];
					strncpy(myname, mLocalPlayer.getName().c_str(), sizeof(myname));
					stream.Write(myname, sizeof(myname));

					// send color settings
					stream.Write(mLocalPlayer.getStaticColor().toInt());

					mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);

					// Send ENTER GAME packet

					RakNet::BitStream stream2;
					stream2.Write((char)ID_CHALLENGE);
					auto writer = createGenericWriter(&stream2);
					writer->generic<PlayerID>( UNASSIGNED_PLAYER_ID );

					mClient->Send(&stream2, HIGH_PRIORITY, RELIABLE_ORDERED, 0);

					mGameState = new NetworkGameState(mClient);

					break;
				}
				case ID_SERVER_STATUS:
					{

					}
					break;
				default:
					std::cout << "Unknown packet " << int(packet->data[0]) << " received\n";
			}
		}
	}

	if(mServer->hasActiveGame())
	{
		mServer->allowNewPlayers(false);
	}

	mServer->processPackets();

	/// \todo make this gamespeed independent
	mLobbyCounter++;
	if(mLobbyCounter % (750 /*10s*/) == 0 )
	{
		mServer->updateLobby();
	}

	mServer->updateGames();

	if( mGameState )
		mGameState->step();
}

const char* NetworkHostState::getStateName() const
{
	return "NetworkHostState";
}

// definition of syslog for client hosted games
void syslog(int pri, const char* format, ...)
{
	// do nothing?
}

// debug counters
int SWLS_PacketCount;
int SWLS_Connections;
int SWLS_Games;
int SWLS_GameSteps;
int SWLS_ServerEntered;
