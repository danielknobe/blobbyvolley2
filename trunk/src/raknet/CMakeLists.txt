set (raknet_SRC
	ArrayList.h
	BitStream.cpp BitStream.h
	ClientContextStruct.h
	GetTime.cpp GetTime.h
	InternalPacket.h
	InternalPacketPool.cpp InternalPacketPool.h
	LinkedList.h
	MTUSize.h
	NetworkTypes.cpp NetworkTypes.h
	PacketEnumerations.h
	PacketPool.cpp PacketPool.h
	PacketPriority.h
	RakClient.cpp RakClient.h
	RakNetStatistics.cpp RakNetStatistics.h
	RakNetworkFactory.cpp RakNetworkFactory.h
	RakPeer.cpp RakPeer.h
	RakServer.cpp RakServer.h
	ReliabilityLayer.cpp ReliabilityLayer.h
	SimpleMutex.cpp SimpleMutex.h
	SingleProducerConsumer.h
	SocketLayer.cpp SocketLayer.h
	)

add_library(raknet STATIC ${raknet_SRC})
