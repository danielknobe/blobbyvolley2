#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include "SpeedController.h"
#include "DedicatedServer.h"
#include "InputSource.h"
#include "PhysicWorld.h"

int main()
{
	RakServer server;
	server.Start(2, 0, 0, 1234);
	int clients = 0;
	
	SpeedController scontroller(60, 60);
	PhysicWorld pworld;
	pworld.reset(LEFT_PLAYER);
	pworld.resetPlayer();
	
	while (1)
	{
		Packet* packet = server.Receive();
		if (packet) switch(packet->data[0])
		{
			case ID_NEW_INCOMING_CONNECTION:
				clients++;
				printf("new connection incoming\n");
				printf("%d clients connected now\n", clients);
				break;
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_CONNECTION_LOST:
				clients--;
				printf("connection close\n");
				printf("%d clients connected now\n", clients);
				break;
			case ID_TIMESTAMP:
			{
				PlayerInput input;
				RakNet::BitStream stream((char*)packet->data, packet->length, false);
				int ival;
				stream.Read(ival);
				stream.Read(ival);
				stream.Read(input.left);
				stream.Read(input.right);
				stream.Read(input.up);
				pworld.setLeftInput(input);
printf("player input received from %d with content %c %c %c\n",
	packet->playerId.port,
		input.left, input.right, input.up);
			}
			default:
				printf("unknown packet recieved\n");
		}
		server.DeallocatePacket(packet);
		pworld.step();
		RakNet::BitStream stream;
		stream.Write(ID_TIMESTAMP);
		stream.Write(RakNet::GetTime());
		pworld.getState(&stream);
		server.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0,
			UNASSIGNED_PLAYER_ID, true);
		
		scontroller.update();
	}
}
