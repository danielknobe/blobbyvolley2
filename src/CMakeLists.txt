set(APP_NAME "Blobby Volley 2")
set(APP_AUTHOR "")

set(CMAKE_CXX_STANDARD 11)

if (MSVC)
	add_compile_options(/utf-8)
	# When building for MSVC with CMake,
	# the compiler warning flags (like /W3) are added by default.
else()
	add_compile_options(-Wall)
endif()

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
	Color.cpp Color.h
	NetworkMessage.cpp NetworkMessage.h
	PhysicWorld.cpp PhysicWorld.h
	RateController.cpp RateController.h
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
	RenderManagerNull.cpp RenderManagerNull.h
	ScriptedInputSource.cpp ScriptedInputSource.h
	SoundManager.cpp SoundManager.h
	Vector.h
	replays/ReplayPlayer.cpp replays/ReplayPlayer.h
	replays/ReplayLoader.cpp
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
	BlobbyApp.h BlobbyApp.cpp
	)

set (blobby-server_SRC ${common_SRC}
	server/servermain.cpp
	)

find_package(Boost REQUIRED)
find_package(OpenGL)
add_subdirectory(raknet)
add_subdirectory(blobnet)

if (SWITCH)
	find_library(NX nx)
endif (SWITCH)

if (OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	add_definitions(-DHAVE_LIBGL)
endif (OPENGL_FOUND)

if (UNIX AND (NOT ANDROID))
	add_definitions("-DBLOBBY_DATA_DIR=\"${CMAKE_INSTALL_PREFIX}/share/blobby\"")
endif()

set(BLOBBY_COMMON_LIBS lua::lua blobnet::blobnet tinyxml2::tinyxml2 PhysFS::PhysFS SDL2::SDL2main SDL2::SDL2 Boost::boost)

# other additional dependencies
if (NOT WIN32)
	find_package(Threads REQUIRED)
	set(BLOBBY_COMMON_LIBS ${BLOBBY_COMMON_LIBS} Threads::Threads)
endif ()

if (SWITCH)
	add_executable(blobby.elf main.cpp ${blobby_SRC})
	target_link_libraries(blobby.elf ${DEVKITPRO}/portlibs/switch/lib/libEGL.a ${DEVKITPRO}/portlibs/switch/lib/libdrm_nouveau.a ${DEVKITPRO}/portlibs/switch/lib/libglapi.a ${DEVKITPRO}/libnx/lib/libnx.a)
	add_custom_command(
		OUTPUT blobby.nro
		COMMAND ${DEVKITPRO}/tools/bin/elf2nro blobby.elf blobby.nro --icon=${CMAKE_SOURCE_DIR}/data/Icon.jpg --nacp=blobby.nacp
		DEPENDS blobby.elf blobby.nacp)
	add_custom_command(
		OUTPUT blobby.nacp
		COMMAND ${DEVKITPRO}/tools/bin/nacptool --create "${APP_NAME}" "${APP_AUTHOR}" "${PROJECT_VERSION}" VERBATIM blobby.nacp
	)
	add_custom_target(blobby ALL DEPENDS blobby.nro)
else ()
	if (ANDROID)
		add_library(blobby SHARED main.cpp ${blobby_SRC})
	elseif (WIN32)
	    add_executable(blobby WIN32 main.cpp ${blobby_SRC})
	else ()
		add_executable(blobby main.cpp ${blobby_SRC})
	endif ()
endif ()

target_link_libraries(blobby ${BLOBBY_COMMON_LIBS} ${OPENGL_LIBRARIES})


if (UNIX AND (NOT ANDROID) OR WIN32)
	add_executable(blobby-server ${blobby-server_SRC})
	target_link_libraries(blobby-server ${BLOBBY_COMMON_LIBS})
endif ()
if (UNIX AND (NOT ANDROID))
	add_executable(blobby-runtest EXCLUDE_FROM_ALL runtest.cpp ${blobby_SRC})
	target_link_libraries(blobby-runtest ${BLOBBY_COMMON_LIBS} ${OPENGL_LIBRARIES})

	add_executable(botbench EXCLUDE_FROM_ALL botbench.cpp ${blobby_SRC})
	target_link_libraries(botbench ${BLOBBY_COMMON_LIBS} ${OPENGL_LIBRARIES})
endif ()

if (MSYS)
	set_target_properties(blobby PROPERTIES LINK_FLAGS "-mwindows") # disable the console window
	set_target_properties(blobby-server PROPERTIES LINK_FLAGS "-mconsole") # enable the console window
endif()

if (WIN32)
	install(TARGETS blobby blobby-server DESTINATION .)
elseif (ANDROID)
	install(TARGETS blobby DESTINATION bin)
elseif (SWITCH)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/blobby.nro DESTINATION .)
elseif (UNIX AND NOT APPLE)
	install(TARGETS blobby blobby-server DESTINATION bin)
endif ()

if (MSVC)
	# set application icon
	target_sources(blobby PRIVATE ../win/res.rc)
	target_sources(blobby-server PRIVATE ../win/res.rc)
endif(MSVC)

# Header Checks
# Create a test target, consisting of one cpp file for each header, where the cpp file does nothing but
# include its header. If this compiles, then all our headers are self-contained.

# gather up the header files
file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)

# create a new target for this test. We use a static library (instead of an executable), because then
# we don't have to worry about things actually linking.
add_library(test-compile-headers STATIC EXCLUDE_FROM_ALL test_compile_main.cpp)

foreach(HEADER ${HEADERS})
	# construct a new file name based on the headers name
	string(MAKE_C_IDENTIFIER "${HEADER}" HEADER_TEST_FILE)
	set(TEST_COMPILE_FILE "${CMAKE_CURRENT_BINARY_DIR}/test_compile/${HEADER_TEST_FILE}.cpp")

	# create the test file and add to the target sources
	configure_file(test_compile.cpp.in ${TEST_COMPILE_FILE} @ONLY)
	target_sources(test-compile-headers PUBLIC ${TEST_COMPILE_FILE})
endforeach()

target_link_libraries(test-compile-headers PRIVATE ${BLOBBY_COMMON_LIBS})
