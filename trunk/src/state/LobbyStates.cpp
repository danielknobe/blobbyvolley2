#include "LobbyStates.h"

#include <stdexcept>
#include <algorithm>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include "raknet/RakClient.h"

#include "IMGUI.h"
#include "NetworkState.h"
#include "NetworkSearchState.h"
#include "UserConfig.h"
#include "GenericIO.h"
#include "GameLogic.h"

LobbyState::LobbyState(ServerInfo info, PreviousState previous) :
		mClient(new RakClient(), [](RakClient* client) { client->Disconnect(25); }),
		mInfo(info), mPrevious( previous ),
		mLobbyState(ConnectionState::CONNECTING)
{
	if (!mClient->Connect(mInfo.hostname, mInfo.port, 0, 0, RAKNET_THREAD_SLEEP_TIME))
		throw( std::runtime_error(std::string("Could not connect to server ") + mInfo.hostname) );

	// send an ENTER_SERVER packet with name and side preference
	RenderManager::getSingleton().redraw();

	/// \todo we need read-only access here!
	UserConfig config;
	config.loadFile("config.xml");
	PlayerSide side = (PlayerSide)config.getInteger("network_side");

	// load player identity
	if(side == LEFT_PLAYER)
	{
		mLocalPlayer = config.loadPlayerIdentity(LEFT_PLAYER, true);
	}
	 else
	{
		mLocalPlayer = config.loadPlayerIdentity(RIGHT_PLAYER, true);
	}
}

LobbyState::~LobbyState()
{
	// disconnect is handled by shared ptr
}

