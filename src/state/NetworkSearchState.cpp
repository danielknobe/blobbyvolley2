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
#include "NetworkSearchState.h"

/* includes */
#include <string>

#include <boost/lexical_cast.hpp>

#include "raknet/RakClient.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/RakServer.h"

#include "NetworkState.h"
#include "TextManager.h"
#include "IMGUI.h"
#include "RakNetPacket.h"
#include "IUserConfigReader.h"


/* implementation */
NetworkSearchState::NetworkSearchState() :
		mPingClient(new RakClient)
{
	IMGUI::getSingleton().resetSelection();
	mSelectedServer = 0;
	mServerBoxPosition = 0;
	mDisplayInfo = false;
	mEnteringServer = false;
	mDisplayUpdateNotification = false;
}

NetworkSearchState::~NetworkSearchState()
{
	// Disconnect from servers
	for (ClientList::iterator iter = mQueryClients.begin();
		iter != mQueryClients.end(); ++iter)
	{
		if (*iter)
		{
			(*iter)->Disconnect(50);
			delete *iter;
		}
	}
}

void NetworkSearchState::step()
{	
	packet_ptr packet;
	
	for (ClientList::iterator iter = mQueryClients.begin();
		iter != mQueryClients.end(); ++iter)
	{
		bool skip = false;
		bool skip_iter = false;
		if (!skip)
		while ((packet = receivePacket(*iter)) && !skip)
		{
			switch(packet->data[0])
			{
				case ID_CONNECTION_REQUEST_ACCEPTED:
				{
					printf("connection accepted from %s:%d\n",
						mPingClient->PlayerIDToDottedIP(
							packet->playerId), packet->playerId.port);

					RakNet::BitStream stream;
					stream.Write((unsigned char)ID_BLOBBY_SERVER_PRESENT);
					stream.Write(BLOBBY_VERSION_MAJOR);
					stream.Write(BLOBBY_VERSION_MINOR);
					(*iter)->Send(&stream, LOW_PRIORITY,
						RELIABLE_ORDERED, 0);
					break;
				}
				case ID_BLOBBY_SERVER_PRESENT:
				{
					//FIXME: We must copy the needed informations, so that we can call DeallocatePacket(packet)
					//FIXME: The client finds a server at this point, which is not valid
					RakNet::BitStream stream((char*)packet->data,
						packet->length, false);
					printf("server is a blobby server\n");
					stream.IgnoreBytes(1);	//ID_BLOBBY_SERVER_PRESENT
					ServerInfo info(stream,
						(*iter)->PlayerIDToDottedIP(
							packet->playerId), packet->playerId.port);

					if (std::find(
							mScannedServers.begin(),
							mScannedServers.end(),
							info) == mScannedServers.end() 
							// check whether the packet sizes match
							&& packet->length == ServerInfo::BLOBBY_SERVER_PRESENT_PACKET_SIZE ){
						mScannedServers.push_back(info);
					}
					// the RakClient will be deleted, so
					// we must free the packet here
					packet.reset();
					(*iter)->Disconnect(50);
					delete *iter;
					iter = mQueryClients.erase(iter);
					if (iter == mQueryClients.end())
						skip_iter = true;
					skip = true;
					break;
				}
				case ID_VERSION_MISMATCH:
				{
					// this packet is send when the client is older than the server!
					// so 
					RakNet::BitStream stream((char*)packet->data,
						packet->length, false);
					stream.IgnoreBytes(1);	// ID_VERSION_MISMATCH
					
					// default values if server does not send versions.
					// thats the 0.9 behaviour
					int smajor = 0, sminor = 9;		
					stream.Read(smajor);	// load server version information
					stream.Read(sminor);
					printf("found blobby server with version %d.%d\n", smajor, sminor);
					
					mDisplayUpdateNotification = true;
					
					// the RakClient will be deleted, so
					// we must free the packet here
					packet.reset();
					(*iter)->Disconnect(50);
					delete *iter;
					iter = mQueryClients.erase(iter);
					if (iter == mQueryClients.end())
						skip_iter = true;
					skip = true;
					break;
				}
				default:
					break;
			}

			if (skip)
				break;
		}
		if (skip_iter)
			break;
	}
	while (packet = receivePacket(mPingClient.get()))
	{
		switch (packet->data[0])
		{
			case ID_PONG:
			{
				std::string hostname = mPingClient->PlayerIDToDottedIP(packet->playerId);
				printf("got ping response by \"%s:%d\", trying to connect\n", hostname.c_str(), packet->playerId.port);
				RakClient* newClient = new RakClient;
				newClient->Connect(
					hostname.c_str(), packet->playerId.port, 0, 0, RAKNET_THREAD_SLEEP_TIME);
				mQueryClients.push_back(newClient);
			}
			default:
				break;
		}
	}

	IMGUI& imgui = IMGUI::getSingleton();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doInactiveMode(false);

	if (mDisplayInfo || mEnteringServer)
	{
		imgui.doInactiveMode(true);
	}

	if (imgui.doButton(GEN_ID, Vector2(10, 20), TextManager::NET_SERVER_SCAN))
		searchServers();

	if (imgui.doButton(GEN_ID, Vector2(420, 20), TextManager::NET_DIRECT_CONNECT) &&
			!mEnteringServer)
	{
		mEnteringServer = true;
		imgui.resetSelection();
		mEnteredServer = "";
		mServerBoxPosition = 0;
	}

	std::vector<std::string> servernames;
	for (int i = 0; i < mScannedServers.size(); i++)
	{
		servernames.push_back(mScannedServers[i].name);
	}

	bool doEnterServer = false;
	if( imgui.doSelectbox(GEN_ID, Vector2(25.0, 60.0), Vector2(775.0, 470.0),
			servernames, mSelectedServer) == SBA_DBL_CLICK )
	{
		doEnterServer = true;
	}

	if (imgui.doButton(GEN_ID, Vector2(50, 480), TextManager::NET_SERVER_INFO) &&
			!mDisplayInfo && !mScannedServers.empty())
	{
		mDisplayInfo = true;
		imgui.resetSelection();
	}

	if (mEnteringServer)
	{
		imgui.doInactiveMode(false);
		imgui.doOverlay(GEN_ID, Vector2(100.0, 200.0), Vector2(650.0, 400.0));
		// Game crashes if the mEnteredServer is not a possible input
		imgui.doEditbox(GEN_ID, Vector2(130.0, 210.0), 20, mEnteredServer, mServerBoxPosition);
		if (imgui.doButton(GEN_ID, Vector2(270.0, 300.0), TextManager::LBL_OK))
		{
			//std::string server = mScannedServers[mSelectedServer].hostname;
			std::string server = mEnteredServer;
			int port = BLOBBY_PORT;
			std::size_t found = mEnteredServer.find(':');
			if (found != std::string::npos) {
				server = mEnteredServer.substr(0, found);
				
				try
				{
					port = boost::lexical_cast<int>(mEnteredServer.substr(found+1));
				}
				catch (boost::bad_lexical_cast)
				{
					/// \todo inform the user that default port was selected
					
				}
				if ((port <= 0) || (port > 65535))
					port = BLOBBY_PORT;
			}

			deleteCurrentState();
			setCurrentState(new NetworkGameState(server, port));
			return;
		}
		if (imgui.doButton(GEN_ID, Vector2(370.0, 300.0), TextManager::LBL_CANCEL))
		{
			mEnteringServer = false;
			imgui.resetSelection();
		}
		imgui.doInactiveMode(true);
	}

	if (mDisplayInfo)
	{
		imgui.doInactiveMode(false);
		imgui.doOverlay(GEN_ID, Vector2(40.0, 80.0), Vector2(760.0, 440.0));
		imgui.doText(GEN_ID, Vector2(50, 100), mScannedServers[mSelectedServer].name);
		imgui.doText(GEN_ID, Vector2(50, 130), mScannedServers[mSelectedServer].hostname);

		std::stringstream activegames;
		activegames << TextManager::getSingleton()->getString(TextManager::NET_ACTIVE_GAMES)
					<< mScannedServers[mSelectedServer].activegames;
		imgui.doText(GEN_ID, Vector2(50, 160), activegames.str());
		std::stringstream waitingplayer;
		waitingplayer << TextManager::getSingleton()->getString(TextManager::NET_WAITING_PLAYER)
					  << mScannedServers[mSelectedServer].waitingplayer;
		imgui.doText(GEN_ID, Vector2(50, 190), waitingplayer.str());
		std::stringstream gamespeed;
		gamespeed << TextManager::getSingleton()->getString(TextManager::OP_SPEED)<<" "
					  << int(100.0 / 75.0 * mScannedServers[mSelectedServer].gamespeed)<<"%";
		imgui.doText(GEN_ID, Vector2(50, 220), gamespeed.str());
		std::string description = mScannedServers[mSelectedServer].description;
		for (int i = 0; i < description.length(); i += 29)
		{
			imgui.doText(GEN_ID, Vector2(50, 250 + i / 29 * 30),
					description.substr(i, 29));
		}

		if (imgui.doButton(GEN_ID, Vector2(410, 405), TextManager::LBL_OK))
		{
			mDisplayInfo = false;
			imgui.resetSelection();
		}
		imgui.doInactiveMode(true);
	}

	if (imgui.doButton(GEN_ID, Vector2(450, 480), TextManager::NET_HOST_GAME) &&
			!mDisplayInfo)
	{
		deleteCurrentState();
		setCurrentState(new NetworkHostState());
		return;
	}

	if (imgui.doButton(GEN_ID, Vector2(230, 530), TextManager::LBL_OK) 
							&& !mScannedServers.empty() || doEnterServer)
	{
		ServerInfo server = mScannedServers[mSelectedServer];
		deleteCurrentState();
		setCurrentState(new NetworkGameState(server.hostname, server.port));
	}
	if (imgui.doButton(GEN_ID, Vector2(480, 530), TextManager::LBL_CANCEL))
	{
		deleteCurrentState();
		setCurrentState(new MainMenuState);
	}
	
	if(mDisplayUpdateNotification)
	{
		imgui.doOverlay(GEN_ID, Vector2(71, 572), Vector2(729, 590), Color(128, 0, 0));
		imgui.doText(GEN_ID, Vector2(85, 577), TextManager::UPDATE_NOTIFICATION, TF_SMALL_FONT);
	}
}

