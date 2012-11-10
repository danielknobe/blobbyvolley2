/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

/* header include */
#include "NetworkGame.h"

/* includes */
#include <sstream>
#include <iostream>

#include <boost/make_shared.hpp>

#include "raknet/RakServer.h"
#include "raknet/BitStream.h"
#include "raknet/GetTime.h"


#include "NetworkMessage.h"
#include "ReplayRecorder.h"
#include "FileRead.h"
#include "FileSystem.h"
#include "GenericIO.h"


/* implementation */

NetworkGame::NetworkGame(RakServer& server,
			PlayerID leftPlayer, PlayerID rightPlayer,
			std::string leftPlayerName, std::string rightPlayerName,
			Color leftColor, Color rightColor,
			PlayerSide switchedSide, std::string rules)
	: 	mServer(server),
		mLeftInput (new DummyInputSource()),
		mRightInput(new DummyInputSource())

{
	mMatch = new DuelMatch(mLeftInput.get(), mRightInput.get(), false, false, rules);

	mLeftPlayer = leftPlayer;
	mRightPlayer = rightPlayer;
	mSwitchedSide = switchedSide;
	mLeftPlayerName = leftPlayerName;
	mRightPlayerName = rightPlayerName;

	mWinningPlayer = NO_PLAYER;

	mPausing = false;

	mRecorder.reset(new ReplayRecorder());
	mRecorder->setPlayerNames(mLeftPlayerName.c_str(), mRightPlayerName.c_str());
	mRecorder->setPlayerColors(leftColor, rightColor);
	mRecorder->setGameSpeed(SpeedController::getMainInstance()->getGameSpeed());

	// buffer for playernames
	char name[16];
	
	// read rulesfile into a string
	int rulesLength = 0;
	boost::shared_array<char> rulesString;
	try
	{
		FileRead file(rules);
		rulesLength = file.length();
		rulesString = file.readRawBytes(rulesLength);
	} 
	 catch (FileLoadException& ex)
	{
		std::cerr << "could not sent rules file to client: " << ex.what() << "\n"; 
	}
	

	// writing data into leftStream
	RakNet::BitStream leftStream;
	leftStream.Write((unsigned char)ID_GAME_READY);
	leftStream.Write((int)SpeedController::getMainInstance()->getGameSpeed());
	strncpy(name, mRightPlayerName.c_str(), sizeof(name));
	leftStream.Write(name, sizeof(name));
	leftStream.Write(rightColor.toInt());
	leftStream.Write(rulesLength);
	if(rulesLength)
		leftStream.Write(rulesString.get(), rulesLength);

	// writing data into rightStream
	RakNet::BitStream rightStream;
	rightStream.Write((unsigned char)ID_GAME_READY);
	rightStream.Write((int)SpeedController::getMainInstance()->getGameSpeed());
	strncpy(name, mLeftPlayerName.c_str(), sizeof(name));
	rightStream.Write(name, sizeof(name));
	rightStream.Write(leftColor.toInt());
	rightStream.Write(rulesLength);
	if(rulesLength)
		rightStream.Write(rulesString.get(), rulesLength);

	mServer.Send(&leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mLeftPlayer, false);
	mServer.Send(&rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mRightPlayer, false);
}

NetworkGame::~NetworkGame()
{
	delete mMatch;
}

void NetworkGame::injectPacket(const packet_ptr& packet)
{
	mPacketQueue.push_back(packet);
}

void NetworkGame::broadcastBitstream(RakNet::BitStream* stream, RakNet::BitStream* switchedstream)
{
	// checks that stream and switchedstream don't have the same content.
	// this is a common mistake that arises from constructs like:
	//		BitStream stream
	//		... fill common data into stream
	//		BitStream switchedstream
	//		.. fill data depending on side in both streams
	//		broadcastBistream(stream, switchedstream)
	//
	//	here, the internal data of switchedstream is the same as stream so all
	//	changes made with switchedstream are done with stream alike. this was not
	//  the intention of this construct so it should be caught by this assertion.
	/// NEVER USE THIS FUNCTION LIKE broadcastBitstream(str, str), use, broadcastBitstream(str) instead
	/// this function is intended for sending two different streams to the two clients

	assert( stream != switchedstream );
	assert( stream->GetData() != switchedstream->GetData() );

	RakNet::BitStream* leftStream =
		mSwitchedSide == LEFT_PLAYER ? switchedstream : stream;
	RakNet::BitStream* rightStream =
		mSwitchedSide == RIGHT_PLAYER ? switchedstream : stream;

	mServer.Send(leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mLeftPlayer, false);
	mServer.Send(rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mRightPlayer, false);
}

