set(NETWORK_SRC
        src/network/DedicatedServer.cpp
        src/network/MatchMaker.cpp
        src/network/NetworkGame.cpp
        src/network/NetworkPlayer.cpp
        src/network/NetworkMessage.cpp
        )

set(IO_SRC
        src/io/File.cpp
        src/io/FileRead.cpp
        src/io/FileSystem.cpp
        src/io/FileWrite.cpp
        src/io/GenericIO.cpp
        )

set(REPLAY_SRC
        src/replays/base64.cpp
        src/replays/ReplayLoader.cpp
        src/replays/ReplayPlayer.cpp
        src/replays/ReplayRecorder.cpp
        src/replays/ReplaySavePoint.cpp
        )

set(MATCH_SRC
        src/match/Clock.cpp
        src/match/DuelMatch.cpp
        src/match/DuelMatchState.cpp
        src/match/GameLogic.cpp
        src/match/GameLogicState.cpp
        src/match/InputSource.cpp
        src/match/PhysicState.cpp
        src/match/PhysicWorld.cpp
        src/match/PlayerInput.cpp
        )

set(OTHER_SRC
        src/BlobbyDebug.cpp
        src/IScriptableComponent.cpp
        src/PlayerIdentity.cpp
        src/SpeedController.cpp
        src/UserConfig.cpp
        src/Color.cpp
        )


add_library(blobby-core
        ${OTHER_SRC}
        ${NETWORK_SRC}
        ${IO_SRC}
        ${REPLAY_SRC}
        ${MATCH_SRC}
        )

# raknet is a public dependency, because NetworkPlayer.h includes its headers
target_link_libraries(blobby-core PUBLIC raknet::raknet ${SDL2_LIBRARIES})
target_link_libraries(blobby-core PRIVATE tinyxml2::tinyxml2 PhysFS::PhysFS lua::lua)
target_include_directories(blobby-core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(blobby-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(blobby-core PRIVATE -Werror=undef)

MakeHeaderTests(test-headers-core "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(test-headers-core PRIVATE blobby-client)