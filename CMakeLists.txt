cmake_minimum_required(VERSION 3.7)

include(CPack)

project(Blobby)

option(BUILD_TESTS "Build test programs" OFF)
option(BUILD_MACOS_BUNDLE "Create a self-containing MacOS bundle" OFF)

include(deps/sdl2.cmake)
include(deps/physfs.cmake)

add_subdirectory(data)
add_subdirectory(src)
add_subdirectory(deps)

add_subdirectory(linux)

if(BUILD_MACOS_BUNDLE)
	add_subdirectory(macos)
endif()

if(BUILD_TESTS)
	add_subdirectory(test)
endif()
