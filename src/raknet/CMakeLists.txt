# We use the two-levels of directories so that from outside, the includes will be "raknet/..."
set(CMAKE_CXX_STANDARD 11)

set (raknet_SRC
        ArrayList.h
        BitStream.cpp BitStream.h
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
        RakPeer.cpp RakPeer.h
        RakServer.cpp RakServer.h
        ReliabilityLayer.cpp ReliabilityLayer.h
        SimpleMutex.cpp SimpleMutex.h
        SingleProducerConsumer.h
        SocketLayer.cpp SocketLayer.h
        )

add_library(raknet STATIC ${raknet_SRC})
target_include_directories(raknet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(raknet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/raknet)

# raknet additional dependencies
if (WIN32)
    target_link_libraries(raknet PUBLIC ws2_32)
endif ()

add_library(raknet::raknet ALIAS raknet)




