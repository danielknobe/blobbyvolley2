# We use the two-levels of directories so that from outside, the includes will be "raknet/..."
set(CMAKE_CXX_STANDARD 11)

set (raknet_SRC
        src/BitStream.cpp
        src/GetTime.cpp
        src/InternalPacketPool.cpp
        src/NetworkTypes.cpp
        src/PacketPool.cpp
        src/RakClient.cpp
        src/RakNetStatistics.cpp
        src/RakPeer.cpp
        src/RakServer.cpp
        src/ReliabilityLayer.cpp
        src/SimpleMutex.cpp
        src/SocketLayer.cpp
        )

add_library(raknet STATIC ${raknet_SRC})
target_include_directories(raknet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(raknet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/raknet)

# raknet additional dependencies
if (WIN32)
    target_link_libraries(raknet PUBLIC ws2_32)
endif ()

add_library(raknet::raknet ALIAS raknet)