void NetworkGame::broadcastBitstream(RakNet::BitStream* stream)
{

	mServer.Send(stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
                        mLeftPlayer, false);
	mServer.Send(stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
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
				broadcastBitstream(&stream);
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
				broadcastBitstream(&stream);
				mPausing = true;
				mMatch->pause();
				break;
			}

			case ID_UNPAUSE:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_UNPAUSE);
				broadcastBitstream(&stream);
				mPausing = false;
				mMatch->unpause();
				break;
			}

			case ID_CHAT_MESSAGE:
			{	RakNet::BitStream stream((char*)packet->data,
						packet->length, false);

				stream.IgnoreBytes(1); // ID_CHAT_MESSAGE
				char message[31];
				/// \todo we need to acertain that this package contains at least 31 bytes!
				///			otherwise, we send just uninitialized memory to the client
				///			thats no real security problem but i think we should address
				///			this nonetheless
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

			case ID_REPLAY:
			{
				boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>();
				stream->Write((unsigned char)ID_REPLAY);
				boost::shared_ptr<GenericOut> out = createGenericWriter(stream);
				mRecorder->send( out );
				assert( stream->GetData()[0] == ID_REPLAY );

				mServer.Send(stream.get(), LOW_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);

				break;
			}

			default:
				printf("unknown packet %d received\n",
					int(packet->data[0]));
				break;
		}
	}

	// don't record the pauses
	if(!mMatch->isPaused())
		mRecorder->record(mMatch->getState());

	mMatch->step();

	int events = mMatch->getEvents();
	if(events & EVENT_LEFT_BLOBBY_HIT)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		stream.Write(mMatch->getWorld().lastHitIntensity());
		stream.Write(LEFT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		switchStream.Write(mMatch->getWorld().lastHitIntensity());
		switchStream.Write(RIGHT_PLAYER);

		broadcastBitstream(&stream, &switchStream);
	}

	if(events & EVENT_RIGHT_BLOBBY_HIT)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		stream.Write(mMatch->getWorld().lastHitIntensity());
		stream.Write(RIGHT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		switchStream.Write(mMatch->getWorld().lastHitIntensity());
		switchStream.Write(LEFT_PLAYER);

		broadcastBitstream(&stream, &switchStream);
	}

	if(events & EVENT_BALL_HIT_LEFT_GROUND)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		stream.Write(LEFT_PLAYER);
		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_GROUND_COLLISION);
		switchStream.Write(RIGHT_PLAYER);
		broadcastBitstream(&stream, &switchStream);
	}

	if(events & EVENT_BALL_HIT_RIGHT_GROUND)
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
	{
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
				mRecorder->finalize( mMatch->getScore(LEFT_PLAYER), mMatch->getScore(RIGHT_PLAYER) );
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
				mRecorder->finalize( mMatch->getScore(LEFT_PLAYER), mMatch->getScore(RIGHT_PLAYER) );
				return active;
			}
			break;
		}
	}

	if (events & EVENT_RESET)
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
	PhysicState ps = world.getState();

	if (mSwitchedSide == LEFT_PLAYER)
		ps.swapSides();
	ps.writeToStream(&stream);

	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mLeftPlayer, false);

	// reset state and stream
	ps = world.getState();
	stream.Reset();
	stream.Write((unsigned char)ID_PHYSIC_UPDATE);
	stream.Write((unsigned char)ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());

	if (mSwitchedSide == RIGHT_PLAYER)
		ps.swapSides();

	ps.writeToStream(&stream);

	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mRightPlayer, false);
}


