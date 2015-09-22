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
#include <stdexcept>
#include <cassert>

#include <boost/make_shared.hpp>

#include "raknet/RakServer.h"
#include "raknet/BitStream.h"
#include "raknet/GetTime.h"


#include "NetworkMessage.h"
#include "ReplayRecorder.h"
#include "FileRead.h"
#include "FileSystem.h"
#include "GenericIO.h"
#include "MatchEvents.h"
#include "PhysicWorld.h"
#include "NetworkPlayer.h"
#include "InputSource.h"

extern int SWLS_GameSteps;

/* implementation */

NetworkGame::NetworkGame(RakServer& server, boost::shared_ptr<NetworkPlayer> leftPlayer,
			boost::shared_ptr<NetworkPlayer> rightPlayer, PlayerSide switchedSide,
			std::string rules, int scoreToWin, float speed) :
	mServer(server),
	mMatch(new DuelMatch(false, rules, scoreToWin)),
	mLeftInput (new InputSource()),
	mRightInput(new InputSource()),
	mRecorder(new ReplayRecorder()),
	mGameValid(true),
	mSpeedController( speed )
{
	// check that both players don't have an active game
	if(leftPlayer->getGame())
	{
		BOOST_THROW_EXCEPTION( std::runtime_error("Trying to start a game with player already in another game!") );
	}

	if(rightPlayer->getGame())
	{
		BOOST_THROW_EXCEPTION( std::runtime_error("Trying to start a game with player already in another game!") );
	}

	mMatch->setPlayers( leftPlayer->getIdentity(), rightPlayer->getIdentity() );
	mMatch->setInputSources(mLeftInput, mRightInput);

	mLeftPlayer = leftPlayer->getID();
	mRightPlayer = rightPlayer->getID();
	mSwitchedSide = switchedSide;

	mRecorder->setPlayerNames(leftPlayer->getName(), rightPlayer->getName());
	mRecorder->setPlayerColors(leftPlayer->getColor(), rightPlayer->getColor());
	mRecorder->setGameSpeed(mSpeedController.getGameSpeed());

	// read rulesfile into a string
	int checksum = 0;
	mRulesLength = 0;
	mRulesSent[0] = false;
	mRulesSent[1] = false;

	rules = FileRead::makeLuaFilename( rules );
	FileRead file(std::string("rules/") + rules);
	checksum = file.calcChecksum(0);
	mRulesLength = file.length();
	mRulesString = file.readRawBytes(mRulesLength);

	// writing rules checksum
	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_RULES_CHECKSUM);
	stream.Write(checksum);
	stream.Write(mMatch->getScoreToWin());
	/// \todo write file author and title, too; maybe add a version number in scripts, too.
	broadcastBitstream(stream);

	// game loop
	mGameThread = std::thread(
		[this]()
		{
			while(mGameValid)
			{
				processPackets();
				step();
				SWLS_GameSteps++;
				mSpeedController.update();
			}
		}					);
}

NetworkGame::~NetworkGame()
{
	mGameValid = false;
	mGameThread.join();
}

void NetworkGame::injectPacket(const packet_ptr& packet)
{
	std::lock_guard<std::mutex> lock(mPacketQueueMutex);
	mPacketQueue.push_back(packet);
}

void NetworkGame::broadcastBitstream(const RakNet::BitStream& stream, const RakNet::BitStream& switchedstream)
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

	assert( &stream != &switchedstream );
	assert( stream.GetData() != switchedstream.GetData() );
	const RakNet::BitStream& leftStream = mSwitchedSide == LEFT_PLAYER ? switchedstream : stream;
	const RakNet::BitStream& rightStream = mSwitchedSide == RIGHT_PLAYER ? switchedstream : stream;

	mServer.Send(&leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
	mServer.Send(&rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
}

void NetworkGame::broadcastBitstream(const RakNet::BitStream& stream)
{

	mServer.Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
	mServer.Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
}

void NetworkGame::processPackets()
{
	while (!mPacketQueue.empty())
	{
		packet_ptr packet;
		{
			std::lock_guard<std::mutex> lock(mPacketQueueMutex);
			packet = mPacketQueue.front();
			mPacketQueue.pop_front();
		}

		processPacket( packet );
	}
}

/// this function processes a single packet received for this network game
void NetworkGame::processPacket( const packet_ptr& packet )
{
	switch(packet->data[0])
	{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
		{
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_OPPONENT_DISCONNECTED);
			broadcastBitstream(stream);
			mMatch->pause();
			mGameValid = false;
			break;
		}

		case ID_INPUT_UPDATE:
		{

			int ival;
			RakNet::BitStream stream((char*)packet->data, packet->length, false);

			// ignore ID_INPUT_UPDATE and ID_TIMESTAMP
			stream.IgnoreBytes(1);
			stream.IgnoreBytes(1);
			stream.Read(ival);
			PlayerInputAbs newInput(stream);

			if (packet->playerId == mLeftPlayer)
			{
				if (mSwitchedSide == LEFT_PLAYER)
					newInput.swapSides();
				mLeftInput->setInput(newInput);
			}
			if (packet->playerId == mRightPlayer)
			{
				if (mSwitchedSide == RIGHT_PLAYER)
					newInput.swapSides();
				mRightInput->setInput(newInput);
			}
			break;
		}

		case ID_PAUSE:
		{
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_PAUSE);
			broadcastBitstream(stream);
			mMatch->pause();
			break;
		}

		case ID_UNPAUSE:
		{
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_UNPAUSE);
			broadcastBitstream(stream);
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
			RakNet::BitStream stream = RakNet::BitStream();
			stream.Write((unsigned char)ID_REPLAY);
			boost::shared_ptr<GenericOut> out = createGenericWriter( &stream );
			mRecorder->send( out );
			assert( stream.GetData()[0] == ID_REPLAY );

			mServer.Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);

			break;
		}

		case ID_RULES:
		{
			boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>((char*)packet->data,
					packet->length, false);
			bool needRules;
			stream->IgnoreBytes(1);
			stream->Read(needRules);
			mRulesSent[mLeftPlayer == packet->playerId ? LEFT_PLAYER : RIGHT_PLAYER] = true;

			if (needRules)
			{
				stream = boost::make_shared<RakNet::BitStream>();
				stream->Write((unsigned char)ID_RULES);
				stream->Write( mRulesLength );
				stream->Write( mRulesString.get(), mRulesLength);
				assert( stream->GetData()[0] == ID_RULES );

				mServer.Send(stream.get(), HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);
			}

			if (isGameStarted())
			{
				// buffer for playernames
				char name[16];

				// writing data into leftStream
				RakNet::BitStream leftStream;
				leftStream.Write((unsigned char)ID_GAME_READY);
				leftStream.Write((int)mSpeedController.getGameSpeed());
				strncpy(name, mMatch->getPlayer(RIGHT_PLAYER).getName().c_str(), sizeof(name));
				leftStream.Write(name, sizeof(name));
				leftStream.Write(mMatch->getPlayer(RIGHT_PLAYER).getStaticColor().toInt());

				// writing data into rightStream
				RakNet::BitStream rightStream;
				rightStream.Write((unsigned char)ID_GAME_READY);
				rightStream.Write((int)mSpeedController.getGameSpeed());
				strncpy(name, mMatch->getPlayer(LEFT_PLAYER).getName().c_str(), sizeof(name));
				rightStream.Write(name, sizeof(name));
				rightStream.Write(mMatch->getPlayer(LEFT_PLAYER).getStaticColor().toInt());

				mServer.Send(&leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
				mServer.Send(&rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
			}

			break;
		}

		default:
			printf("unknown packet %d received\n",
				int(packet->data[0]));
			break;
	}
}

