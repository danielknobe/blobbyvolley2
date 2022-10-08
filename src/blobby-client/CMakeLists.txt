set(INPUT_SRC
        input/JoystickInput.cpp
        input/JoystickPool.cpp
        input/KeyboardInput.cpp
        input/MouseInput.cpp
        input/TouchInput.cpp
        input/InputManager.cpp
        input/InputSourceFactory.cpp
        input/LocalInputSource.cpp
        input/ScriptedInputSource.cpp
        )

set(STATE_SRC
        state/State.cpp
        state/GameState.cpp
        state/LocalGameState.cpp
        state/NetworkState.cpp
        state/OptionsState.cpp
        state/NetworkSearchState.cpp
        state/ReplayState.cpp
        state/ReplaySelectionState.cpp
        state/LobbyStates.cpp
        )

set(RENDER_SRC
        render/Blood.cpp
        render/RenderManager.cpp
        render/RenderManagerGL2D.cpp
        render/RenderManagerSDL.cpp
        render/RenderManagerNull.cpp
        )

set(OTHER_SRC
        IMGUI.cpp
        SoundManager.cpp
        TextManager.cpp
        )

find_package(OpenGL)
if (OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
    add_definitions(-DHAVE_LIBGL)
endif (OPENGL_FOUND)

add_library(blobby-client ${INPUT_SRC} ${STATE_SRC} ${RENDER_SRC} ${OTHER_SRC})
target_include_directories(blobby-client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(blobby-client PUBLIC blobby-core SDL2::SDL2)
target_link_libraries(blobby-client PRIVATE  blobnet::blobnet lua::lua ${OPENGL_LIBRARIES} tinyxml2::tinyxml2)
target_compile_options(blobby-client PRIVATE -Werror=undef)

if (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(blobby-client PUBLIC Threads::Threads)
endif()


MakeHeaderTests(test-headers-client "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test-headers-client PRIVATE blobby-client)
