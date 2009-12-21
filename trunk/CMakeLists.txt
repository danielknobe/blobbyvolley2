cmake_minimum_required(VERSION 2.6)

include(CPack)

project(Blobby)

# process the config.h
configure_file(${Blobby_SOURCE_DIR}/config.h.in
               ${Blobby_BINARY_DIR}/config.h)
include_directories(${Blobby_BINARY_DIR})

add_subdirectory(data)
add_subdirectory(src)