void LobbyState::step_impl()
{
	// process packets
	packet_ptr packet;
	while (packet = mClient->Receive())
	{
		switch(packet->data[0])
		{
			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
				RakNet::BitStream stream = makeEnterServerPacket( mLocalPlayer );
				mClient->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0);

				mSubState = boost::make_shared<LobbyMainSubstate>(mClient, 0, 0, 3);
				break;
			}
			case ID_CONNECTION_ATTEMPT_FAILED:
			{
				mLobbyState = ConnectionState::CONNECTION_FAILED;
				break;
			}

			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
			{
				mLobbyState = ConnectionState::DISCONNECTED;
				break;
			};
			// we ignore these packets. they tell us about remote connections, which we handle manually with ID_SERVER_STATUS packets.
			case ID_REMOTE_EXISTING_CONNECTION:
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				// not surprising, but we are not interested in his packet
				break;
			case ID_LOBBY:
				{
				mLobbyState = ConnectionState::CONNECTED;
				RakNet::BitStream stream = packet->getStream();
				auto in = createGenericReader( &stream );
				unsigned char t;
				in->byte(t);
				in->byte(t);
				if((LobbyPacketType)t == LobbyPacketType::SERVER_STATUS)
				{
					uint32_t player_count;
					in->uint32( player_count );
					in->generic<std::vector<unsigned int>>( mStatus.mPossibleSpeeds );
					in->generic<std::vector<std::string>>( mStatus.mPossibleRules );
					in->generic<std::vector<std::string>>( mStatus.mPossibleRulesAuthor );

					std::vector<unsigned int> gameids;
					std::vector<std::string> gamenames;
					std::vector<unsigned char> gamespeeds;
					std::vector<unsigned char> gamerules;
					std::vector<unsigned char> gamescores;
					in->generic<std::vector<unsigned int>>( gameids );
					in->generic<std::vector<std::string>>( gamenames );
					in->generic<std::vector<unsigned char>>( gamespeeds );
					in->generic<std::vector<unsigned char>>( gamerules );
					in->generic<std::vector<unsigned char>>( gamescores );

					mStatus.mOpenGames.clear();
					for( unsigned i = 0; i < gameids.size(); ++i)
					{
						mStatus.mOpenGames.push_back( ServerStatusData::OpenGame{ gameids.at(i), gamenames.at(i), gamerules.at(i), gamespeeds.at(i), gamescores.at(i)});
					}

					// find out which settings most closely resemble the local config
					bool first_config = mPreferedSpeed == -1; // detect whether we set config for the first time
					boost::shared_ptr<IUserConfigReader> config = IUserConfigReader::createUserConfigReader("config.xml");

					// speed
					int speed = config->getInteger("gamefps");
					auto& speeds = mStatus.mPossibleSpeeds;
					auto closest_speed = std::min_element( speeds.begin(), speeds.end(),
									[speed](int a, int b){ return std::abs(a-speed) < std::abs(b-speed); } );
					mPreferedSpeed = std::distance( speeds.begin(), closest_speed );

					// rules
					auto gamelogic = createGameLogic(config->getString("rules"), nullptr, 1);
					std::string rule = gamelogic->getTitle();
					auto& rules = mStatus.mPossibleRules;
					auto found = std::find( rules.begin(), rules.end(), rule);
					/// \todo we need to open the lua file here to get the actual ruleset name.
					if( found != rules.end())
						mPreferedRules = std::distance( rules.begin(), found );

					// points
					int points = config->getInteger( "scoretowin" );
					std::array<unsigned, 8> scores{2, 5, 10, 15, 20, 25, 40, 50};
					auto closest_score = std::min_element( scores.begin(), scores.end(),
									[points](int a, int b){ return std::abs(a-points) < std::abs(b-points); } );
					mPreferedScore = std::distance( scores.begin(), closest_score );

					// if this is the first time we receive the config, create new main substate
					if( first_config )
					{
						mSubState = boost::make_shared<LobbyMainSubstate>(mClient, mPreferedSpeed, mPreferedRules, mPreferedScore);
					}

				} else if((LobbyPacketType)t == LobbyPacketType::GAME_STATUS)
				{
					mSubState = boost::make_shared<LobbyGameSubstate>(mClient, in);
				} else if((LobbyPacketType)t == LobbyPacketType::REMOVED_FROM_GAME)
				{
					mSubState = boost::make_shared<LobbyMainSubstate>(mClient, mPreferedSpeed, mPreferedRules, mPreferedScore);
				}
				}
				break;
			case ID_RULES_CHECKSUM: // this packet is send when a game was created, so we probably are joining a game here.
				// this is only a valid request if we are in the lobby game substate
				assert(dynamic_cast<LobbyGameSubstate*>(mSubState.get()) != nullptr);
				{
				RakNet::BitStream stream((char*)packet->data, packet->length, false);

				stream.IgnoreBytes(1);	// ignore ID_RULES_CHECKSUM

				int serverChecksum, scoreToWin;
				stream.Read(serverChecksum);
				stream.Read(scoreToWin);

				switchState( new NetworkGameState( mClient, serverChecksum, scoreToWin ) );
				}
				break;
			default:
				std::cout << "Unknown packet " << int(packet->data[0]) << " received\n";
		}
	}


	IMGUI& imgui = IMGUI::getSingleton();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doInactiveMode(false);

	// server name
	imgui.doText(GEN_ID, Vector2(400 - 12 * std::strlen(mInfo.name), 20), mInfo.name);

	// server description
	if (mLobbyState != ConnectionState::CONNECTED )
	{
		// server description
		if (mLobbyState == ConnectionState::CONNECTING )
		{
			imgui.doText(GEN_ID, Vector2( 100, 55 ), TextManager::NET_CONNECTING);
		}
		else if (mLobbyState == ConnectionState::DISCONNECTED )
		{
			imgui.doText(GEN_ID, Vector2( 100, 55 ), TextManager::NET_DISCONNECT);
		}
		else if (mLobbyState == ConnectionState::CONNECTION_FAILED )
		{
			imgui.doText(GEN_ID, Vector2( 100, 55 ), TextManager::NET_CON_FAILED);
		}

		// empty player list
		unsigned int s;
		imgui.doSelectbox(GEN_ID, Vector2(25.0, 90.0), Vector2(375.0, 470.0), std::vector<std::string>{}, s);

		// empty info panel
		imgui.doOverlay(GEN_ID, Vector2(425.0, 90.0), Vector2(775.0, 470.0));
	}
	 else
	{
		std::string description = mInfo.description;
		for (unsigned int i = 0; i < description.length(); i += 63)
		{
			imgui.doText(GEN_ID, Vector2(25, 55 + i / 63 * 15), description.substr(i, 63), TF_SMALL_FONT);
		}


		mSubState->step( mStatus );
	}



	// back button
	if (imgui.doButton(GEN_ID, Vector2(50, 530), TextManager::LBL_CANCEL))
	{
		if( mPrevious == PreviousState::ONLINE )
			switchState( new OnlineSearchState );
		else if ( mPrevious == PreviousState::LAN )
			switchState( new LANSearchState );
		else
			switchState( new MainMenuState );
	}
}

const char* LobbyState::getStateName() const
{
	return "LobbyState";
}

// ----------------------------------------------------------------------------
// 				M a i n     S u b s t a t e
// ----------------------------------------------------------------------------
LobbyMainSubstate::LobbyMainSubstate(boost::shared_ptr<RakClient> client,
									unsigned prefspeed, unsigned prefrules, unsigned prefscore ) :
	mClient(client),
	mChosenSpeed( prefspeed ),
	mChosenRules( prefrules ),
	mChosenScore( prefscore )
{

}

