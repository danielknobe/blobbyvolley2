enable_testing()

set(CMAKE_CXX_STANDARD 11)

set(SRC
	../src/BlobbyDebug.cpp    ../src/BlobbyDebug.h
	../src/FileWrite.cpp      ../src/FileWrite.h
	../src/FileRead.cpp       ../src/FileRead.h
	../src/FileSystem.cpp     ../src/FileSystem.h
	../src/File.cpp           ../src/File.h
	../src/GenericIO.cpp      ../src/GenericIO.h
	../src/PlayerInput.h      ../src/PlayerInput.cpp
	../src/DuelMatchState.cpp ../src/DuelMatchState.h
	../src/GameLogicState.cpp ../src/GameLogicState.h
	../src/PhysicState.cpp    ../src/PhysicState.h
	../src/DuelMatch.cpp      ../src/DuelMatch.h
	../src/Clock.cpp          ../src/Clock.h
	../src/PhysicWorld.cpp    ../src/PhysicWorld.h 
	../src/GameLogic.cpp      ../src/GameLogic.h
	../src/InputSource.cpp    ../src/InputSource.h
	../src/IScriptableComponent.cpp ../src/IScriptableComponent.h
	../src/PlayerIdentity.cpp ../src/PlayerIdentity.h
	../src/UserConfig.cpp     ../src/UserConfig.h
	../src/Color.cpp          ../src/Color.h
)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(PhysFS REQUIRED)
find_package(SDL2 REQUIRED)

if ("${SDL2_LIBRARIES}" STREQUAL "")
	set(SDL2_LIBRARIES "SDL2::SDL2")
endif ("${SDL2_LIBRARIES}" STREQUAL "")

add_executable(blobbytest GenericIOTest.cpp ${SRC})

target_include_directories(blobbytest PRIVATE ${Boost_INCLUDE_DIR} ${PHYSFS_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS} ../src)
target_compile_definitions(blobbytest PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(blobbytest ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${PHYSFS_LIBRARY} ${SDL2_LIBRARIES} lua raknet tinyxml2)
