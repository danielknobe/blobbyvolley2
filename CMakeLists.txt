cmake_minimum_required(VERSION 3.10)

project(blobby VERSION 1.1.1)

option(BUILD_TESTS "Build test programs" OFF)
option(BUILD_MACOS_BUNDLE "Create a self-containing MacOS bundle" OFF)

if (POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

include(deps/sdl2.cmake)
include(deps/physfs.cmake)

add_subdirectory(data)
add_subdirectory(src)
add_subdirectory(deps)

add_subdirectory(linux)

add_subdirectory(macos)

if(BUILD_TESTS)
	add_subdirectory(test)
endif()

include(CPack)
