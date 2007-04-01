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

#include <sstream>

#include "NetworkState.h"
#include "NetworkMessage.h"
#include "NetworkGame.h"

#include "ReplayRecorder.h"

#include "raknet/RakClient.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include "IMGUI.h"
#include "SoundManager.h"
#include "LocalInputSource.h"
#include "raknet/RakServer.h"
#include "raknet/StringCompressor.h"


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
	UserConfig config;
	try
	{
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
					stream.Write(ID_BLOBBY_SERVER_PRESENT);
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
					int ival;
					stream.Read(ival);
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

	if (imgui.doButton(GEN_ID, Vector2(10, 20), "scan for servers"))
		broadcast();

	if (imgui.doButton(GEN_ID, Vector2(420, 20), "direct connect") &&
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

	if (imgui.doButton(GEN_ID, Vector2(50, 480), "server info") &&
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
		if (imgui.doButton(GEN_ID, Vector2(270.0, 300.0), "ok"))
		{
			//std::string server = mScannedServers[mSelectedServer].hostname;
			delete this;
			mCurrentState = new NetworkGameState(mEnteredServer.c_str(), BLOBBY_PORT);
			return;
		}
		if (imgui.doButton(GEN_ID, Vector2(370.0, 300.0), "cancel"))
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
		activegames << "active games: " << mScannedServers[mSelectedServer].activegames;
		imgui.doText(GEN_ID, Vector2(50, 160), activegames.str());
		std::stringstream waitingplayer;
		waitingplayer << "waiting player: " << mScannedServers[mSelectedServer].waitingplayer;
		imgui.doText(GEN_ID, Vector2(50, 190), waitingplayer.str());
		std::string description = mScannedServers[mSelectedServer].description;
		for (int i = 0; i < description.length(); i += 29)
		{
			imgui.doText(GEN_ID, Vector2(50, 220 + i / 29 * 30),
					description.substr(i, 29));
		}

		if (imgui.doButton(GEN_ID, Vector2(410, 405), "ok"))
		{
			mDisplayInfo = false;
			imgui.resetSelection();
		}
		imgui.doInactiveMode(true);
	}

	if (imgui.doButton(GEN_ID, Vector2(450, 480), "host game") &&
			!mDisplayInfo)
	{
		delete this;
		mCurrentState = new NetworkHostState();
		return;
	}

	if (imgui.doButton(GEN_ID, Vector2(230, 530), "ok") && !mScannedServers.empty())
	{
		std::string server = mScannedServers[mSelectedServer].hostname;
		delete this;
		mCurrentState = new NetworkGameState(server.c_str(), BLOBBY_PORT);
	}
	if (imgui.doButton(GEN_ID, Vector2(480, 530), "cancel"))
	{
		delete this;
		mCurrentState = new MainMenuState;
	}
}

void NetworkSearchState::broadcast()
{
	mScannedServers.clear();
	mPingClient->PingServer("255.255.255.255", BLOBBY_PORT, 0, true);
	mPingClient->PingServer("88.198.43.14", BLOBBY_PORT, 0, true);
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

	mLeftColor = Color(config.getInteger("r1"),
		config.getInteger("g1"),
		config.getInteger("b1"));
	mRightColor = Color(config.getInteger("r2"),
		config.getInteger("g2"),
		config.getInteger("b2"));
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

	mGameSpeed = config.getFloat("gamespeed");	//loading game speed from local config
	if (mGameSpeed < 0.1)						//before receiving it from server
		mGameSpeed = 1.0;

	float gameFPS = config.getFloat("gamefps");
	SpeedController::setGameFPS(gameFPS);
	mGameFPSController = new SpeedController(gameFPS <= 0 ? 60 : gameFPS);
	SpeedController::setCurrentGameFPSInstance(mGameFPSController);

	mPhysicWorld.resetPlayer();

	mReplayRecorder = new ReplayRecorder(MODE_RECORDING_DUEL);
	mReplayRecorder->setGameFPS(gameFPS);

	mFakeMatch = new DuelMatch(0, 0, false, true);
	mFakeMatch->setPlayerName(config.getString(mOwnSide ? "right_player_name" : "left_player_name"));
	RenderManager::getSingleton().setScore(0, 0, false, false);
}

NetworkGameState::~NetworkGameState()
{
	mClient->Disconnect(50);
	delete mLocalInput;
	delete mClient;
	delete mReplayRecorder;
	delete mFakeMatch;
}

void NetworkGameState::step()
{
	SpeedController::setCurrentGameFPSInstance(mGameFPSController);
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
				stream.Write(ID_ENTER_GAME);
				stream.Write(mOwnSide);
				StringCompressor::Instance()->EncodeString((char*)mFakeMatch->getPlayerName().c_str(), 16, &stream);
				stream.Write(mGameSpeed);
				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
				RenderManager::getSingleton().setPlayernames(mOwnSide ?  "" : mFakeMatch->getPlayerName(), mOwnSide ? mFakeMatch->getPlayerName() : "");
				mNetworkState = WAITING_FOR_OPPONENT;
				break;
			}
			case ID_PHYSIC_UPDATE:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				int ival;
				stream.Read(ival);
				stream.Read(ival);	//ID_TIMESTAMP
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
				int ival;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.Read(ival);
				stream.Read((int&)mWinningPlayer);
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
				int ival;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.Read(ival);
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
				int ival;
				float intensity;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.Read(ival);
				stream.Read(intensity);
				SoundManager::getSingleton().playSound("sounds/bums.wav",
						intensity + BALL_HIT_PLAYER_SOUND_VOLUME);
				mPhysicWorld.setBallValidity(false);
				break;
			}
			case ID_PAUSE:
				if (mNetworkState = PLAYING)
					mNetworkState = PAUSING;
				break;
			case ID_UNPAUSE:
				if (mNetworkState == PAUSING)
				{
					mNetworkState = PLAYING;
					mGameFPSController->endPause();
				}
				break;
			case ID_GAME_READY:
			{
				int ival;
				char* charName = new char[16];
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.Read(ival);
				StringCompressor::Instance()->DecodeString(charName, 16, &stream);
				mFakeMatch->setOpponentName(std::string(charName));
				if (mOwnSide)
				{
					RenderManager::getSingleton().setPlayernames(mFakeMatch->getOpponentName(), mFakeMatch->getPlayerName());
					mReplayRecorder->setPlayerNames(mFakeMatch->getOpponentName(), mFakeMatch->getPlayerName());
				}
				else
				{
					RenderManager::getSingleton().setPlayernames(mFakeMatch->getPlayerName(), mFakeMatch->getOpponentName());
					mReplayRecorder->setPlayerNames(mFakeMatch->getPlayerName(), mFakeMatch->getOpponentName());
				}
				stream.Read(mGameSpeed);
				mReplayRecorder->setGameSpeed(mGameSpeed);
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
		delete mCurrentState;
		mCurrentState = new MainMenuState;
	}
	else if (InputManager::getSingleton()->exit() && mSaveReplay)
	{
		mSaveReplay = false;
		IMGUI::getSingleton().resetSelection();
	}
	else if (mSaveReplay)
	{
		imgui.doOverlay(GEN_ID, Vector2(150, 200), Vector2(650, 400));
		imgui.doText(GEN_ID, Vector2(190, 220), "Name of the Replay:");
		static unsigned cpos;
		imgui.doEditbox(GEN_ID, Vector2(180, 270), 18, mFilename, cpos);
		if (imgui.doButton(GEN_ID, Vector2(220, 330), "OK"))
		{
			if (mFilename != "")
			{
				mReplayRecorder->save(std::string("replays/") + mFilename + std::string(".bvr"));
			}
			mSaveReplay = false;
			imgui.resetSelection();
		}
		if (imgui.doButton(GEN_ID, Vector2(440, 330), "Cancel"))
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
					"connecting to server...");
			break;
		}
		case WAITING_FOR_OPPONENT:
		{
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 310.0));
			imgui.doText(GEN_ID, Vector2(150.0, 250.0),
					"waiting for opponent...");
			break;
		}
		case OPPONENT_DISCONNECTED:
		{
			imgui.doCursor();
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 370.0));
			imgui.doText(GEN_ID, Vector2(140.0, 250.0),
					"opponent left the game");
			if (imgui.doButton(GEN_ID, Vector2(230.0, 300.0),
					"ok"))
			{
				delete mCurrentState;
				mCurrentState = new MainMenuState;
			}
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0), "Save Replay"))
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
					"disconnected from server");
			if (imgui.doButton(GEN_ID, Vector2(230.0, 320.0),
					"ok"))
			{
				delete mCurrentState;
				mCurrentState = new MainMenuState;
			}
			if (imgui.doButton(GEN_ID, Vector2(350.0, 320.0), "Save Replay"))
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
					"connection failed");
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0),
					"ok"))
			{
				delete mCurrentState;
				mCurrentState = new MainMenuState;
			}
			break;
		}
		case SERVER_FULL:
		{
			imgui.doCursor();
			imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
					Vector2(700.0, 370.0));
			imgui.doText(GEN_ID, Vector2(200.0, 250.0),
					"server is full");
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0),
					"ok"))
			{
				delete mCurrentState;
				mCurrentState = new MainMenuState;
			}
			break;
		}
		case PLAYING:
		{
			if (InputManager::getSingleton()->exit())
			{
				RakNet::BitStream stream;
				stream.Write(ID_PAUSE);
				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
			}
			if (SpeedController::getCurrentGameFPSInstance()->beginFrame())
			{
				float timeDelta = SpeedController::getCurrentGameFPSInstance()->getTimeDelta();
				mPhysicWorld.step(timeDelta, mGameSpeed);
				mFakeMatch->step(timeDelta, mGameSpeed);
				RakNet::BitStream stream;
				stream.Write(ID_INPUT_UPDATE);
				stream.Write(ID_TIMESTAMP);
				stream.Write(RakNet::GetTime());
				stream.Write(input.left);
				stream.Write(input.right);
				stream.Write(input.up);
				mClient->Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
				SpeedController::getCurrentGameFPSInstance()->endFrame();
			}
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
			imgui.doText(GEN_ID, Vector2(274, 300), "has won the game!");
			if (imgui.doButton(GEN_ID, Vector2(290, 360), "ok"))
			{
				delete mCurrentState;
				mCurrentState = new MainMenuState();
			}
			if (imgui.doButton(GEN_ID, Vector2(380, 360), "Save Replay"))
			{
				mSaveReplay = true;
				imgui.resetSelection();
			}
			imgui.doCursor();
			break;
		}
		case PAUSING:
		{
			imgui.doOverlay(GEN_ID, Vector2(200, 200), Vector2(650, 400));
			imgui.doText(GEN_ID, Vector2(300, 260), "game paused");
			if (imgui.doButton(GEN_ID, Vector2(230, 330), "continue"))
			{
				RakNet::BitStream stream;
				stream.Write(ID_UNPAUSE);
				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
			}
			if (imgui.doButton(GEN_ID, Vector2(500, 330), "quit"))
			{
				delete this;
				mCurrentState = new MainMenuState;
			}
			if (imgui.doButton(GEN_ID, Vector2(310, 370), "Save Replay"))
			{
				mSaveReplay = true;
				imgui.resetSelection();
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
	if (mNetworkGame)
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
				stream.Write(ID_BLOBBY_SERVER_PRESENT);
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
				stream.Read(ival);
				stream.Read(ival);
				char* charName = new char[16];
				StringCompressor::Instance()->DecodeString(charName, 16, &stream);
				std::string playerName(charName);
				PlayerSide newSide = (PlayerSide)ival;

				if (mLocalPlayerSide == NO_PLAYER)
				{ // First player is probably the local one
					mLocalPlayerSide = newSide;
					mLocalPlayer = packet->playerId;
					mLocalPlayerName = playerName;

					stream.Read(mGameSpeed);
					if (mGameSpeed < 0.1)
						mGameSpeed = 1.0;
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
					}
					else
					{
						leftPlayer = packet->playerId;
						rightPlayer = mLocalPlayer;
						leftPlayerName = playerName;
						rightPlayerName = mLocalPlayerName;
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
						mGameSpeed, switchSide);
				}
			}
		}
	}
	mGameState->step();
	if (dynamic_cast<NetworkHostState*>(mCurrentState) != 0)
	{
		if (mNetworkGame)
		{
				mNetworkGame->step();
		}
	}
}