void LobbyMainSubstate::step(const ServerStatusData& status)
{
	IMGUI& imgui = IMGUI::getSingleton();

	// player list
	std::vector<std::string> gamelist;
	gamelist.push_back( TextManager::getSingleton()->getString(TextManager::NET_OPEN_GAME) );
	for ( const auto& game : status.mOpenGames)
	{
		gamelist.push_back( game.name );
	}

	bool doEnterGame = imgui.doSelectbox(GEN_ID, Vector2(25.0, 90.0), Vector2(375.0, 470.0), gamelist, mSelectedGame) == SBA_DBL_CLICK;

	// if selected game is invalid (i.e. a game was removed and the reference now is wrong), set to random
	if(mSelectedGame >= gamelist.size())
		mSelectedGame = 0;

	if(mSelectedGame != 0)
	{
		unsigned gameIndex = mSelectedGame - 1;

		// info panel
		imgui.doOverlay(GEN_ID, Vector2(425.0, 90.0), Vector2(775.0, 470.0));

		// info panel contents:
		//  * gamespeed
		imgui.doText(GEN_ID, Vector2(435, 100), TextManager::getSingleton()->getString(TextManager::NET_SPEED) +
					 boost::lexical_cast<std::string>(int(0.5 + 100.0 / 75.0 * status.mPossibleSpeeds.at(status.getGame(gameIndex).speed))) + "%");
		//  * points
		imgui.doText(GEN_ID, Vector2(435, 135), TextManager::getSingleton()->getString(TextManager::NET_POINTS) +
											 boost::lexical_cast<std::string>(status.getGame(gameIndex).score) );

		//  * rulesfile
		imgui.doText(GEN_ID, Vector2(435, 170), TextManager::getSingleton()->getString(TextManager::NET_RULES_TITLE) );
		std::string rulesstring = status.mPossibleRules.at(status.getGame(gameIndex).rules);
		for (unsigned int i = 0; i < rulesstring.length(); i += 25)
		{
			imgui.doText(GEN_ID, Vector2(445, 205 + i / 25 * 15), rulesstring.substr(i, 25), TF_SMALL_FONT);
		}

		// open game button
		if( imgui.doButton(GEN_ID, Vector2(435, 430), TextManager::getSingleton()->getString(TextManager::NET_JOIN) ) ||
			doEnterGame)
		{
			// send open game packet to server
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_LOBBY);
			stream.Write((unsigned char)LobbyPacketType::JOIN_GAME);
			stream.Write( status.getGame(gameIndex).id );
			/// \todo add a name

			mClient->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0);
		}
	}
	// open game
	else
	{
		// info panel
		imgui.doOverlay(GEN_ID, Vector2(425.0, 90.0), Vector2(775.0, 470.0));

		// info panel contents:
		//  * gamespeed
		if(imgui.doButton(GEN_ID, Vector2(435, 100), TextManager::getSingleton()->getString(TextManager::NET_SPEED) +
					 boost::lexical_cast<std::string>(int(0.5 + 100.0 / 75.0 * status.mPossibleSpeeds.at(mChosenSpeed))) + "%"))
		{
			mChosenSpeed = (mChosenSpeed + 1) % status.mPossibleSpeeds.size();
		}
		//  * points
		if(imgui.doButton(GEN_ID, Vector2(435, 135), TextManager::getSingleton()->getString(TextManager::NET_POINTS) +
											 boost::lexical_cast<std::string>( mPossibleScores.at(mChosenScore) ) ))
		{
			mChosenScore = (mChosenScore + 1) % mPossibleScores.size();
		}

		//  * rulesfile
		if(imgui.doButton(GEN_ID, Vector2(435, 170), TextManager::getSingleton()->getString(TextManager::NET_RULES_TITLE) ))
		{
			mChosenRules = (mChosenRules + 1) % status.mPossibleRules.size();
		}
		std::string rulesstring = status.mPossibleRules.at(mChosenRules) + TextManager::getSingleton()->getString(TextManager::NET_RULES_BY);
		rulesstring += " " + status.mPossibleRulesAuthor.at(mChosenRules);
		for (unsigned int i = 0; i < rulesstring.length(); i += 25)
		{
			imgui.doText(GEN_ID, Vector2(445, 205 + i / 25 * 15), rulesstring.substr(i, 25), TF_SMALL_FONT);
		}

		// open game button
		if( imgui.doButton(GEN_ID, Vector2(435, 430), TextManager::getSingleton()->getString(TextManager::NET_OPEN_GAME) ))
		{
			// send open game packet to server
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_LOBBY);
			stream.Write((unsigned char)LobbyPacketType::OPEN_GAME);
			stream.Write( mChosenSpeed );
			stream.Write( mPossibleScores.at(mChosenScore) );
			stream.Write( mChosenRules );
			/// \todo add a name

			mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
		}
	}

}

