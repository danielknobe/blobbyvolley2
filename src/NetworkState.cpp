#include <sstream>

#include "NetworkState.h"
#include "NetworkMessage.h"

#include "raknet/RakClient.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include "IMGUI.h"
#include "SoundManager.h"
#include "LocalInputSource.h"


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
				SoundManager::getSingleton().playSound("sounds/pfiff.wav", 0.2);
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
						intensity + 0.4);
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
				mNetworkState = OPPONENT_DISCONNECTED;
				break;
				
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_CONNECTION_LOST:
				mNetworkState = DISCONNECTED;
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
	
	if (mOwnSide == LEFT_PLAYER)
		mPhysicWorld.setLeftInput(input);
	else
		mPhysicWorld.setRightInput(input);

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
					"opponent disconnected");
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
			tmp << "Spieler " << mWinningPlayer + 1;
			imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(650, 450));
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