bool NetworkGame::isGameValid() const
{
	return mGameValid;
}


void NetworkGame::step()
{
	if (!isGameStarted())
		return;

	// don't record the pauses
	if(!mMatch->isPaused())
	{
		mRecorder->record(mMatch->getState());

		mMatch->step();

		broadcastGameEvents();

		PlayerSide winning = mMatch->winningPlayer();
		if (winning != NO_PLAYER)
		{
			// if someone has won, the game is paused
			mMatch->pause();
			mRecorder->record(mMatch->getState());
			mRecorder->finalize( mMatch->getScore(LEFT_PLAYER), mMatch->getScore(RIGHT_PLAYER) );

			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_WIN_NOTIFICATION);
			stream.Write(winning);

			RakNet::BitStream switchStream;
			switchStream.Write((unsigned char)ID_WIN_NOTIFICATION);
			switchStream.Write(winning == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER);

			broadcastBitstream(stream, switchStream);
		}

		broadcastPhysicState(mMatch->getState());
	}
}

void NetworkGame::broadcastPhysicState(const DuelMatchState& state) const
{
	DuelMatchState ms = state;	// modifyable copy

	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_GAME_UPDATE);

	/// \todo this required dynamic memory allocation! not good!
	boost::shared_ptr<GenericOut> out = createGenericWriter( &stream );

	if (mSwitchedSide == LEFT_PLAYER)
		ms.swapSides();

	out->generic<DuelMatchState> (ms);
	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, mLeftPlayer, false);

	// reset state and stream
	stream.Reset();
	stream.Write((unsigned char)ID_GAME_UPDATE);

	out = createGenericWriter( &stream );

	// either switch back, or perform switching for right side
	if (mSwitchedSide == LEFT_PLAYER || mSwitchedSide == RIGHT_PLAYER)
		ms.swapSides();

	out->generic<DuelMatchState> (ms);

	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, mRightPlayer, false);
}

// helper function that writes a single event to bit stream in a space efficient way.
void NetworkGame::writeEventToStream(RakNet::BitStream& stream, MatchEvent e, bool switchSides ) const
{
	stream.Write((unsigned char)e.event);
	if( switchSides )
		stream.Write((unsigned char)(e.side == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER ) );
	else
		stream.Write((unsigned char)e.side);
	if( e.event == MatchEvent::BALL_HIT_BLOB )
		stream.Write( e.intensity );
}

void NetworkGame::broadcastGameEvents() const
{
	RakNet::BitStream stream;

	auto events = mMatch->getEvents();
	// send the events
	if( events.empty() )
		return;
	// add all the events to the stream
	stream.Write( (unsigned char)ID_GAME_EVENTS );
	for(auto& e : events)
		writeEventToStream(stream, e, mSwitchedSide == LEFT_PLAYER );
	stream.Write((char)0);
	mServer.Send( &stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);

	stream.Reset();
	stream.Write( (unsigned char)ID_GAME_EVENTS );
	for(auto& e : events)
		writeEventToStream(stream, e, mSwitchedSide == RIGHT_PLAYER );
	stream.Write((char)0);
	mServer.Send( &stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
}

PlayerID NetworkGame::getPlayerID( PlayerSide side ) const
{
	if( side == LEFT_PLAYER )
	{
		return mLeftPlayer;
	}
	else if(side == RIGHT_PLAYER)
	{
		return mRightPlayer;
	}

	assert(0);
}

