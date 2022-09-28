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

#include <utility>

#include "raknet/RakClient.h"
#include "raknet/PacketEnumerations.h"

#include "NetworkState.h"
#include "replays/ReplayRecorder.h"
#include "match/DuelMatch.h"
#include "IMGUI.h"
#include "SoundManager.h"
#include "input/LocalInputSource.h"
#include "UserConfig.h"
#include "io/FileExceptions.h"
#include "io/GenericIO.h"
#include "io/FileRead.h"
#include "io/FileWrite.h"
#include "SpeedController.h"
#include "LobbyStates.h"

// global variable to save the lag
int CURRENT_NETWORK_LAG = -1;


/* implementation */
NetworkGameState::NetworkGameState( std::shared_ptr<RakClient> client, int rule_checksum, int score_to_win)
	: GameState(new DuelMatch(true, DEFAULT_RULES_FILE, score_to_win))
	, mNetworkState(WAITING_FOR_OPPONENT)
	, mWaitingForReplay(false)
	, mClient(std::move(client))
	, mWinningPlayer(NO_PLAYER)
	, mSelectedChatmessage(0)
	, mChatCursorPosition(0)
{
	std::shared_ptr<IUserConfigReader> config = IUserConfigReader::createUserConfigReader("config.xml");
	mOwnSide = (PlayerSide)config->getInteger("network_side");
	mUseRemoteColor = config->getBool("use_remote_color");
	mLocalInput.reset(new LocalInputSource(mOwnSide));
	mLocalInput->setMatch(mMatch.get());

	// game is not started until two players are connected
	mMatch->pause();

	// load/init players

	if(mOwnSide == LEFT_PLAYER)
	{
		PlayerIdentity localplayer = config->loadPlayerIdentity(LEFT_PLAYER, true);
		PlayerIdentity remoteplayer = config->loadPlayerIdentity(RIGHT_PLAYER, true);
		mLocalPlayer = &mMatch->getPlayer( LEFT_PLAYER );
		mRemotePlayer = &mMatch->getPlayer( RIGHT_PLAYER );
		mMatch->setPlayers( localplayer, remoteplayer );
	}
	else
	{
		PlayerIdentity localplayer = config->loadPlayerIdentity(RIGHT_PLAYER, true);
		PlayerIdentity remoteplayer = config->loadPlayerIdentity(LEFT_PLAYER, true);
		mLocalPlayer = &mMatch->getPlayer( RIGHT_PLAYER );
		mRemotePlayer = &mMatch->getPlayer( LEFT_PLAYER );
		mMatch->setPlayers( remoteplayer, localplayer );
	}

	mRemotePlayer->setName("");

	// check the rules
	int ourChecksum = 0;
	if (rule_checksum != 0)
	{
		try
		{
			FileRead rulesFile(TEMP_RULES_NAME);
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
	stream2.Write(bool(rule_checksum != 0 && rule_checksum != ourChecksum));
	mClient->Send(&stream2, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
}

NetworkGameState::~NetworkGameState()
{
	CURRENT_NETWORK_LAG = -1;
	mClient->Disconnect(50);
}

void NetworkGameState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();

	packet_ptr packet;
	while (nullptr != (packet = mClient->Receive()))
	{
		switch(packet->data[0])
		{
			case ID_GAME_UPDATE:
			{
				RakNet::BitStream stream(packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_GAME_UPDATE
				unsigned timeBack;
				stream.Read(timeBack);
				CURRENT_NETWORK_LAG = SDL_GetTicks() - timeBack;
				DuelMatchState ms;
				/// \todo this is a performance nightmare: we create a new reader for every packet!
				///			there should be a better way to do that
				std::shared_ptr<GenericIn> in = createGenericReader(&stream);
				in->generic<DuelMatchState> (ms);
				// inject network data into game
				mMatch->setState( ms );
				break;
			}

			case ID_GAME_EVENTS:
			{
				RakNet::BitStream stream(packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_GAME_EVENTS
				//printf("Physic packet received. Time: %d\n", ival);
				// read events
				char event = 0;
				for(stream.Read(event); event != 0; stream.Read(event))
				{
					char side;
					float intensity = -1;
					stream.Read(side);
					if( event == MatchEvent::BALL_HIT_BLOB )
						stream.Read(intensity);
					MatchEvent me{ MatchEvent::EventType(event), (PlayerSide)side, intensity };
					mMatch->trigger( me );
				}
				break;
			}
			case ID_WIN_NOTIFICATION:
			{
				RakNet::BitStream stream(packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_WIN_NOTIFICATION
				stream.Read((int&)mWinningPlayer);

				// last point must not be added anymore, because
				// the score is also simulated local, so it is already
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
					SDL_StopTextInput();
					mNetworkState = PLAYING;
					mMatch->unpause();
				}
				break;
			case ID_GAME_READY:
			{
				char charName[16];
				RakNet::BitStream stream(packet->data, packet->length, false);

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
				auto ncolor = Color(temp);

				mRemotePlayer->setName(charName);

				setDefaultReplayName(mLocalPlayer->getName(), mRemotePlayer->getName());

				// check whether to use remote player color
				if(mUseRemoteColor)
				{
					mRemotePlayer->setStaticColor(ncolor);
				}

				mNetworkState = PLAYING;
				// start game
				mMatch->unpause();

				// game ready whistle
				SoundManager::getSingleton().playSound(SoundManager::WHISTLE, ROUND_START_SOUND_VOLUME);
				break;
			}
			case ID_RULES_CHECKSUM:
			{
				assert(0);
				break;
			}
			case ID_RULES:
			{
				RakNet::BitStream stream(packet->data, packet->length, false);

				stream.IgnoreBytes(1);	// ignore ID_RULES

				int rulesLength;
				stream.Read(rulesLength);
				if (rulesLength)
				{
                    std::vector<char> rulesString(rulesLength + 1, '\0');
					stream.Read(rulesString.data(), rulesLength);
					FileWrite rulesFile("rules/"+TEMP_RULES_NAME);
					rulesFile.write(rulesString.data(), rulesLength);
					rulesFile.close();
					mMatch->setRules(TEMP_RULES_NAME);
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
				RakNet::BitStream stream(packet->data, packet->length, false);
				stream.IgnoreBytes(1);	// ID_CHAT_MESSAGE
				// Insert Message in the log and focus the last element
				char message[31];
				stream.Read(message, sizeof(message));
				message[30] = '\0';

				// Insert Message in the log and focus the last element
				mChatlog.push_back((std::string) message);
				mChatOrigin.push_back(false);
				mSelectedChatmessage = mChatlog.size() - 1;
				SoundManager::getSingleton().playSound(SoundManager::CHAT, ROUND_START_SOUND_VOLUME);
				break;
			}
			case ID_REPLAY:
			{
				/// \todo we should take more action if server sends replay
				///		even if not requested!
				if(!mWaitingForReplay)
					break;

				RakNet::BitStream stream(packet->data, packet->length, false);
				stream.IgnoreBytes(1);	// ID_REPLAY

				// read stream into a dummy replay recorder
				std::shared_ptr<GenericIn> reader = createGenericReader( &stream );
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
				RakNet::BitStream stream(packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_BLOBBY_SERVER_PRESENT
				ServerInfo info(stream,	mClient->PlayerIDToDottedIP(packet->playerId), packet->playerId.port);

				if (packet->length == ServerInfo::BLOBBY_SERVER_PRESENT_PACKET_SIZE )
				{
					switchState(new LobbyState(info, PreviousState::MAIN));
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
		}
	}
	else if (InputManager::getSingleton()->exit() && mSaveReplay)
	{
		mSaveReplay = false;
		IMGUI::getSingleton().resetSelection();
	}
	else if (!mErrorMessage.empty())
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
			stream.Write( SDL_GetTicks() );
			input.writeTo(stream);
			mClient->Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
			break;
		}
		case PLAYER_WON:
		{
			mMatch->updateEvents(); // so the last whistle will be sounded
			displayWinningPlayerScreen(mWinningPlayer);
			if (imgui.doButton(GEN_ID, Vector2(290, 360), TextManager::LBL_OK))
			{
				switchState(new MainMenuState());
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
			// Query
			displayQueryPrompt(20,
				TextManager::GAME_PAUSED,
				std::make_tuple(TextManager::LBL_CONTINUE, [&](){
					RakNet::BitStream stream;
					stream.Write((unsigned char)ID_UNPAUSE);
					mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
				}),
				std::make_tuple(TextManager::GAME_QUIT,    [&](){ switchState(new MainMenuState); }),
				std::make_tuple(TextManager::RP_SAVE, [&](){ mSaveReplay = true; imgui.resetSelection(); }));

			// Chat
			imgui.doChatbox(GEN_ID, Vector2(10, 240), Vector2(790, 500), mChatlog, mSelectedChatmessage, mChatOrigin);
			if (imgui.doEditbox(GEN_ID, Vector2(30, 510), 30, mChattext, mChatCursorPosition, 0, true))
			{

				// GUI-Hack, so that we can send messages
				if ((InputManager::getSingleton()->getLastActionKey() == "Return") && (!mChattext.empty()))
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
					SoundManager::getSingleton().playSound(SoundManager::CHAT, ROUND_START_SOUND_VOLUME);
				}
			}
			imgui.doCursor();
		}
	}
}

const char* NetworkGameState::getStateName() const
{
	return "NetworkGameState";
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
