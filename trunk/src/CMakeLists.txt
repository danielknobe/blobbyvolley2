add_subdirectory(lua)
add_subdirectory(tinyxml)
ADD_DEFINITIONS(-std=c++11)
add_subdirectory(raknet)
add_subdirectory(blobnet)

add_definitions(-DTIXML_USE_STL)
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS "-Wall")
include_directories(.)

set(common_SRC
	base64.cpp base64.h
	BlobbyDebug.cpp BlobbyDebug.h
	Clock.cpp Clock.h
	DuelMatch.cpp DuelMatch.h
	FileRead.cpp FileRead.h
	FileSystem.cpp FileSystem.h
	FileWrite.cpp FileWrite.h
	File.cpp File.h
	GameLogic.cpp GameLogic.h
	GenericIO.cpp GenericIO.h
	Global.h
	NetworkMessage.cpp NetworkMessage.h
	PhysicWorld.cpp PhysicWorld.h
	SpeedController.cpp SpeedController.h
	UserConfig.cpp UserConfig.h
	PhysicState.cpp PhysicState.h
	DuelMatchState.cpp DuelMatchState.h
	GameLogicState.cpp GameLogicState.h
	InputSource.cpp InputSource.h
	PlayerInput.h PlayerInput.cpp
	IScriptableComponent.cpp IScriptableComponent.h
	PlayerIdentity.cpp PlayerIdentity.h
	server/DedicatedServer.cpp server/DedicatedServer.h
	server/NetworkPlayer.cpp server/NetworkPlayer.h
	server/NetworkGame.cpp server/NetworkGame.h
	server/MatchMaker.cpp server/MatchMaker.h
	replays/ReplayRecorder.cpp replays/ReplayRecorder.h
	replays/ReplaySavePoint.cpp replays/ReplaySavePoint.h
	)

set (blobby_SRC ${common_SRC} ${inputdevice_SRC}
	Blood.cpp Blood.h
	TextManager.cpp TextManager.h
	main.cpp
	IMGUI.cpp IMGUI.h
	InputDevice.h
	InputManager.cpp InputManager.h
	LocalInputSource.cpp LocalInputSource.h
	RenderManager.cpp RenderManager.h
	RenderManagerGL2D.cpp RenderManagerGL2D.h
#	RenderManagerGP2X.cpp RenderManagerGP2X.h
	RenderManagerSDL.cpp RenderManagerSDL.h
	ScriptedInputSource.cpp ScriptedInputSource.h
	SoundManager.cpp SoundManager.h
	Vector.h
	replays/ReplayPlayer.cpp replays/ReplayPlayer.h
	replays/ReplayLoader.cpp
	InputSourceFactory.cpp InputSourceFactory.h
	state/State.cpp state/State.h
	state/GameState.cpp state/GameState.h
	state/LocalGameState.cpp state/LocalGameState.h
	state/NetworkState.cpp state/NetworkState.h
	state/OptionsState.cpp state/OptionsState.h
	state/NetworkSearchState.cpp state/NetworkSearchState.h
	state/ReplayState.cpp state/ReplayState.h
	state/ReplaySelectionState.cpp state/ReplaySelectionState.h
	state/LobbyStates.cpp state/LobbyStates.h
	input_device/JoystickInput.cpp
	input_device/JoystickPool.cpp input_device/JoystickPool.h
	input_device/KeyboardInput.cpp
	input_device/MouseInput.cpp
	input_device/TouchInput.cpp
	)

set (blobby-server_SRC ${common_SRC}
	server/servermain.cpp
	)

find_package(Boost REQUIRED)
find_package(PhysFS REQUIRED)
find_package(OpenGL)
find_package(Threads REQUIRED)

INCLUDE(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)

include_directories(
	${Boost_INCLUDE_DIR} ${PHYSFS_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS}
)
if (OPENGL_FOUND)
	add_definitions(-DHAVE_LIBGL)
endif (OPENGL_FOUND)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
	set(RAKNET_LIBRARIES raknet ws2_32)
else (CMAKE_SYSTEM_NAME STREQUAL Windows)
	set(RAKNET_LIBRARIES raknet)
endif (CMAKE_SYSTEM_NAME STREQUAL Windows)

add_executable(blobby ${blobby_SRC})
target_link_libraries(blobby lua raknet blobnet tinyxml ${RAKNET_LIBRARIES} ${PHYSFS_LIBRARY} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} pthread)

if (UNIX)
	add_executable(blobby-server ${blobby-server_SRC})
	target_link_libraries(blobby-server lua raknet blobnet tinyxml ${RAKNET_LIBRARIES} ${PHYSFS_LIBRARY} ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif (UNIX)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
	set_target_properties(blobby PROPERTIES LINK_FLAGS "-mwindows") # disable the console window
endif (CMAKE_SYSTEM_NAME STREQUAL Windows)

if (WIN32)
	install(TARGETS blobby DESTINATION .)
elseif (UNIX)
	install(TARGETS blobby blobby-server DESTINATION bin)
endif (WIN32)
