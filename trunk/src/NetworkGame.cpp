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
#include "raknet/StringCompressor.h"

NetworkGame::NetworkGame(RakServer& server,
			PlayerID leftPlayer, PlayerID rightPlayer,
			std::string leftPlayerName, std::string rightPlayerName,
			float gameSpeed, PlayerSide switchedSide)
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

	mGameSpeed = gameSpeed;
	mGameFPSController = new SpeedController();
	SpeedController::setCurrentGameFPSInstance(mGameFPSController);

	mPausing = false;

	RakNet::BitStream leftStream;
	leftStream.Write(ID_GAME_READY);
	StringCompressor::Instance()->EncodeString((char*)mRightPlayerName.c_str(), 16, &leftStream);
	leftStream.Write(mGameSpeed);

	RakNet::BitStream rightStream;
	rightStream.Write(ID_GAME_READY);
	StringCompressor::Instance()->EncodeString((char*)mLeftPlayerName.c_str(), 16, &rightStream);
	rightStream.Write(mGameSpeed);

	mServer.Send(&leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mLeftPlayer, false);
	mServer.Send(&rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mRightPlayer, false);
}

NetworkGame::~NetworkGame()
{
	delete mGameFPSController;
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
	SpeedController::setCurrentGameFPSInstance(mGameFPSController);
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
				stream.Write(ID_OPPONENT_DISCONNECTED);
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
				stream.Read(ival);
				stream.Read(ival);	//ID_TIMESTAMP
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
				stream.Write(ID_PAUSE);
				broadcastBitstream(&stream, &stream);
				mPausing = true;
				break;
			}
			case ID_UNPAUSE:
			{
				RakNet::BitStream stream;
				stream.Write(ID_UNPAUSE);
				broadcastBitstream(&stream, &stream);
				mPausing = false;
				mGameFPSController->endPause();
				break;
			}
			default:
				printf("unknown packet %d recieved\n",
					int(packet->data[0]));
			break;
		}
	};

	if (!mGameFPSController->beginFrame())
		return active;

	float timeDelta = mGameFPSController->getTimeDelta();

	if (!mPausing)
		mPhysicWorld.step(timeDelta, mGameSpeed);

	float time = SpeedController::getGameFPS()/1000 * mGameSpeed;

	if (0 == mSquishLeft)
	{
		if (mPhysicWorld.ballHitLeftPlayer())
		{
			RakNet::BitStream stream;
			stream.Write(ID_BALL_PLAYER_COLLISION);
			stream.Write(mPhysicWorld.lastHitIntensity());
			broadcastBitstream(&stream, &stream);
			mLeftHitcount++;
			mRightHitcount = 0;
			mSquishLeft = time;
		}
	}
	else
	{
		mSquishLeft += time;
		if(mSquishLeft > 9*time)
			mSquishLeft=0;
	}

	if(0 == mSquishRight)
	{
		if (mPhysicWorld.ballHitRightPlayer())
		{
			RakNet::BitStream stream;
			stream.Write(ID_BALL_PLAYER_COLLISION);
			stream.Write(mPhysicWorld.lastHitIntensity());
			broadcastBitstream(&stream, &stream);
			mRightHitcount++;
			mLeftHitcount = 0;
			mSquishRight = time;
		}
	}
	else
	{
		mSquishRight += time;
		if(mSquishRight > 9*time)
			mSquishRight=0;
	}

	if (mPhysicWorld.ballHitLeftGround() || mLeftHitcount > 3)
	{
		RakNet::BitStream stream;
		stream.Write(ID_BALL_GROUND_COLLISION);

		broadcastBitstream(&stream, &stream);

		if (mLeftHitcount > 3)
			mPhysicWorld.dampBall();
		if (mServingPlayer == 1)
			mRightScore++;

		mServingPlayer = RIGHT_PLAYER;
		mPhysicWorld.setBallValidity(0);
		mRightHitcount = 0;
		mLeftHitcount = 0;
		mSquishRight = 0;
		mSquishLeft = 0;
	}

	if (mPhysicWorld.ballHitRightGround() || mRightHitcount > 3)
	{
		RakNet::BitStream stream;
		stream.Write(ID_BALL_GROUND_COLLISION);

		broadcastBitstream(&stream, &stream);

		if(mRightHitcount > 3)
			mPhysicWorld.dampBall();
		if (mServingPlayer == 0)
			mLeftScore++;

		mServingPlayer = LEFT_PLAYER;
		mPhysicWorld.setBallValidity(0);
		mRightHitcount = 0;
		mLeftHitcount = 0;
		mSquishRight = 0;
		mSquishLeft = 0;
	}

	if (mLeftScore >= 15 && mLeftScore >= mRightScore + 2)
	{
		RakNet::BitStream stream;
		stream.Write(ID_WIN_NOTIFICATION);
		stream.Write(LEFT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write(ID_WIN_NOTIFICATION);
		switchStream.Write(RIGHT_PLAYER);

		broadcastBitstream(&stream, &switchStream);
		return active;
	}
	if (mRightScore >= 15 && mRightScore >= mLeftScore + 2)
	{
		RakNet::BitStream stream;
		stream.Write(ID_WIN_NOTIFICATION);
		stream.Write(RIGHT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write(ID_WIN_NOTIFICATION);
		switchStream.Write(LEFT_PLAYER);

		broadcastBitstream(&stream, &switchStream);
		return active;
	}

	if (mPhysicWorld.roundFinished())
	{
		RakNet::BitStream stream;
		stream.Write(ID_BALL_RESET);
		stream.Write(mServingPlayer);
		stream.Write(mLeftScore);
		stream.Write(mRightScore);

		RakNet::BitStream switchStream;
		switchStream.Write(ID_BALL_RESET);
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

	mGameFPSController->endFrame();

	return active;
}

void NetworkGame::broadcastPhysicState()
{
	RakNet::BitStream stream;
	stream.Write(ID_PHYSIC_UPDATE);
	stream.Write(ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());
	if (mSwitchedSide == LEFT_PLAYER)
		mPhysicWorld.getSwappedState(&stream);
	else
		mPhysicWorld.getState(&stream);
	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mLeftPlayer, false);

	stream.Reset();
	stream.Write(ID_PHYSIC_UPDATE);
	stream.Write(ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());
	if (mSwitchedSide == RIGHT_PLAYER)
		mPhysicWorld.getSwappedState(&stream);
	else
		mPhysicWorld.getState(&stream);
	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mRightPlayer, false);
}
