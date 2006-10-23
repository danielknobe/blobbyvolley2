#include <physfs.h>

#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include "SpeedController.h"
#include "DedicatedServer.h"
#include "InputSource.h"
#include "PhysicWorld.h"
#include "NetworkGame.h"
#include "UserConfig.h"
#include "NetworkMessage.h"

#ifdef WIN32
#undef main
#endif

int main(int argc, char** argv)
{
	PHYSFS_init(argv[0]);
	PHYSFS_addToSearchPath("data", 1);

	GameList gamelist;
	PlayerMap playermap;
	RakServer server;
	UserConfig config;

	PlayerID firstPlayer;
	PlayerSide firstPlayerSide = NO_PLAYER;

	config.loadFile("server.xml");
	int port = config.getInteger("port");
	float speed = config.getFloat("speed");
	int clients = 0;

	ServerInfo myinfo(config);
	
	if (!server.Start(200, 0, 0, port))
	{
		std::cerr << "blobby-server: Couldn't bind to port " << port;
		std::cerr << " !" << std::endl;
	}

	SpeedController scontroller(speed);

	while (1)
	{
		Packet* packet;
		while (packet = server.Receive())
		{
			switch(packet->data[0])
			{
				case ID_NEW_INCOMING_CONNECTION:
					clients++;
					printf("new connection incoming\n");
					printf("%d clients connected now\n", clients);
					break;
				case ID_DISCONNECTION_NOTIFICATION:
				case ID_CONNECTION_LOST:
				{
					bool cond1 = firstPlayerSide != NO_PLAYER;
					bool cond2 = firstPlayer == packet->playerId;
					if (cond1 && cond2)
						firstPlayerSide = NO_PLAYER;
					if (playermap[packet->playerId])
						playermap[packet->playerId]->injectPacket(packet);
					playermap.erase(packet->playerId);
					clients--;
					printf("connection close\n");
					printf("%d clients connected now\n", clients);
					break;
				}
				case ID_INPUT_UPDATE:
				case ID_PAUSE:
				case ID_UNPAUSE:
					if (playermap[packet->playerId])
						playermap[packet->playerId]->injectPacket(packet);
					break;
				case ID_ENTER_GAME:
				{
					int ival;
					RakNet::BitStream stream((char*)packet->data, 
							packet->length, false);
					stream.Read(ival);
					stream.Read(ival);
					PlayerSide newSide = (PlayerSide)ival;

					if (firstPlayerSide == NO_PLAYER)
					{
						firstPlayer = packet->playerId;
						firstPlayerSide = newSide;
					}
					else // We have two players now
					{
						PlayerID leftPlayer = 
							LEFT_PLAYER == firstPlayerSide ?
							firstPlayer : packet->playerId;
						PlayerID rightPlayer =
							RIGHT_PLAYER == firstPlayerSide ?
							firstPlayer : packet->playerId;
						PlayerSide switchSide = NO_PLAYER;
						if (newSide == firstPlayerSide)
						{
							if (newSide == LEFT_PLAYER)
								switchSide = RIGHT_PLAYER;
							if (newSide == RIGHT_PLAYER)
								switchSide = LEFT_PLAYER;
						}
						NetworkGame* newgame = new NetworkGame(
							server, leftPlayer, rightPlayer,
							switchSide);
						playermap[leftPlayer] = newgame;
						playermap[rightPlayer] = newgame;
						gamelist.push_back(newgame);
						
						firstPlayerSide = NO_PLAYER;
					}
					break;
				}
				case ID_PONG:
					break;
				case ID_BLOBBY_SERVER_PRESENT:
				{
					myinfo.activegames = gamelist.size();
					if (firstPlayerSide == NO_PLAYER)
					{
						strncpy(myinfo.waitingplayer, "none",
							sizeof(myinfo.waitingplayer) - 1);
					}
					else
					{
						// TODO: Insert waiting players name here
						strncpy(myinfo.waitingplayer, "somebody",
							sizeof(myinfo.waitingplayer) - 1);
					}

					RakNet::BitStream stream;
					stream.Write(ID_BLOBBY_SERVER_PRESENT);
					myinfo.writeToBitstream(stream);
					server.Send(&stream, HIGH_PRIORITY,
						RELIABLE_ORDERED, 0,
						packet->playerId, false);
					break;
				}
				case ID_RECEIVED_STATIC_DATA:
					break;
				default:
					printf("unknown packet %d recieved\n",
						int(packet->data[0]));
				}
			}
		for (GameList::iterator iter = gamelist.begin(); gamelist.end() != iter; ++iter)
		{
			(*iter)->step();
		}
		server.DeallocatePacket(packet);
		scontroller.update();
	}
}
