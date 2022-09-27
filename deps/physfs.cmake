find_package(PhysFS REQUIRED)

# PhysFS does not provide any CMake targets, so we make our own
add_library(PhysFS INTERFACE)
target_include_directories(PhysFS INTERFACE ${PHYSFS_INCLUDE_DIR})
target_link_libraries(PhysFS INTERFACE ${PHYSFS_LIBRARY})
add_library(PhysFS::PhysFS ALIAS PhysFS)
