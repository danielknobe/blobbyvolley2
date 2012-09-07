set (raknet_SRC
	ArrayList.h
	AsynchronousFileIO.cpp AsynchronousFileIO.h
	BigTypes.h
	BinarySearchTree.h
	BitStream.cpp BitStream.h
	CheckSum.cpp CheckSum.h
	ClientContextStruct.h
	DataBlockEncryptor.cpp DataBlockEncryptor.h
	EncodeClassName.cpp EncodeClassName.h
	ExtendedOverlappedPool.cpp ExtendedOverlappedPool.h
	GetTime.cpp GetTime.h
	HuffmanEncodingTree.cpp HuffmanEncodingTree.h
	HuffmanEncodingTreeFactory.cpp HuffmanEncodingTreeFactory.h
	HuffmanEncodingTreeNode.h
	InternalPacket.h
	InternalPacketPool.cpp InternalPacketPool.h
	LinkedList.h
	MessageHandlerInterface.cpp MessageHandlerInterface.h
	MTUSize.h
	NetworkIDGenerator.cpp NetworkIDGenerator.h
	NetworkTypes.cpp NetworkTypes.h
	OrderedList.cpp OrderedList.h
	PacketEnumerations.h
	PacketPool.cpp PacketPool.h
	PacketPriority.h
	QueueLinkedList.h
	RakClient.cpp RakClient.h
	RakClientInterface.h
	RakNetQueue.h
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
