cmake_minimum_required(VERSION 3.3)

# Work around older versions of CMake not providing the ANDROID variable
if(${CMAKE_VERSION} VERSION_LESS "3.7")
	if (UNIX AND (${CMAKE_SYSTEM_NAME} STREQUAL "Android"))
		set(ANDROID ON)
	endif()
endif()

include(CPack)

project(Blobby)

option(BUILD_TESTS "Build test programs" OFF)

# process the config.h
configure_file(${Blobby_SOURCE_DIR}/config.h.in
               ${Blobby_BINARY_DIR}/config.h)
include_directories(${Blobby_BINARY_DIR})

include(deps/sdl2.cmake)
include(deps/physfs.cmake)

add_subdirectory(data)
add_subdirectory(src)
add_subdirectory(deps)

add_subdirectory(linux)

if(BUILD_TESTS)
	add_subdirectory(test)
endif(BUILD_TESTS)