const char* NetworkSearchState::getStateName() const
{
	return "NetworkSearchState";
}

// the different networkmodi classes (online/LAN)
OnlineSearchState::OnlineSearchState()
{
	searchServers();
}


void OnlineSearchState::searchServers()
{
	/// \todo does anyone know how exaclty mPingClient works?
	mScannedServers.clear();
	//TODO: Insert Masterserverconnection code here! At the moment we are using the old code here!
	mPingClient->PingServer("blobby.blub-game.com", BLOBBY_PORT, 0, true);
	/// \todo thats a hack to make us use our speed server. add a better 
	///			method to connect to servers with arbitrary Ports
	mPingClient->PingServer("blobby.blub-game.com", BLOBBY_PORT, 0, true);
	mPingClient->PingServer("pgb.game-host.org", BLOBBY_PORT+1, 0, true);
	
	/// \todo check if we already try to connect to this one!
	mPingClient->PingServer(
		IUserConfigReader::createUserConfigReader("config.xml")->getString("network_last_server").c_str(),
		BLOBBY_PORT, 0, true);
	
}

const char* OnlineSearchState::getStateName() const
{
	return "OnlineSearchState";
}

LANSearchState::LANSearchState()
{
	searchServers();
}

void LANSearchState::searchServers()
{
	mScannedServers.clear();
	mPingClient->PingServer("255.255.255.255", BLOBBY_PORT, 0, true);
}

const char* LANSearchState::getStateName() const
{
	return "LANSearchState";
}
