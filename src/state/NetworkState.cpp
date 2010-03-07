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
#include <sstream>

#include "NetworkState.h"
#include "NetworkMessage.h"
#include "NetworkGame.h"
#include "TextManager.h"

#include "ReplayRecorder.h"

#include "raknet/RakClient.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include "IMGUI.h"
#include "SoundManager.h"
#include "LocalInputSource.h"
#include "raknet/RakServer.h"
// We don't need the stringcompressor

NetworkSearchState::NetworkSearchState()
{
	IMGUI::getSingleton().resetSelection();
	mSelectedServer = 0;
	mServerBoxPosition = 0;
	mDisplayInfo = false;
	mEnteringServer = false;

	mPingClient = new RakClient;
	broadcast();
}

NetworkSearchState::~NetworkSearchState()
{
	
	try
	{
		UserConfig config;
		config.loadFile("config.xml");
		if (mScannedServers.size() > 0)
			config.setString("network_last_server",
					mScannedServers.at(mSelectedServer).hostname);
		config.saveFile("config.xml");
	}
	catch (std::exception& e)
	{
		std::cerr << "ERROR! in ~NetworkSearchState() " << e.what() << std::endl;
	}

	for (ClientList::iterator iter = mQueryClients.begin();
		iter != mQueryClients.end(); iter++)
	{
		if (*iter)
		{
			(*iter)->Disconnect(50);
			delete *iter;
		}
	}
	delete mPingClient;
}

