#include "NetworkGame.h"
#include "NetworkMessage.h"
#include "raknet/RakServer.h"
#include "raknet/BitStream.h"
#include "raknet/GetTime.h"

NetworkGame::NetworkGame(RakServer& server,
			PlayerID leftPlayer, PlayerID rightPlayer,
			PlayerSide switchedSide)
	: mServer(server)
{
	mPhysicWorld.resetPlayer();

	mLeftPlayer = leftPlayer;
	mRightPlayer = rightPlayer;
	mSwitchedSide = switchedSide;

	mLeftScore = 0;
	mRightScore = 0;
	mServingPlayer = NO_PLAYER;

	mLeftHitcount = 0;
	mRightHitcount = 0;
	mSquishLeft = 0;
	mSquishRight = 0;
	mWinningPlayer = NO_PLAYER;

	RakNet::BitStream stream;
	stream.Write(ID_GAME_READY);
	broadcastBitstream(&stream, true);
}

NetworkGame::~NetworkGame()
{
}

void NetworkGame::injectPacket(Packet* packet)
{
	mPacketQueue.push(*packet);
}

void NetworkGame::broadcastBitstream(RakNet::BitStream* stream, bool reliable)
{
 	PacketReliability reliability =
		reliable ? RELIABLE_ORDERED : UNRELIABLE_SEQUENCED;

	mServer.Send(stream, HIGH_PRIORITY, reliability, 0,
                        mLeftPlayer, false);
	mServer.Send(stream, HIGH_PRIORITY, reliability, 0,
                        mRightPlayer, false);
}

bool NetworkGame::step()
{
	while (!mPacketQueue.empty())
	{
		Packet* packet = &mPacketQueue.front();
		mPacketQueue.pop();

		switch(packet->data[0])
		{
			case ID_CONNECTION_LOST:
				printf("client lost connection\n");
				break;
			case ID_INPUT_UPDATE:
			{
				PlayerInput newInput;
				int ival;
				RakNet::BitStream stream((char*)packet->data,
						packet->length, false);
				stream.Read(ival);
				stream.Read(ival);
				stream.Read(ival);
				stream.Read(newInput.left);
				stream.Read(newInput.right);
				stream.Read(newInput.up);

				if (packet->playerId == mLeftPlayer)
					mPhysicWorld.setLeftInput(newInput);
				if (packet->playerId == mRightPlayer)
					mPhysicWorld.setRightInput(newInput);
				break;
			}
			default:
				printf("unknown packet %d recieved\n",
					int(packet->data[0]));
			break;
		}
	};

	mPhysicWorld.step();

	if (0 == mSquishLeft)
	{
		if (mPhysicWorld.ballHitLeftPlayer())
		{
			RakNet::BitStream stream;
			stream.Write(ID_BALL_PLAYER_COLLISION);
			stream.Write(mPhysicWorld.lastHitIntensity());
			stream.Write(LEFT_PLAYER);
			broadcastBitstream(&stream, true);
			mLeftHitcount++;
			mRightHitcount = 0;
			mSquishLeft = 1;
		}
	}
	else
	{
		mSquishLeft += 1;
		if(mSquishLeft > 9)
			mSquishLeft=0;
	}

	if(0 == mSquishRight)
	{
		if (mPhysicWorld.ballHitRightPlayer())
		{
			RakNet::BitStream stream;
			stream.Write(ID_BALL_PLAYER_COLLISION);
			stream.Write(mPhysicWorld.lastHitIntensity());
			stream.Write(RIGHT_PLAYER);
			broadcastBitstream(&stream, true);
			mRightHitcount++;
			mLeftHitcount = 0;
			mSquishRight = 1;
		}
	}
	else
	{
		mSquishRight += 1;
		if(mSquishRight > 9)
			mSquishRight=0;	
	}

	if (mPhysicWorld.ballHitLeftGround() || mLeftHitcount > 3)
	{
		RakNet::BitStream stream;
		stream.Write(ID_BALL_GROUND_COLLISION);
		stream.Write(LEFT_PLAYER);
		broadcastBitstream(&stream, true);

		if (mLeftHitcount > 3)
			mPhysicWorld.dampBall();
		if (mServingPlayer == 1)
			mRightScore++;
		mServingPlayer = RIGHT_PLAYER;
		mPhysicWorld.setBallValidity(0);
		mRightHitcount = 0;
		mLeftHitcount = 0;
	}

	if (mPhysicWorld.ballHitRightGround() || mRightHitcount > 3)
	{
		RakNet::BitStream stream;
		stream.Write(ID_BALL_GROUND_COLLISION);
		stream.Write(RIGHT_PLAYER);
		broadcastBitstream(&stream, true);

		if(mRightHitcount > 3)
			mPhysicWorld.dampBall();
		if (mServingPlayer == 0)
			mLeftScore++;
		mServingPlayer = LEFT_PLAYER;
		mPhysicWorld.setBallValidity(0);
		mRightHitcount = 0;
		mLeftHitcount = 0;
	}

	if (mPhysicWorld.roundFinished())
	{
		RakNet::BitStream stream;
		stream.Write(ID_BALL_RESET);
		stream.Write(mServingPlayer);
		stream.Write(mLeftScore);
		stream.Write(mRightScore);
		broadcastBitstream(&stream, true);
		mPhysicWorld.reset(mServingPlayer);
	}

	RakNet::BitStream stream;
	stream.Write(ID_PHYSIC_UPDATE);
	stream.Write(ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());
	mPhysicWorld.getState(&stream);
	broadcastBitstream(&stream, false);

	return true;
}
