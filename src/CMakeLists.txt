add_subdirectory(lua)
add_subdirectory(tinyxml)
add_subdirectory(raknet)
add_subdirectory(blobnet)

set(APP_NAME "Blobby Volley 2")
set(APP_AUTHOR "")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
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
	IMGUI.cpp IMGUI.h
	InputDevice.h
	InputManager.cpp InputManager.h
	LocalInputSource.cpp LocalInputSource.h
	RenderManager.cpp RenderManager.h
	RenderManagerGL2D.cpp RenderManagerGL2D.h
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
find_package(SDL2 REQUIRED)

if (SWITCH)
	find_library(NX nx)
endif (SWITCH)

if ("${SDL2_LIBRARIES}" STREQUAL "")
	set(SDL2_LIBRARIES "SDL2::SDL2")
endif ("${SDL2_LIBRARIES}" STREQUAL "")

if (NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
	find_package(Threads REQUIRED)
endif (NOT CMAKE_SYSTEM_NAME STREQUAL Windows)

#INCLUDE(FindPkgConfig)
#PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)

include_directories(
	${Boost_INCLUDE_DIR} ${PHYSFS_INCLUDE_DIR}
	${SDL2_INCLUDE_DIRS}
)

if (OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	add_definitions(-DHAVE_LIBGL)
endif (OPENGL_FOUND)

# raknet additional dependencies
set(RAKNET_LIBRARIES raknet)
if (CMAKE_SYSTEM_NAME STREQUAL Windows)
	set(RAKNET_LIBRARIES ${RAKNET_LIBRARIES} ws2_32)
endif (CMAKE_SYSTEM_NAME STREQUAL Windows)

# sdl additional dependencies
if (CMAKE_SYSTEM_NAME STREQUAL iOS)
	set(SDL2_LIBRARIES ${SDL2_LIBRARIES}
		"-framework AudioToolbox"
		"-framework AVFoundation"
		"-framework CoreAudio"
		"-framework CoreAudioKit"
		"-framework CoreGraphics"
		"-framework CoreMotion"
		"-framework Foundation"
		"-framework GameController"
		"-framework MediaPlayer"
		"-framework OpenGLES"
		"-framework QuartzCore"
		"-framework UIKit")
endif (CMAKE_SYSTEM_NAME STREQUAL iOS)

# other additional dependencies
set(ADDITIONAL_LIBRARIES)
if (NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
	set(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} pthread)
endif (NOT CMAKE_SYSTEM_NAME STREQUAL Windows)

if (SWITCH)
	add_executable(blobby.elf main.cpp ${blobby_SRC})
	target_link_libraries(blobby.elf lua raknet blobnet tinyxml2 ${RAKNET_LIBRARIES} ${PHYSFS_LIBRARY} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${DEVKITPRO}/portlibs/switch/lib/libEGL.a ${DEVKITPRO}/portlibs/switch/lib/libdrm_nouveau.a ${DEVKITPRO}/portlibs/switch/lib/libglapi.a ${DEVKITPRO}/libnx/lib/libnx.a)
	add_custom_command(
		OUTPUT blobby.nro
		COMMAND ${DEVKITPRO}/tools/bin/elf2nro blobby.elf blobby.nro --icon=${CMAKE_SOURCE_DIR}/data/Icon.jpg --nacp=blobby.nacp
		DEPENDS blobby.elf blobby.nacp)
	add_custom_command(
		OUTPUT blobby.nacp
		COMMAND ${DEVKITPRO}/tools/bin/nacptool --create "${APP_NAME}" "${APP_AUTHOR}" "${PROJECT_VERSION}" VERBATIM blobby.nacp
	)
	add_custom_target(blobby ALL DEPENDS blobby.nro)
else (SWITCH)
	add_executable(blobby main.cpp ${blobby_SRC})
	target_link_libraries(blobby lua raknet blobnet tinyxml2 ${RAKNET_LIBRARIES} ${PHYSFS_LIBRARY} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${ADDITIONAL_LIBRARIES})
endif (SWITCH)


if (UNIX)
	add_executable(blobby-server ${blobby-server_SRC})
	target_link_libraries(blobby-server lua raknet blobnet tinyxml2 ${RAKNET_LIBRARIES} ${PHYSFS_LIBRARY} ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	add_executable(blobby-runtest EXCLUDE_FROM_ALL runtest.cpp ${blobby_SRC})
	target_link_libraries(blobby-runtest lua raknet blobnet tinyxml2 ${RAKNET_LIBRARIES} ${PHYSFS_LIBRARY} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${ADDITIONAL_LIBRARIES})
endif (UNIX)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
	set_target_properties(blobby PROPERTIES LINK_FLAGS "-mwindows") # disable the console window
endif (CMAKE_SYSTEM_NAME STREQUAL Windows)

if (WIN32)
	install(TARGETS blobby DESTINATION .)
elseif (UNIX)
	install(TARGETS blobby blobby-server DESTINATION bin)
elseif (SWITCH)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/blobby.nro DESTINATION .)
endif (WIN32)
