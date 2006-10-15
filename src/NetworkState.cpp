#include "NetworkState.h"

#include "raknet/RakClient.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include "IMGUI.h"
#include "DuelMatch.h"
#include "LocalInputSource.h"


NetworkGameState::NetworkGameState(const std::string& servername, Uint16 port)
{
	IMGUI::getSingleton().resetSelection();
	mLocalInput = new LocalInputSource(LEFT_PLAYER);
	mRemoteInput = new DummyInputSource();
	mMatch = new DuelMatch(mLocalInput, mRemoteInput, true, false);

	mClient = new RakClient();
	mClient->Connect(servername.c_str(), port, 0, 0, 0);

}

NetworkGameState::~NetworkGameState()
{
	mClient->Disconnect(50);
	delete mMatch;
	delete mLocalInput;
	delete mRemoteInput;
	delete mClient;
}

void NetworkGameState::step()
{
	static int initTimeDiff = -1;
	Packet* packet;
	while (packet = mClient->Receive())
	{
		switch(packet->data[0])
		{
			case ID_CONNECTION_REQUEST_ACCEPTED:
				printf("Connection accepted.\n");
				break;
			case ID_TIMESTAMP:
			{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				int cval;
				int ival;
				stream.Read(cval);
				stream.Read(ival);
				if (initTimeDiff == -1)
					initTimeDiff = ival;
				printf("Physic packet received. Time: %d\n", RakNet::GetTime() - ival + initTimeDiff);
				mMatch->setState(&stream);
				break;
			}
			default:
				break;
		}
		mClient->DeallocatePacket(packet);
	}
	PlayerInput input = mLocalInput->getInput();

	RakNet::BitStream stream;
	stream.Write(ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());
	stream.Write(input.left);
	stream.Write(input.right);
	stream.Write(input.up);
	mClient->Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);

	mMatch->step();
	if (InputManager::getSingleton()->exit())
	{
		delete mCurrentState;
		mCurrentState = new MainMenuState;
	}
}

