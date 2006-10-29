#include <sstream>

#include "NetworkState.h"
#include "NetworkMessage.h"
#include "NetworkGame.h"

#include "raknet/RakClient.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include "IMGUI.h"
#include "SoundManager.h"
#include "LocalInputSource.h"
#include "raknet/RakServer.h"

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
		config.setString("network_last_server",
				mScannedServers.at(mSelectedServer).hostname);
		config.saveFile("config.xml");
	}
	catch (std::exception)
	{
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
					printf("connection accepted\n");
					RakNet::BitStream stream;
					stream.Write(ID_BLOBBY_SERVER_PRESENT);
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
		imgui.doEditbox(GEN_ID, Vector2(130.0, 210.0), 20, mEnteredServer, mServerBoxPosition);
		if (imgui.doButton(GEN_ID, Vector2(270.0, 300.0), "ok"))
		{
			std::string server = mScannedServers[mSelectedServer].hostname;
			delete this;
			mCurrentState = new NetworkGameState(server.c_str(), BLOBBY_PORT);
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
		std::string description = mScannedServers[mSelectedServer].description;
		for (int i = 0; i < description.length(); i += 29)
		{
			imgui.doText(GEN_ID, Vector2(50, 190 + i / 29 * 30),
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

	mClient = new RakClient();
	if (mClient->Connect(servername.c_str(), port, 0, 0, 0))
		mNetworkState = CONNECTING;
	else
		mNetworkState = CONNECTION_FAILED;

	mPhysicWorld.resetPlayer();
	mFakeMatch = new DuelMatch(0, 0, false, true);
	RenderManager::getSingleton().setScore(0, 0, false, false);
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
				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
				mNetworkState = WAITING_FOR_OPPONENT;
				break;
			}
			case ID_PHYSIC_UPDATE:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				int ival;
				stream.Read(ival);
				stream.Read(ival);
				stream.Read(ival);
				RakNet::BitStream streamCopy = RakNet::BitStream(stream);
				//printf("Physic packet received. Time: %d\n", ival);
				mPhysicWorld.setState(&stream);
				mFakeMatch->setState(&streamCopy);
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
					mNetworkState = PLAYING;
				break;
			case ID_GAME_READY:
				mNetworkState = PLAYING;
				break;
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

	if (InputManager::getSingleton()->exit() && mNetworkState != PLAYING)
	{
		delete mCurrentState;
		mCurrentState = new MainMenuState;
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
					"opponent left he game");
			if (imgui.doButton(GEN_ID, Vector2(350.0, 320.0),
					"ok"))
			{
				delete mCurrentState;
				mCurrentState = new MainMenuState;
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
			if (imgui.doButton(GEN_ID, Vector2(350.0, 300.0),
					"ok"))
			{
				delete mCurrentState;
				mCurrentState = new MainMenuState;
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
			mPhysicWorld.step();
			mFakeMatch->step();
			if (InputManager::getSingleton()->exit())
			{
				RakNet::BitStream stream;
				stream.Write(ID_PAUSE);
				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
			}
			RakNet::BitStream stream;
			stream.Write(ID_INPUT_UPDATE);
			stream.Write(ID_TIMESTAMP);
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
			tmp << "player " << mWinningPlayer + 1;
			imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(700, 450));
			imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
			imgui.doText(GEN_ID, Vector2(274, 250), tmp.str());
			imgui.doText(GEN_ID, Vector2(274, 300), "has won the game!");
			if (imgui.doButton(GEN_ID, Vector2(290, 350), "OK"))
			{
				delete mCurrentState;
				mCurrentState = new MainMenuState();
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
			if (imgui.doButton(GEN_ID, Vector2(480, 330), "quit"))
			{
				delete this;
				mCurrentState = new MainMenuState;
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
				ServerInfo myinfo("somebody");
				myinfo.activegames = mNetworkGame ? 1 : 0;
				// TODO: Insert waiting players name here
				strncpy(myinfo.waitingplayer, "somebody",
					sizeof(myinfo.waitingplayer) - 1);
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
				PlayerSide newSide = (PlayerSide)ival;
				if (mLocalPlayerSide == NO_PLAYER)
				{ // First player ist probably the local one
					mLocalPlayerSide = newSide;
					mLocalPlayer = packet->playerId;
				}
				else
				{
					mRemotePlayer = packet->playerId;
					PlayerID leftPlayer = 
						LEFT_PLAYER == mLocalPlayerSide ?
						mLocalPlayer : packet->playerId;
					PlayerID rightPlayer =
						RIGHT_PLAYER == mLocalPlayerSide ?
						mLocalPlayer : packet->playerId;
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
						switchSide);
				}
			}
		}
	}
	mGameState->step();
	if (mNetworkGame)
		mNetworkGame->step();
}

