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

#include "NetworkGame.h"
#include "NetworkMessage.h"
#include "raknet/RakServer.h"
#include "raknet/BitStream.h"
#include "raknet/GetTime.h"
// We don't need the stringcompressor

NetworkGame::NetworkGame(RakServer& server,
			PlayerID leftPlayer, PlayerID rightPlayer,
			std::string leftPlayerName, std::string rightPlayerName,
			PlayerSide switchedSide)
	: mServer(server)
{
	mPhysicWorld.resetPlayer();

	mLeftPlayer = leftPlayer;
	mRightPlayer = rightPlayer;
	mSwitchedSide = switchedSide;
	mLeftPlayerName = leftPlayerName;
	mRightPlayerName = rightPlayerName;

	mLeftScore = 0;
	mRightScore = 0;
	mServingPlayer = NO_PLAYER;

	mLeftHitcount = 0;
	mRightHitcount = 0;
	mSquishLeft = 0;
	mSquishRight = 0;
	mWinningPlayer = NO_PLAYER;

	mPausing = false;

	// buffer for playernames
	char name[16];

	// writing data into leftStream
	RakNet::BitStream leftStream;
	leftStream.Write((unsigned char)ID_GAME_READY);
	strncpy(name, mRightPlayerName.c_str(), sizeof(name));
	leftStream.Write(name, sizeof(name));
	
	// writing data into rightStream
	RakNet::BitStream rightStream;
	rightStream.Write((unsigned char)ID_GAME_READY);
	strncpy(name, mLeftPlayerName.c_str(), sizeof(name));
	rightStream.Write(name, sizeof(name));
	
	mServer.Send(&leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mLeftPlayer, false);
	mServer.Send(&rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mRightPlayer, false);
}

NetworkGame::~NetworkGame()
{
}

void NetworkGame::injectPacket(Packet* packet)
{
	mPacketQueue.push_back(*packet);
}

void NetworkGame::broadcastBitstream(RakNet::BitStream* stream, RakNet::BitStream* switchedstream)
{
	RakNet::BitStream* leftStream =
		mSwitchedSide == LEFT_PLAYER ? switchedstream : stream;
	RakNet::BitStream* rightStream =
		mSwitchedSide == RIGHT_PLAYER ? switchedstream : stream;

	mServer.Send(leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mLeftPlayer, false);
	mServer.Send(rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mRightPlayer, false);
}

bool NetworkGame::step()
{
	bool active = true;
	
	while (!mPacketQueue.empty())
	{
		Packet* packet = &mPacketQueue.front();
		mPacketQueue.pop_front();

		switch(packet->data[0])
		{
			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_OPPONENT_DISCONNECTED);
				broadcastBitstream(&stream, &stream);
				mPausing = true;
				active = false;
				break;
			}
			case ID_INPUT_UPDATE:
			{
				PlayerInput newInput;
				int ival;
				RakNet::BitStream stream((char*)packet->data,
						packet->length, false);
				
				// ignore ID_INPUT_UPDATE and ID_TIMESTAMP
				stream.IgnoreBytes(1);
				stream.IgnoreBytes(1);
				stream.Read(ival);
				stream.Read(newInput.left);
				stream.Read(newInput.right);
				stream.Read(newInput.up);

				if (packet->playerId == mLeftPlayer)
				{
					if (mSwitchedSide == LEFT_PLAYER)
						newInput.swap();
					mPhysicWorld.setLeftInput(newInput);
				}
				if (packet->playerId == mRightPlayer)
				{
					if (mSwitchedSide == RIGHT_PLAYER)
						newInput.swap();
					mPhysicWorld.setRightInput(newInput);
				}
				break;
			}
			case ID_PAUSE:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_PAUSE);
				broadcastBitstream(&stream, &stream);
				mPausing = true;
				break;
			}
			case ID_UNPAUSE:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_UNPAUSE);
				broadcastBitstream(&stream, &stream);
				mPausing = false;
				break;
			}
			default:
				printf("unknown packet %d recieved\n",
					int(packet->data[0]));
			break;
		}
	};

	if (!mPausing)
		mPhysicWorld.step();

	if (0 == mSquishLeft)
	{
		if (mPhysicWorld.ballHitLeftPlayer())
		{
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
			stream.Write(mPhysicWorld.lastHitIntensity());
			RakNet::BitStream switchStream(stream);
			stream.Write(LEFT_PLAYER);
			switchStream.Write(RIGHT_PLAYER);
			broadcastBitstream(&stream, &switchStream);
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
			stream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
			stream.Write(mPhysicWorld.lastHitIntensity());
			RakNet::BitStream switchStream(stream);
			stream.Write(RIGHT_PLAYER);
			switchStream.Write(LEFT_PLAYER);
			broadcastBitstream(&stream, &switchStream);
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
		stream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		stream.Write(LEFT_PLAYER);
		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		switchStream.Write(RIGHT_PLAYER);
		broadcastBitstream(&stream, &switchStream);

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
		stream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		stream.Write(RIGHT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		switchStream.Write(LEFT_PLAYER);

		broadcastBitstream(&stream, &switchStream);

		if(mRightHitcount > 3)
			mPhysicWorld.dampBall();
		if (mServingPlayer == 0)
			mLeftScore++;
		mServingPlayer = LEFT_PLAYER;
		mPhysicWorld.setBallValidity(0);
		mRightHitcount = 0;
		mLeftHitcount = 0;
	}
	
	if (mLeftScore >= 15 && mLeftScore >= mRightScore + 2)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_WIN_NOTIFICATION);
		stream.Write(LEFT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_WIN_NOTIFICATION);
		switchStream.Write(RIGHT_PLAYER);

		broadcastBitstream(&stream, &switchStream);
		return active;
	}
	if (mRightScore >= 15 && mRightScore >= mLeftScore + 2)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_WIN_NOTIFICATION);
		stream.Write(RIGHT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_WIN_NOTIFICATION);
		switchStream.Write(LEFT_PLAYER);

		broadcastBitstream(&stream, &switchStream);
		return active;
	}

	if (mPhysicWorld.roundFinished())
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_RESET);
		stream.Write(mServingPlayer);
		stream.Write(mLeftScore);
		stream.Write(mRightScore);

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_RESET);
		switchStream.Write(
			mServingPlayer == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER);
		switchStream.Write(mRightScore);
		switchStream.Write(mLeftScore);

		broadcastBitstream(&stream, &switchStream);

		mPhysicWorld.reset(mServingPlayer);
	}

	if (!mPausing)
	{
		broadcastPhysicState();
	}

	return active;
}

void NetworkGame::broadcastPhysicState()
{
	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_PHYSIC_UPDATE);
	stream.Write((unsigned char)ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());
	if (mSwitchedSide == LEFT_PLAYER)
		mPhysicWorld.getSwappedState(&stream);
	else
		mPhysicWorld.getState(&stream);
	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mLeftPlayer, false);

	stream.Reset();
	stream.Write((unsigned char)ID_PHYSIC_UPDATE);
	stream.Write((unsigned char)ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());
	if (mSwitchedSide == RIGHT_PLAYER)
		mPhysicWorld.getSwappedState(&stream);
	else
		mPhysicWorld.getState(&stream);
	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mRightPlayer, false);
}


