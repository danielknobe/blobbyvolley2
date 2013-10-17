set (raknet_SRC
	ArrayList.h
	BigTypes.h
	BinarySearchTree.h
	BitStream.cpp BitStream.h
	CheckSum.cpp CheckSum.h
	ClientContextStruct.h
	DataBlockEncryptor.cpp DataBlockEncryptor.h
	ExtendedOverlappedPool.cpp ExtendedOverlappedPool.h
	GetTime.cpp GetTime.h
	InternalPacket.h
	InternalPacketPool.cpp InternalPacketPool.h
	LinkedList.h
	MessageHandlerInterface.cpp MessageHandlerInterface.h
	MTUSize.h
	NetworkTypes.cpp NetworkTypes.h
	OrderedList.h
	PacketEnumerations.h
	PacketPool.cpp PacketPool.h
	PacketPriority.h
	QueueLinkedList.h
	RakClient.cpp RakClient.h
	RakClientInterface.h
	RakNetStatistics.cpp RakNetStatistics.h
	RakNetworkFactory.cpp RakNetworkFactory.h
	RakPeer.cpp RakPeer.h
	RakPeerInterface.h
	RakServer.cpp RakServer.h
	RakServerInterface.h
	Rand.cpp Rand.h
	ReliabilityLayer.cpp ReliabilityLayer.h
	rijndael-boxes.h
	rijndael.cpp rijndael.h
	RSACrypt.h
	SHA1.cpp SHA1.h
	SimpleMutex.cpp SimpleMutex.h
	SingleProducerConsumer.h
	SocketLayer.cpp SocketLayer.h
	Types.h
	)

add_library(raknet STATIC ${raknet_SRC})	
