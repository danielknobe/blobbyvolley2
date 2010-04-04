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
			Color leftColor, Color rightColor, 
			PlayerSide switchedSide)
	: mServer(server)
{
	mLeftInput = new DummyInputSource();
	mRightInput = new DummyInputSource();
	mMatch = new DuelMatch(mLeftInput, mRightInput, false);

	mLeftPlayer = leftPlayer;
	mRightPlayer = rightPlayer;
	mSwitchedSide = switchedSide;
	mLeftPlayerName = leftPlayerName;
	mRightPlayerName = rightPlayerName;

	mWinningPlayer = NO_PLAYER;

	mPausing = false;

	// buffer for playernames
	char name[16];

	// writing data into leftStream
	RakNet::BitStream leftStream;
	leftStream.Write((unsigned char)ID_GAME_READY);
	leftStream.Write((int)SpeedController::getMainInstance()->getGameSpeed());
	strncpy(name, mRightPlayerName.c_str(), sizeof(name));
	leftStream.Write(name, sizeof(name));
	leftStream.Write(rightColor.toInt());
	
	// writing data into rightStream
	RakNet::BitStream rightStream;
	rightStream.Write((unsigned char)ID_GAME_READY);
	rightStream.Write((int)SpeedController::getMainInstance()->getGameSpeed());
	strncpy(name, mLeftPlayerName.c_str(), sizeof(name));
	rightStream.Write(name, sizeof(name));
	rightStream.Write(leftColor.toInt());
	
	mServer.Send(&leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mLeftPlayer, false);
	mServer.Send(&rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mRightPlayer, false);
}

NetworkGame::~NetworkGame()
{
	delete mLeftInput;
	delete mRightInput;
	delete mMatch;
}
void NetworkGame::injectPacket(const packet_ptr& packet)
{
	mPacketQueue.push_back(packet);
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
		packet_ptr packet = mPacketQueue.front();
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
				mMatch->pause();
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
					mLeftInput->setInput(newInput);
				}
				if (packet->playerId == mRightPlayer)
				{
					if (mSwitchedSide == RIGHT_PLAYER)
						newInput.swap();
					mRightInput->setInput(newInput);
				}
				break;
			}
			case ID_PAUSE:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_PAUSE);
				broadcastBitstream(&stream, &stream);
				mPausing = true;
				mMatch->pause();
				break;
			}
			case ID_UNPAUSE:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_UNPAUSE);
				broadcastBitstream(&stream, &stream);
				mPausing = false;
				mMatch->unpause();
				break;
			}
			case ID_CHAT_MESSAGE:
			{	RakNet::BitStream stream((char*)packet->data,
						packet->length, false);
				
				stream.IgnoreBytes(1); // ID_CHAT_MESSAGE
				char message[31];
				stream.Read(message, sizeof(message));

				RakNet::BitStream stream2;
				stream2.Write((unsigned char)ID_CHAT_MESSAGE);
				stream2.Write(message, sizeof(message));
				if (mLeftPlayer == packet->playerId)
					mServer.Send(&stream2, LOW_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
				else
					mServer.Send(&stream2, LOW_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
			break;
			}
			default:
				printf("unknown packet %d received\n",
					int(packet->data[0]));
			break;
		}
	}
	
	mMatch->step();

	int events = mMatch->getEvents();
	if(events & DuelMatch::EVENT_LEFT_BLOBBY_HIT)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		stream.Write(mMatch->getWorld().lastHitIntensity());
		RakNet::BitStream switchStream(stream);
		stream.Write(LEFT_PLAYER);
		switchStream.Write(RIGHT_PLAYER);
		broadcastBitstream(&stream, &switchStream);
	}
	
	if(events & DuelMatch::EVENT_RIGHT_BLOBBY_HIT)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		stream.Write(mMatch->getWorld().lastHitIntensity());
		RakNet::BitStream switchStream(stream);
		stream.Write(RIGHT_PLAYER);
		switchStream.Write(LEFT_PLAYER);
		broadcastBitstream(&stream, &switchStream);
	}
	
	if(events & DuelMatch::EVENT_ERROR_LEFT)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		stream.Write(LEFT_PLAYER);
		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		switchStream.Write(RIGHT_PLAYER);
		broadcastBitstream(&stream, &switchStream);
	}
	
	if(events & DuelMatch::EVENT_ERROR_RIGHT)
	{
		RakNet::BitStream stream;
		// is it correct to send ID_BALL_GROUND_COLLISION even if the
		// error was a forth hit of a player?
		stream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		stream.Write(RIGHT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		switchStream.Write(LEFT_PLAYER);

		broadcastBitstream(&stream, &switchStream);
	}
	
	if(!mPausing)
		switch(mMatch->winningPlayer()){
			case LEFT_PLAYER:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_WIN_NOTIFICATION);
				stream.Write(LEFT_PLAYER);
		
				RakNet::BitStream switchStream;
				switchStream.Write((unsigned char)ID_WIN_NOTIFICATION);
				switchStream.Write(RIGHT_PLAYER);
		
				broadcastBitstream(&stream, &switchStream);
				
				// if someone has won, the game is paused 
				mPausing = true;
				mMatch->pause();
				return active;
			}
			break;
			case RIGHT_PLAYER:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_WIN_NOTIFICATION);
				stream.Write(RIGHT_PLAYER);
		
				RakNet::BitStream switchStream;
				switchStream.Write((unsigned char)ID_WIN_NOTIFICATION);
				switchStream.Write(LEFT_PLAYER);
		
				broadcastBitstream(&stream, &switchStream);
				
				// if someone has won, the game is paused 
				mPausing = true;
				mMatch->pause();
				return active;
			}
			break;
		}

	if (events & DuelMatch::EVENT_RESET)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_RESET);
		stream.Write(mMatch->getServingPlayer());
		stream.Write(mMatch->getScore(LEFT_PLAYER));
		stream.Write(mMatch->getScore(RIGHT_PLAYER));
		stream.Write(mMatch->getClock().getTime());

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_RESET);
		switchStream.Write(
			mMatch->getServingPlayer() == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER);
		switchStream.Write(mMatch->getScore(RIGHT_PLAYER));
		switchStream.Write(mMatch->getScore(LEFT_PLAYER));
		switchStream.Write(mMatch->getClock().getTime());

		broadcastBitstream(&stream, &switchStream);
	}

	if (!mPausing)
	{
		broadcastPhysicState();
	}

	return active;
}

void NetworkGame::broadcastPhysicState()
{
	const PhysicWorld& world = mMatch->getWorld();
	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_PHYSIC_UPDATE);
	stream.Write((unsigned char)ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());
	if (mSwitchedSide == LEFT_PLAYER)
		world.getSwappedState(&stream);
	else
		world.getState(&stream);
	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mLeftPlayer, false);

	stream.Reset();
	stream.Write((unsigned char)ID_PHYSIC_UPDATE);
	stream.Write((unsigned char)ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());
	if (mSwitchedSide == RIGHT_PLAYER)
		world.getSwappedState(&stream);
	else
		world.getState(&stream);
	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mRightPlayer, false);
}