// -----------------------------------------------------------------------------------------
// 			"Host" State Substate
// -----------------------------------------------------------------------------------------

LobbyGameSubstate::LobbyGameSubstate(boost::shared_ptr<RakClient> client, boost::shared_ptr<GenericIn> in):
	mClient( client )
{
 	in->uint32( mGameID );
	PlayerID creator;
	in->generic<PlayerID>(creator);
	mIsHost = mClient->GetPlayerID() == creator;

	in->string(mGameName);
	in->uint32(mSpeed);
	in->uint32(mRules);
	in->uint32(mScore);
	in->generic<std::vector<PlayerID>>(mOtherPlayers);
	in->generic<std::vector<std::string>>(mOtherPlayerNames);
}

void LobbyGameSubstate::step( const ServerStatusData& status )
{
	IMGUI& imgui = IMGUI::getSingleton();
	bool no_players = mOtherPlayers.empty();
	if(mOtherPlayerNames.empty())
	{
		mOtherPlayerNames.push_back(""); // fake entry to prevent crash! not nice!
	}

	bool doEnterGame = false;
	if( imgui.doSelectbox(GEN_ID, Vector2(25.0, 90.0), Vector2(375.0, 470.0), mOtherPlayerNames, mSelectedPlayer) == SBA_DBL_CLICK )
	{
		doEnterGame = true;
	}

	// info panel
	imgui.doOverlay(GEN_ID, Vector2(425.0, 90.0), Vector2(775.0, 470.0));

	// info panel contents:
	//  * gamespeed
	imgui.doText(GEN_ID, Vector2(435, 100), TextManager::getSingleton()->getString(TextManager::NET_SPEED) +
				 boost::lexical_cast<std::string>(int(0.5 + 100.0 / 75.0 * status.mPossibleSpeeds.at(mSpeed))) + "%");
	//  * points
	imgui.doText(GEN_ID, Vector2(435, 135), TextManager::getSingleton()->getString(TextManager::NET_POINTS) +
										 boost::lexical_cast<std::string>( mScore ) );
	//  * rulesfile
	imgui.doText(GEN_ID, Vector2(435, 170), TextManager::getSingleton()->getString(TextManager::NET_RULES_TITLE) );
	std::string rulesstring = status.mPossibleRules.at(mRules) + TextManager::getSingleton()->getString(TextManager::NET_RULES_BY);
	rulesstring += " " + status.mPossibleRulesAuthor.at(mRules);
	for (unsigned int i = 0; i < rulesstring.length(); i += 25)
	{
		imgui.doText(GEN_ID, Vector2(445, 205 + i / 25 * 15), rulesstring.substr(i, 25), TF_SMALL_FONT);
	}

	// open game button
	if( mIsHost )
	{
		if( imgui.doButton(GEN_ID, Vector2(435, 395), TextManager::getSingleton()->getString(TextManager::NET_LEAVE) ) )
		{
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_LOBBY);
			stream.Write((unsigned char)LobbyPacketType::LEAVE_GAME);
			mClient->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0);
		}

		if( !no_players && (imgui.doButton(GEN_ID, Vector2(435, 430), TextManager::getSingleton()->getString(TextManager::MNU_LABEL_START) ) || doEnterGame) )
		{
			// Start Game
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_LOBBY);
			stream.Write((unsigned char)LobbyPacketType::START_GAME);
			auto writer = createGenericWriter(&stream);
			writer->generic<PlayerID>( mOtherPlayers.at(mSelectedPlayer) );
			mClient->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0);
		}

		if( no_players )
		{
			imgui.doText(GEN_ID, Vector2(435, 430), TextManager::getSingleton()->getString(TextManager::GAME_WAITING));
		}
	} else
	{
		if( imgui.doButton(GEN_ID, Vector2(435, 430), TextManager::getSingleton()->getString(TextManager::NET_LEAVE) ) )
		{
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_LOBBY);
			stream.Write((unsigned char)LobbyPacketType::LEAVE_GAME);
			mClient->Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0);
		}
	}
}

