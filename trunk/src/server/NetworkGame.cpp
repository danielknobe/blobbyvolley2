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


/* implementation */

NetworkGame::NetworkGame(RakServer& server, boost::shared_ptr<NetworkPlayer> leftPlayer, boost::shared_ptr<NetworkPlayer> rightPlayer,
			PlayerSide switchedSide, std::string rules)
: mServer(server)
, mMatch(new DuelMatch(false, rules))
, mLeftInput (new InputSource())
, mRightInput(new InputSource())
, mRecorder(new ReplayRecorder())
, mPausing(false)
, mGameValid(true)
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
	mRecorder->setGameSpeed(SpeedController::getMainInstance()->getGameSpeed());

	// read rulesfile into a string
	int checksum = 0;
	mRulesLength = 0;
	mRulesSent[0] = false;
	mRulesSent[1] = false;

	FileRead file(std::string("rules/") + rules);
	checksum = file.calcChecksum(0);
	mRulesLength = file.length();
	mRulesString = file.readRawBytes(mRulesLength);

	// writing rules checksum
	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_RULES_CHECKSUM);
	stream.Write(checksum);
	/// \todo write file author and title, too; maybe add a version number in scripts, too.
	broadcastBitstream(&stream);
}

NetworkGame::~NetworkGame()
{
}

void NetworkGame::injectPacket(const packet_ptr& packet)
{
	mPacketQueue.push_back(packet);
}

void NetworkGame::broadcastBitstream(const RakNet::BitStream* stream, const RakNet::BitStream* switchedstream)
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
	const RakNet::BitStream* leftStream = mSwitchedSide == LEFT_PLAYER ? switchedstream : stream;
	const RakNet::BitStream* rightStream = mSwitchedSide == RIGHT_PLAYER ? switchedstream : stream;

	mServer.Send(leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
	mServer.Send(rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
}

void NetworkGame::broadcastBitstream(const RakNet::BitStream* stream)
{

	mServer.Send(stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
	mServer.Send(stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
}

void NetworkGame::processPackets()
{
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
				boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>();
				bool needRules;
				stream->Read(needRules);
				mRulesSent[mLeftPlayer == packet->playerId ? LEFT_PLAYER : RIGHT_PLAYER] = true;

				if (needRules)
				{
					stream = boost::make_shared<RakNet::BitStream>();
					stream->Write((unsigned char)ID_RULES);
					stream->Write(mRulesLength);
					stream->Write(mRulesString.get(), mRulesLength);
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
					leftStream.Write((int)SpeedController::getMainInstance()->getGameSpeed());
					strncpy(name, mMatch->getPlayer(RIGHT_PLAYER).getName().c_str(), sizeof(name));
					leftStream.Write(name, sizeof(name));
					leftStream.Write(mMatch->getPlayer(RIGHT_PLAYER).getStaticColor().toInt());

					// writing data into rightStream
					RakNet::BitStream rightStream;
					rightStream.Write((unsigned char)ID_GAME_READY);
					rightStream.Write((int)SpeedController::getMainInstance()->getGameSpeed());
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
		mRecorder->record(mMatch->getState());

	mMatch->step();

	int events = mMatch->getEvents();
	if(events & EVENT_LEFT_BLOBBY_HIT)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		stream.Write(mMatch->getWorld().getLastHitIntensity());
		stream.Write(LEFT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		switchStream.Write(mMatch->getWorld().getLastHitIntensity());
		switchStream.Write(RIGHT_PLAYER);

		broadcastBitstream(&stream, &switchStream);
	}

	if(events & EVENT_RIGHT_BLOBBY_HIT)
	{
		RakNet::BitStream stream;
		stream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		stream.Write(mMatch->getWorld().getLastHitIntensity());
		stream.Write(RIGHT_PLAYER);

		RakNet::BitStream switchStream;
		switchStream.Write((unsigned char)ID_BALL_PLAYER_COLLISION);
		switchStream.Write(mMatch->getWorld().getLastHitIntensity());
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
		PlayerSide winning = mMatch->winningPlayer();
		if (winning != NO_PLAYER)
		{
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_WIN_NOTIFICATION);
			stream.Write(winning);

			RakNet::BitStream switchStream;
			switchStream.Write((unsigned char)ID_WIN_NOTIFICATION);
			switchStream.Write(winning == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER);

			broadcastBitstream(&stream, &switchStream);

			// if someone has won, the game is paused
			mPausing = true;
			mMatch->pause();
			mRecorder->record(mMatch->getState());
			mRecorder->finalize( mMatch->getScore(LEFT_PLAYER), mMatch->getScore(RIGHT_PLAYER) );
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
}

void NetworkGame::broadcastPhysicState()
{
	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_PHYSIC_UPDATE);
	stream.Write((unsigned char)ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());
	DuelMatchState ms = mMatch->getState();

	boost::shared_ptr<GenericOut> out = createGenericWriter( &stream );

	if (mSwitchedSide == LEFT_PLAYER)
		ms.swapSides();

	out->generic<DuelMatchState> (ms);

	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mLeftPlayer, false);

	// reset state and stream
	ms = mMatch->getState();
	stream.Reset();
	stream.Write((unsigned char)ID_PHYSIC_UPDATE);
	stream.Write((unsigned char)ID_TIMESTAMP);
	stream.Write(RakNet::GetTime());

	out = createGenericWriter( &stream );

	if (mSwitchedSide == RIGHT_PLAYER)
		ms.swapSides();

	out->generic<DuelMatchState> (ms);

	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
		mRightPlayer, false);
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

