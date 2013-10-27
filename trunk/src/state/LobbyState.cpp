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

#include "LobbyState.h"

#include <stdexcept>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "IMGUI.h"
#include "NetworkState.h"
#include "UserConfig.h"
#include "GenericIO.h"

LobbyState::LobbyState(ServerInfo info) : mClient(new RakClient(), [](RakClient* client) { client->Disconnect(25); }), mInfo(info), mSelectedPlayer(0)
{
	if (!mClient->Connect(mInfo.hostname, mInfo.port, 0, 0, RAKNET_THREAD_SLEEP_TIME))
		throw( std::runtime_error(std::string("Could not connect to server ") + mInfo.hostname) );

	mLobbyState = CONNECTING;

	// send an ENTER_SERVER packet with name and side preference
	// resert imgui
	IMGUI::getSingleton().resetSelection();
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
	// we should properly disconnect if we do not connect to server!
	//mClient->Disconnect(50);
}

void LobbyState::step()
{
	// process packets
	packet_ptr packet;
	while (packet = mClient->Receive())
	{
		switch(packet->data[0])
		{
			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_ENTER_SERVER);

				// Send preferred side
				stream.Write( mLocalPlayer.getPreferredSide() );

				// Send playername
				char myname[16];
				strncpy(myname, mLocalPlayer.getName().c_str(), sizeof(myname));
				stream.Write(myname, sizeof(myname));

				// send color settings
				stream.Write(mLocalPlayer.getStaticColor().toInt());

				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);

				mLobbyState = CONNECTED;
				break;
			}
			case ID_CONNECTION_ATTEMPT_FAILED:
			{
				mLobbyState = CONNECTION_FAILED;
				break;
			}

			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
			{
				mLobbyState = DISCONNECTED;
				break;
			};
			case ID_SERVER_STATUS:
			{
				RakNet::BitStream stream = packet->getStream();
				auto in = createGenericReader( &stream );
				unsigned char t;
				in->byte(t);

				std::vector<std::string> names;
				std::vector<PlayerID> ids;

				PlayerID own_id = mClient->GetPlayerID(  );

				in->generic<std::vector<std::string>>( names );
				in->generic<std::vector<PlayerID>>( ids );
				unsigned int games;
				in->uint32( games );
				mInfo.activegames = games;

				// now add every player as possible opponent, except us
				mConnectedPlayers.clear();
				for(unsigned int i = 0; i < names.size(); ++i)
				{
					if(ids[i] != own_id)
					{
						mConnectedPlayers.push_back( {names[i], ids[i]} );
					}
				}
			}
			break;
			case ID_CHALLENGE:
			{
				RakNet::BitStream stream = packet->getStream();
				auto in = createGenericReader( &stream );
				unsigned char t;
				in->byte(t);

				PlayerID challenger;
				in->generic<PlayerID>( challenger );

				// find player with that id
				auto pl = std::find_if( mConnectedPlayers.begin(), mConnectedPlayers.end(), [challenger](WaitingPlayer& wp) { return wp.id == challenger; } );
				mChallenge = pl->displayname;
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
	if (mLobbyState == CONNECTING )
	{
		imgui.doText(GEN_ID, Vector2( 100, 55 ), TextManager::NET_CONNECTING);
	}
	else if (mLobbyState == DISCONNECTED )
	{
		imgui.doText(GEN_ID, Vector2( 100, 55 ), TextManager::NET_DISCONNECT);
	}
	else if (mLobbyState == CONNECTION_FAILED )
	{
		imgui.doText(GEN_ID, Vector2( 100, 55 ), TextManager::NET_CON_FAILED);
	}
	else
	{
		std::string description = mInfo.description;
		for (unsigned int i = 0; i < description.length(); i += 80)
		{
			imgui.doText(GEN_ID, Vector2(50, 55 + i / 80 * 15), description.substr(i, 80), TF_SMALL_FONT);
		}
	}

	// player list

	std::vector<std::string> playerlist;
	if( mLobbyState == CONNECTED )
	{
		playerlist.push_back( TextManager::getSingleton()->getString(TextManager::NET_RANDOM_OPPONENT) );
		for (unsigned int i = 0; i < mConnectedPlayers.size(); i++)
		{
			playerlist.push_back(mConnectedPlayers[i].displayname );
		}
	}

	bool doEnterGame = false;
	if( imgui.doSelectbox(GEN_ID, Vector2(25.0, 90.0), Vector2(375.0, 470.0),
			playerlist, mSelectedPlayer) == SBA_DBL_CLICK )
	{
		doEnterGame = true;
	}

	// info panel
	imgui.doOverlay(GEN_ID, Vector2(425.0, 90.0), Vector2(775.0, 470.0));

	// info panel contents:
	//  * gamespeed
	imgui.doText(GEN_ID, Vector2(435, 100), TextManager::getSingleton()->getString(TextManager::NET_SPEED) +
				 boost::lexical_cast<std::string>(int(100.0 / 75.0 * mInfo.gamespeed)) + "%");
	//  * number of active games
	imgui.doText(GEN_ID, Vector2(435, 135), TextManager::getSingleton()->getString(TextManager::NET_ACTIVE_GAMES) +
										boost::lexical_cast<std::string>(mInfo.activegames) );
	//  * rulesfile
	imgui.doText(GEN_ID, Vector2(435, 170), TextManager::getSingleton()->getString(TextManager::NET_RULES_TITLE) );
	std::string rulesstring = mInfo.rulestitle + TextManager::getSingleton()->getString(TextManager::NET_RULES_BY) + mInfo.rulesauthor;
	for (unsigned int i = 0; i < rulesstring.length(); i += 25)
	{
		imgui.doText(GEN_ID, Vector2(445, 205 + i / 25 * 15), rulesstring.substr(i, 25), TF_SMALL_FONT);
	}

	// * last challenge
	imgui.doText(GEN_ID, Vector2(435, 245), TextManager::getSingleton()->getString(TextManager::NET_CHALLENGE));
	imgui.doText(GEN_ID, Vector2(435, 280), " " + mChallenge);



	// back button
	if (imgui.doButton(GEN_ID, Vector2(480, 530), TextManager::LBL_CANCEL))
	{
		deleteCurrentState();
		setCurrentState(new MainMenuState);
	}

	// ok button
	if (mLobbyState == CONNECTED && (imgui.doButton(GEN_ID, Vector2(230, 530), TextManager::LBL_OK) || doEnterGame))
	{
		RakNet::BitStream stream;
		stream.Write((char)ID_CHALLENGE);
		auto writer = createGenericWriter(&stream);
		if( mSelectedPlayer != 0 )
		{
			writer->generic<PlayerID>( mConnectedPlayers[mSelectedPlayer-1].id );
		}
		 else
		{
			writer->generic<PlayerID>( UNASSIGNED_PLAYER_ID );
		}

		mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);

		// new state must be created here, so client won't get deleted
		auto newState = new NetworkGameState(mClient);
		deleteCurrentState();
		setCurrentState( newState );
	}
}

const char* LobbyState::getStateName() const
{
	return "LobbyState";
}
