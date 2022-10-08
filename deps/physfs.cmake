option(BLOBBY_BUILD_PHYSFS "Build own version of PhysFS even if the system provides one" OFF)

if (NOT BLOBBY_BUILD_PHYSFS)
    find_package(PhysFS)
endif()

if ( ${PhysFS_FOUND} )
    # PhysFS does not provide any CMake targets, so we make our own
    add_library(PhysFS INTERFACE)
    target_include_directories(PhysFS INTERFACE ${PHYSFS_INCLUDE_DIR})
    target_link_libraries(PhysFS INTERFACE ${PHYSFS_LIBRARY})
    add_library(PhysFS::PhysFS ALIAS PhysFS)
else ()
    # If we cannot find physfs, we build our own
    if (NOT BLOBBY_BUILD_PHYSFS)
        message(STATUS "Could not find physfs library! PhysFS source is added as subdirectory")
    else()
        message(STATUS "Building PhysFS as requested.")
    endif()

    # only build the parts of physfs we actually need
    set(DISABLE_THESE_OPTIONS PHYSFS_BUILD_DOCS PHYSFS_ARCHIVE_GRP PHYSFS_ARCHIVE_WAD PHYSFS_ARCHIVE_HOG
            PHYSFS_ARCHIVE_MVL PHYSFS_ARCHIVE_QPAK PHYSFS_ARCHIVE_SLB PHYSFS_ARCHIVE_ISO9660 PHYSFS_ARCHIVE_VDF
            PHYSFS_BUILD_STATIC)
    foreach(PHYSFS_OPT ${DISABLE_THESE_OPTIONS} )
        option(${PHYSFS_OPT} "" OFF)
    endforeach()

    add_subdirectory(deps/physfs)
endif ()