void NetworkSearchState::step()
{
	Packet* packet;

	for (ClientList::iterator iter = mQueryClients.begin();
		iter != mQueryClients.end(); iter++)
	{
		bool skip = false;
		if (!skip)
		while ((packet = (*iter)->Receive()) && !skip)
		{
			switch(packet->data[0])
			{
				case ID_CONNECTION_REQUEST_ACCEPTED:
				{
					printf("connection accepted from %s\n",
						mPingClient->PlayerIDToDottedIP(
							packet->playerId));

					RakNet::BitStream stream;
					stream.Write((unsigned char)ID_BLOBBY_SERVER_PRESENT);
					stream.Write(BLOBBY_VERSION_MAJOR);
					stream.Write(BLOBBY_VERSION_MINOR);
					(*iter)->Send(&stream, HIGH_PRIORITY,
						RELIABLE_ORDERED, 0);
					break;
				}
				case ID_BLOBBY_SERVER_PRESENT:
				{
					RakNet::BitStream stream((char*)packet->data,
						packet->length, false);
					printf("server is a blobby server\n");
					stream.IgnoreBytes(1);	//ID_BLOBBY_SERVER_PRESENT
					ServerInfo info(stream,
						(*iter)->PlayerIDToDottedIP(
							packet->playerId));
					if (std::find(
							mScannedServers.begin(),
							mScannedServers.end(),
							info) == mScannedServers.end())
						mScannedServers.push_back(info);
					(*iter)->Disconnect(50);
					delete *iter;
					iter = mQueryClients.erase(iter);
					skip = true;
					break;
				}
				default:
					break;
			}
			if (skip)
				break;
		}
	}

	while (packet = mPingClient->Receive())
	{
		switch (packet->data[0])
		{
			case ID_PONG:
			{
				std::string hostname = mPingClient->PlayerIDToDottedIP(packet->playerId);
				printf("got ping response by \"%s\", trying to connect\n", hostname.c_str());
				RakClient* newClient = new RakClient;
				newClient->Connect(
					hostname.c_str(), BLOBBY_PORT, 0, 0, 0);
				mQueryClients.push_back(newClient);
			}
			default:
				break;
		}
	}

	IMGUI& imgui = IMGUI::getSingleton();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doInactiveMode(false);

	if (mDisplayInfo || mEnteringServer)
	{
		imgui.doInactiveMode(true);
	}

	if (imgui.doButton(GEN_ID, Vector2(10, 20), TextManager::getSingleton()->getString(TextManager::NET_SERVER_SCAN)))
		broadcast();

	if (imgui.doButton(GEN_ID, Vector2(420, 20), TextManager::getSingleton()->getString(TextManager::NET_DIRECT_CONNECT)) &&
			!mEnteringServer)
	{
		mEnteringServer = true;
		imgui.resetSelection();
		mEnteredServer = "";
		mServerBoxPosition = 0;
	}

	std::vector<std::string> servernames;
	for (int i = 0; i < mScannedServers.size(); i++)
	{
		servernames.push_back(mScannedServers[i].name);
	}

	imgui.doSelectbox(GEN_ID, Vector2(25.0, 60.0), Vector2(775.0, 470.0),
			servernames, mSelectedServer);

	if (imgui.doButton(GEN_ID, Vector2(50, 480), TextManager::getSingleton()->getString(TextManager::NET_SERVER_INFO)) &&
			!mDisplayInfo && !mScannedServers.empty())
	{
		mDisplayInfo = true;
		imgui.resetSelection();
	}

	if (mEnteringServer)
	{
		imgui.doInactiveMode(false);
		imgui.doOverlay(GEN_ID, Vector2(100.0, 200.0), Vector2(650.0, 400.0));
		// Game crashes if the mEnteredServer is not a possible input
		imgui.doEditbox(GEN_ID, Vector2(130.0, 210.0), 20, mEnteredServer, mServerBoxPosition);
		if (imgui.doButton(GEN_ID, Vector2(270.0, 300.0), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
		{
			//std::string server = mScannedServers[mSelectedServer].hostname;
			deleteCurrentState();
			setCurrentState(new NetworkGameState(mEnteredServer.c_str(), BLOBBY_PORT));
			return;
		}
		if (imgui.doButton(GEN_ID, Vector2(370.0, 300.0), TextManager::getSingleton()->getString(TextManager::LBL_CANCEL)))
		{
			mEnteringServer = false;
			imgui.resetSelection();
		}
		imgui.doInactiveMode(true);
	}

	if (mDisplayInfo)
	{
		imgui.doInactiveMode(false);
		imgui.doOverlay(GEN_ID, Vector2(40.0, 80.0), Vector2(760.0, 440.0));
		imgui.doText(GEN_ID, Vector2(50, 100), mScannedServers[mSelectedServer].name);
		imgui.doText(GEN_ID, Vector2(50, 130), mScannedServers[mSelectedServer].hostname);

		std::stringstream activegames;
		activegames << TextManager::getSingleton()->getString(TextManager::NET_ACTIVE_GAMES) 
					<< mScannedServers[mSelectedServer].activegames;
		imgui.doText(GEN_ID, Vector2(50, 160), activegames.str());
		std::stringstream waitingplayer;
		waitingplayer << TextManager::getSingleton()->getString(TextManager::NET_WAITING_PLAYER)
					  << mScannedServers[mSelectedServer].waitingplayer;
		imgui.doText(GEN_ID, Vector2(50, 190), waitingplayer.str());
		std::string description = mScannedServers[mSelectedServer].description;
		for (int i = 0; i < description.length(); i += 29)
		{
			imgui.doText(GEN_ID, Vector2(50, 220 + i / 29 * 30),
					description.substr(i, 29));
		}

		if (imgui.doButton(GEN_ID, Vector2(410, 405), TextManager::getSingleton()->getString(TextManager::LBL_OK)))
		{
			mDisplayInfo = false;
			imgui.resetSelection();
		}
		imgui.doInactiveMode(true);
	}

	if (imgui.doButton(GEN_ID, Vector2(450, 480), TextManager::getSingleton()->getString(TextManager::NET_HOST_GAME)) &&
			!mDisplayInfo)
	{
		deleteCurrentState();
		setCurrentState(new NetworkHostState());
		return;
	}

	if (imgui.doButton(GEN_ID, Vector2(230, 530), TextManager::getSingleton()->getString(TextManager::LBL_OK)) 
							&& !mScannedServers.empty())
	{
		std::string server = mScannedServers[mSelectedServer].hostname;
		deleteCurrentState();
		setCurrentState(new NetworkGameState(server.c_str(), BLOBBY_PORT));
	}
	if (imgui.doButton(GEN_ID, Vector2(480, 530), TextManager::getSingleton()->getString(TextManager::LBL_CANCEL)))
	{
		deleteCurrentState();
		setCurrentState(new MainMenuState);
	}
}

void NetworkSearchState::broadcast()
{
	mScannedServers.clear();
	mPingClient->PingServer("255.255.255.255", BLOBBY_PORT, 0, true);
	mPingClient->PingServer("blobby.openanno.org", BLOBBY_PORT, 0, true);
	UserConfig config;
	config.loadFile("config.xml");
	mPingClient->PingServer(
		config.getString("network_last_server").c_str(),
		BLOBBY_PORT, 0, true);
}

NetworkGameState::NetworkGameState(const std::string& servername, Uint16 port)
{
	IMGUI::getSingleton().resetSelection();
	mWinningPlayer = NO_PLAYER;
	UserConfig config;
	config.loadFile("config.xml");
	mOwnSide = (PlayerSide)config.getInteger("network_side");
	mLocalInput = new LocalInputSource(mOwnSide);
	mSaveReplay = false;
	std::stringstream temp;
	temp << time(0);
	mFilename = temp.str();

	// FIXME: We load the colors from config atm
	mLeftColor = Color(config.getInteger("left_blobby_color_r"),
		config.getInteger("left_blobby_color_g"),
		config.getInteger("left_blobby_color_b"));
	mRightColor = Color(config.getInteger("right_blobby_color_r"),
		config.getInteger("right_blobby_color_g"),
		config.getInteger("right_blobby_color_b"));
	mLeftOscillate = config.getBool("left_blobby_oscillate");
	mRightOscillate = config.getBool("right_blobby_oscillate");

	RenderManager::getSingleton().setBlobColor(0, mLeftColor);
	RenderManager::getSingleton().setBlobColor(1, mRightColor);
	RenderManager::getSingleton().redraw();

	mClient = new RakClient();
	if (mClient->Connect(servername.c_str(), port, 0, 0, 0))
		mNetworkState = CONNECTING;
	else
		mNetworkState = CONNECTION_FAILED;

	mPhysicWorld.resetPlayer();

	mReplayRecorder = new ReplayRecorder(MODE_RECORDING_DUEL);

	mFakeMatch = new DuelMatch(0, 0, false, true);
	mFakeMatch->setPlayerName(config.getString(mOwnSide ? "right_player_name" : "left_player_name"));
	RenderManager::getSingleton().setScore(0, 0, false, false);

	mSelectedChatmessage = 0;
	mChatCursorPosition = 0;
	mChattext = "";
}

NetworkGameState::~NetworkGameState()
{
	mClient->Disconnect(50);
	delete mLocalInput;
	delete mClient;
	delete mReplayRecorder;
	delete mFakeMatch;
}

void NetworkGameState::setLeftColor(Color ncol){
	mLeftColor = ncol;
	RenderManager::getSingleton().setBlobColor(0, mLeftColor);
	RenderManager::getSingleton().redraw();
}
void NetworkGameState::setRightColor(Color ncol){
	mRightColor = ncol;
	RenderManager::getSingleton().setBlobColor(1, mRightColor);
	RenderManager::getSingleton().redraw();
}
Color NetworkGameState::getLeftColor() const{
	return mLeftColor;
}
Color NetworkGameState::getRightColor() const{
	return mRightColor;
}

void NetworkGameState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	RenderManager* rmanager = &RenderManager::getSingleton();

	Packet* packet;
	while (packet = mClient->Receive())
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
				strncpy(myname, mFakeMatch->getPlayerName().c_str(), sizeof(myname));
				stream.Write(myname, sizeof(myname));

				// send color settings
				switch(mOwnSide){
					case LEFT_PLAYER:
						stream.Write(mLeftColor.toInt());
						break;
					case RIGHT_PLAYER:
						stream.Write(mRightColor.toInt());
						break;
				}

				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);

				RenderManager::getSingleton().setPlayernames(mOwnSide ?  "" : mFakeMatch->getPlayerName(), mOwnSide ? mFakeMatch->getPlayerName() : "");
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
				RakNet::BitStream streamCopy = RakNet::BitStream(stream);
				//printf("Physic packet received. Time: %d\n", ival);
				mPhysicWorld.setState(&stream);
				mFakeMatch->setState(&streamCopy);
				mReplayRecorder->record(mPhysicWorld.getPlayersInput());
				break;
			}
			case ID_WIN_NOTIFICATION:
			{
				if(mNetworkState != PLAYER_WON){
					RakNet::BitStream stream((char*)packet->data, packet->length, false);
					stream.IgnoreBytes(1);	//ID_WIN_NOTIFICATION
					stream.Read((int&)mWinningPlayer);
					
					// adds the last point, it must have been made by the winning player
					switch(mWinningPlayer){
						case LEFT_PLAYER:
							mLeftScore++;
							break;
						case RIGHT_PLAYER:
							mRightScore++;
							break;
						default:
							assert(0);
					}
					rmanager->setScore(mLeftScore, mRightScore,
						mServingPlayer == 0, mServingPlayer == 1);
						
					mNetworkState = PLAYER_WON;
				}
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
				stream.Read(mLeftScore);
				stream.Read(mRightScore);
				mPhysicWorld.reset(mServingPlayer);
				if (mLeftScore == 0 && mRightScore == 0)
					mReplayRecorder->setServingPlayer(mServingPlayer);
				rmanager->setScore(mLeftScore, mRightScore,
					mServingPlayer == 0, mServingPlayer == 1);
				break;
			}
			case ID_BALL_GROUND_COLLISION:
			{
				SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
				mPhysicWorld.setBallValidity(false);
				break;
			}
			case ID_BALL_PLAYER_COLLISION:
			{
				float intensity;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_PLAYER_BALL_COLLISION
				stream.Read(intensity);
				SoundManager::getSingleton().playSound("sounds/bums.wav",
						intensity + BALL_HIT_PLAYER_SOUND_VOLUME);
				break;
			}
			case ID_PAUSE:
				if (mNetworkState == PLAYING)
					mNetworkState = PAUSING;
				break;
			case ID_UNPAUSE:
				if (mNetworkState == PAUSING)
				{
					mNetworkState = PLAYING;
				}
				break;
			case ID_GAME_READY:
			{
				char charName[16];
				RakNet::BitStream stream((char*)packet->data, packet->length, false);

				stream.IgnoreBytes(1);	// ignore ID_GAME_READY

				// read playername
				stream.Read(charName, sizeof(charName));

				// ensures that charName is null terminated
				charName[sizeof(charName)-1] = '\0';

				// read colors
				int temp;
				stream.Read(temp);
				Color ncolor = temp;

				mFakeMatch->setOpponentName(std::string(charName));
				if (mOwnSide)
				{
					RenderManager::getSingleton().setPlayernames(mFakeMatch->getOpponentName(), mFakeMatch->getPlayerName());
					setLeftColor(ncolor);
					mReplayRecorder->setPlayerNames(mFakeMatch->getOpponentName(), mFakeMatch->getPlayerName());
				}
				else
				{
					RenderManager::getSingleton().setPlayernames(mFakeMatch->getPlayerName(), mFakeMatch->getOpponentName());
					mReplayRecorder->setPlayerNames(mFakeMatch->getPlayerName(), mFakeMatch->getOpponentName());
					setRightColor(ncolor);
				}
				// Workarround for SDL-Renderer
				// Hides the GUI when networkgame starts
				rmanager->redraw();	

				mNetworkState = PLAYING;
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
				mSelectedChatmessage = mChatlog.size() - 1;
				break;
			}
			default:
				printf("Received unknown Packet %d\n", packet->data[0]);
				break;
		}
		mClient->DeallocatePacket(packet);
	}

	PlayerInput input = mNetworkState == PLAYING ?
		mLocalInput->getInput() : PlayerInput();

	rmanager->setBlob(0, mPhysicWorld.getBlob(LEFT_PLAYER),
		mPhysicWorld.getBlobState(LEFT_PLAYER));
	rmanager->setBlob(1, mPhysicWorld.getBlob(RIGHT_PLAYER),
		mPhysicWorld.getBlobState(RIGHT_PLAYER));
	rmanager->setBall(mPhysicWorld.getBall(),
		mPhysicWorld.getBallRotation());

	float time = float(SDL_GetTicks()) / 1000.0;
	if (mLeftOscillate)
		rmanager->setBlobColor(0, Color(
			int((sin(time*2) + 1.0) * 128),
			int((sin(time*4) + 1.0) * 128),
			int((sin(time*3) + 1.0) * 128)));
	if (mRightOscillate)
		rmanager->setBlobColor(1, Color(
			int((cos(time*2) + 1.0) * 128),
			int((cos(time*4) + 1.0) * 128),
			int((cos(time*3) + 1.0) * 128)));

	if (InputManager::getSingleton()->exit() && mNetworkState != PLAYING)
	{
		deleteCurrentState();
		setCurrentState(new MainMenuState);
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
				mReplayRecorder->save(std::string("replays/") + mFilename + std::string(".bvr"));
			}
			mSaveReplay = false;
			imgui.resetSelection();
		}
		if (imgui.doButton(GEN_ID, Vector2(440, 330), TextManager::getSingleton()->getString(TextManager::LBL_CANCEL)))
		{
			mSaveReplay = false;
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
			mPhysicWorld.step(); 	 
			mFakeMatch->step();
			if (InputManager::getSingleton()->exit())
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_PAUSE);
				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
			}
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_INPUT_UPDATE);
			stream.Write((unsigned char)ID_TIMESTAMP);
			stream.Write(RakNet::GetTime());
			stream.Write(input.left);
			stream.Write(input.right);
			stream.Write(input.up);
			mClient->Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
			break;
		}
		case PLAYER_WON:
		{
			std::stringstream tmp;
			if(mWinningPlayer==LEFT_PLAYER)
				tmp << (mOwnSide ? mFakeMatch->getOpponentName() : mFakeMatch->getPlayerName());
			else
				tmp << (mOwnSide ? mFakeMatch->getPlayerName() : mFakeMatch->getOpponentName());
			imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(700, 450));
			imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
			imgui.doText(GEN_ID, Vector2(274, 240), tmp.str());
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
			imgui.doOverlay(GEN_ID, Vector2(200, 30), Vector2(650, 180));
			imgui.doText(GEN_ID, Vector2(300, 40), TextManager::getSingleton()->getString(TextManager::GAME_PAUSED));
			if (imgui.doButton(GEN_ID, Vector2(230, 100), TextManager::getSingleton()->getString(TextManager::LBL_CONTINUE)))
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_UNPAUSE);
				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
			}
			if (imgui.doButton(GEN_ID, Vector2(500, 100), TextManager::getSingleton()->getString(TextManager::GAME_QUIT)))
			{
				deleteCurrentState();
				setCurrentState(new MainMenuState);
			}
			if (imgui.doButton(GEN_ID, Vector2(310, 130), TextManager::getSingleton()->getString(TextManager::RP_SAVE)))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}
			// Chat
			imgui.doSelectbox(GEN_ID, Vector2(10, 190), Vector2(790, 450), mChatlog, mSelectedChatmessage);
			if (imgui.doEditbox(GEN_ID, Vector2(10, 460), 30, mChattext, mChatCursorPosition))
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
					mSelectedChatmessage = mChatlog.size() - 1;
					mChattext = "";
					mChatCursorPosition = 0;
				}
			}
			imgui.doCursor();
		}
	}
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
	Packet* packet;
	while (packet = mServer->Receive())
	{
		switch (packet->data[0])
		{
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_CONNECTION_LOST:
			case ID_INPUT_UPDATE:
			case ID_CHAT_MESSAGE:
			case ID_PAUSE:
			case ID_UNPAUSE:
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
							mGameState->setLeftColor(color);
							break;
						case RIGHT_PLAYER:
							mGameState->setRightColor(color);
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
						mGameState->setRightColor(color);
					}
					else
					{
						leftPlayer = packet->playerId;
						rightPlayer = mLocalPlayer;
						leftPlayerName = playerName;
						rightPlayerName = mLocalPlayerName;
						// set other color
						mGameState->setLeftColor(color);
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
						mGameState->getLeftColor(), mGameState->getRightColor(),
						switchSide);
				}
			}
		}
		mServer->DeallocatePacket(packet);
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

