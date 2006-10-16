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

	server.Start(2, 0, 0, port);
	
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
					if (playermap[packet->playerId])
						playermap[packet->playerId]->injectPacket(packet);
					playermap.erase(packet->playerId);
					clients--;
					printf("connection close\n");
					printf("%d clients connected now\n", clients);
					break;
				case ID_INPUT_UPDATE:
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
							switchSide = newSide;
						NetworkGame* newgame = new NetworkGame(
							server, leftPlayer, rightPlayer,
							switchSide);
						playermap[leftPlayer] = newgame;
						playermap[rightPlayer] = newgame;

						gamelist.push_back(newgame);
					}
					break;
				}
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
