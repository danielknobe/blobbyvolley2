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
	mLocalInput = new LocalInputSource(LEFT_PLAYER);
	mInputEnabled = false;
	mFailed = false;
	mWinningPlayer = NO_PLAYER;
	UserConfig config;
	config.loadFile("config.xml");
	mOwnSide = (PlayerSide)config.getInteger("network_side");

	mClient = new RakClient();
	mClient->Connect(servername.c_str(), port, 0, 0, 0);

	mPhysicWorld.resetPlayer();
}

NetworkGameState::~NetworkGameState()
{
	mClient->Disconnect(50);
	delete mLocalInput;
	delete mClient;
}

void NetworkGameState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	RenderManager* rmanager = &RenderManager::getSingleton();

	if (mWinningPlayer != NO_PLAYER)
	{
		std::stringstream tmp;
		tmp << "Spieler " << mWinningPlayer + 1;
		imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(650, 450));
		imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
		imgui.doText(GEN_ID, Vector2(274, 250), tmp.str());
		imgui.doText(GEN_ID, Vector2(274, 300), "hat gewonnen!");
		if (imgui.doButton(GEN_ID, Vector2(290, 350), "OK"))
		{
			delete mCurrentState;
			mCurrentState = new MainMenuState();
		}
		imgui.doCursor();
	}

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
				printf("Connection accepted.\n");
				break;
			}
			case ID_PHYSIC_UPDATE:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				int ival;
				stream.Read(ival);
				stream.Read(ival);
				stream.Read(ival);
				printf("Physic packet received. Time: %d\n", ival);
				mPhysicWorld.setState(&stream);
				break;
			}
			case ID_WIN_NOTIFICATION:
			{
				int ival;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				stream.Read(ival);
				stream.Read((int&)mWinningPlayer);
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
				break;
			}
			case ID_BALL_GROUND_COLLISION:
			{
				SoundManager::getSingleton().playSound("pfiff.wav", 0.2);
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
				SoundManager::getSingleton().playSound("bums.wav", 
						intensity + 0.4);
				mPhysicWorld.setBallValidity(false);
				break;
			}
			case ID_GAME_READY:
				mInputEnabled = true;
				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				mFailed = true;
				break;
			case ID_RECEIVED_STATIC_DATA:
			{
				break;
			}
			default:
				printf("Received unknown Packet %d\n", packet->data[0]);
				break;
		}
		mClient->DeallocatePacket(packet);
	}
	PlayerInput input = PlayerInput();
	
	if (mInputEnabled)
	{
		input = mLocalInput->getInput();
		RakNet::BitStream stream;
		stream.Write(ID_INPUT_UPDATE);
		stream.Write(ID_TIMESTAMP);
		stream.Write(RakNet::GetTime());
		stream.Write(input.left);
		stream.Write(input.right);
		stream.Write(input.up);
		mClient->Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
	}
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

	mPhysicWorld.step();
	if (InputManager::getSingleton()->exit())
	{
		delete mCurrentState;
		mCurrentState = new MainMenuState;
	}
	else if (mFailed)
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
	}
	else if (!mClient->IsConnected())
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 210.0),
				Vector2(700.0, 310.0));
		imgui.doText(GEN_ID, Vector2(150.0, 250.0),
				"connecting to server...");
	}

}

