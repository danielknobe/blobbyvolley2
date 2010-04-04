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
#include "Blood.h"

#include "ReplayRecorder.h"

#include "raknet/RakClient.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include "IMGUI.h"
#include "SoundManager.h"
#include "LocalInputSource.h"
#include "raknet/RakServer.h"
#include "RakNetPacket.h"
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
	packet_ptr packet;
	
	for (ClientList::iterator iter = mQueryClients.begin();
		iter != mQueryClients.end(); iter++)
	{
		bool skip = false;
		if (!skip)
		while ((packet = receivePacket(*iter)) && !skip)
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
					(*iter)->Send(&stream, LOW_PRIORITY,
						RELIABLE_ORDERED, 0);
					break;
				}
				case ID_BLOBBY_SERVER_PRESENT:
				{
					//FIXME: We must copy the needed informations, so that we can call DeallocatePacket(packet)
					//FIXME: The client finds a server at this point, which is not valid
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
							info) == mScannedServers.end() 
							// check whether the packet sizes match
							&& packet->length == ServerInfo::BLOBBY_SERVER_PRESENT_PACKET_SIZE ){
						mScannedServers.push_back(info);
					}
					// the RakClient will be deleted, so
					// we must free the packet here
					packet.reset();
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
	while (packet = receivePacket(mPingClient))
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
		std::stringstream gamespeed;
		gamespeed << TextManager::getSingleton()->getString(TextManager::OP_SPEED)<<" "
					  << int(100.0 / 75.0 * mScannedServers[mSelectedServer].gamespeed)<<"%";
		imgui.doText(GEN_ID, Vector2(50, 220), gamespeed.str());
		std::string description = mScannedServers[mSelectedServer].description;
		for (int i = 0; i < description.length(); i += 29)
		{
			imgui.doText(GEN_ID, Vector2(50, 250 + i / 29 * 30),
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

NetworkGameState::NetworkGameState(const std::string& servername, Uint16 port):
	mLeftPlayer(LEFT_PLAYER),
	mRightPlayer(RIGHT_PLAYER)
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

	RenderManager::getSingleton().redraw();

	mClient = new RakClient();
	if (mClient->Connect(servername.c_str(), port, 0, 0, 0))
		mNetworkState = CONNECTING;
	else
		mNetworkState = CONNECTION_FAILED;

	mReplayRecorder = new ReplayRecorder(MODE_RECORDING_DUEL);

	mFakeMatch = new DuelMatch(0, 0, true);
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
	delete mReplayRecorder;
	delete mFakeMatch;
}

void NetworkGameState::setLeftColor(Color ncol){
	mLeftPlayer.setColor(ncol);
	RenderManager::getSingleton().setBlobColor(0, mLeftPlayer.getColor());
	RenderManager::getSingleton().redraw();
}
void NetworkGameState::setRightColor(Color ncol){
	mRightPlayer.setColor(ncol);
	RenderManager::getSingleton().setBlobColor(1, mRightPlayer.getColor());
	RenderManager::getSingleton().redraw();
}
Color NetworkGameState::getLeftColor() const{
	return mLeftPlayer.getColor();
}
Color NetworkGameState::getRightColor() const{
	return mRightPlayer.getColor();
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
				mReplayRecorder->record(mFakeMatch->getPlayersInput());
				break;
			}
			case ID_WIN_NOTIFICATION:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.IgnoreBytes(1);	//ID_WIN_NOTIFICATION
				stream.Read((int&)mWinningPlayer);
				
				// adds the last point, it must have been made by the winning player
				switch(mWinningPlayer){
					case LEFT_PLAYER:
						mFakeMatch->setScore(mFakeMatch->getScore(LEFT_PLAYER)+1, mFakeMatch->getScore(RIGHT_PLAYER));
						break;
					case RIGHT_PLAYER:
						mFakeMatch->setScore(mFakeMatch->getScore(LEFT_PLAYER), mFakeMatch->getScore(RIGHT_PLAYER)+1);
						break;
					default:
						assert(0);
				}
					
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
				stream.Read(nLeftScore);
				stream.Read(nRightScore);
				mFakeMatch->setScore(nLeftScore, nRightScore);
				mFakeMatch->setServingPlayer(mServingPlayer);
				
				if (nLeftScore == 0 && nRightScore == 0)
					mReplayRecorder->setServingPlayer(mServingPlayer);
				break;
			}
			case ID_BALL_GROUND_COLLISION:
			{
				 SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
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
				mReplayRecorder->setPlayerNames(mLeftPlayer.getName(), mRightPlayer.getName());
				mRemotePlayer->setColor(ncolor);
				
				// Workarround for SDL-Renderer
				// Hides the GUI when networkgame starts
				rmanager->redraw();	

				mNetworkState = PLAYING;
				// start game
				mFakeMatch->unpause();
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
				std::cout<<packet->data<<"\n";
				break;
		}
	}

	PlayerInput input = mNetworkState == PLAYING ?
		mLocalInput->getInput() : PlayerInput();

	presentGame(*mFakeMatch, false);
	rmanager->setBlobColor(LEFT_PLAYER, mLeftPlayer.getColor());
	rmanager->setBlobColor(RIGHT_PLAYER, mRightPlayer.getColor());

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
				tmp << mLeftPlayer.getName();
			else
				tmp << mRightPlayer.getName();
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

