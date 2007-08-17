add_subdirectory(lua)
add_subdirectory(tinyxml)
add_subdirectory(raknet)

add_definitions(-DTIXML_USE_STL)

set(common_SRC	
	Global.h
	NetworkGame.cpp NetworkGame.h
	NetworkMessage.cpp NetworkMessage.h
	PhysicWorld.cpp PhysicWorld.h
	SpeedController.cpp SpeedController.h
	UserConfig.cpp UserConfig.h
	)
	
set (blobby_SRC ${common_SRC}
	Blood.cpp Blood.h
	DuelMatch.cpp DuelMatch.h
	main.cpp
	IMGUI.cpp IMGUI.h
	InputDevice.cpp InputDevice.h
	InputManager.cpp InputManager.h
	InputSource.cpp InputSource.h
	LocalInputSource.h
	NetworkState.cpp NetworkState.h
	OptionsState.cpp OptionsState.h
	RenderManager.cpp RenderManager.h
	RenderManagerGL2D.cpp RenderManagerGL2D.h
	RenderManagerGP2X.cpp RenderManagerGP2X.h
	RenderManagerSDL.cpp RenderManagerSDL.h
	ReplayInputSource.cpp ReplayInputSource.h
	ReplayRecorder.cpp ReplayRecorder.h
	ScriptedInputSource.cpp ScriptedInputSource.h
	SoundManager.cpp SoundManager.h
	State.cpp State.h
	Vector.h
	)
	
set (blobby-server_SRC ${common_SRC}
	DedicatedServer.cpp DedicatedServer.h
	)

find_package(PhysFS REQUIRED)
find_package(SDL REQUIRED)
find_package(OpenGL)

if (OPENGL_FOUND)
	add_definitions(-DHAVE_LIBGL)
endif (OPENGL_FOUND)

add_executable(blobby ${blobby_SRC})
add_executable(blobby-server ${blobby-server_SRC})

target_link_libraries(blobby lua raknet tinyxml ${PHYSFS_LIBRARY} ${SDL_LIBRARY} ${OPENGL_LIBRARIES})

target_link_libraries(blobby-server raknet tinyxml ${PHYSFS_LIBRARY} ${SDL_LIBRARY})
