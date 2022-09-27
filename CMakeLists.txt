cmake_minimum_required(VERSION 3.3)

include(CPack)

project(Blobby)

option(BUILD_TESTS "Build test programs" OFF)

# process the config.h
configure_file(${Blobby_SOURCE_DIR}/config.h.in
               ${Blobby_BINARY_DIR}/config.h)
include_directories(${Blobby_BINARY_DIR})

add_subdirectory(data)
add_subdirectory(src)
add_subdirectory(deps)

add_subdirectory(linux)

if(BUILD_TESTS)
	add_subdirectory(test)
endif(BUILD_TESTS)
