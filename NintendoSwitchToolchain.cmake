set(DEVKITPRO $ENV{DEVKITPRO})
set(NX 1)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc")
set(CMAKE_CXX_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-g++")
set(CMAKE_AR "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc-ar" CACHE STRING "")
set(CMAKE_RANLIB "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc-ranlib" CACHE STRING "")

set(PKG_CONFIG "${DEVKITPRO}/portlibs/bin/aarch64-none-elf-pkg-config" CACHE STRING "")
set(CPPFLAGS "-D__SWITCH__ -I${DEVKITPRO}/libnx/include -I${DEVKITPRO}/portlibs/switch/include -DLUA_32BITS -DDEBUG")
set(CMAKE_C_FLAGS "${CPPFLAGS} -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIC -ffunction-sections" CACHE STRING "C flags")
#set(CMAKE_CXX_FLAGS "${CPPFLAGS} ${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -std=gnu++11" CACHE STRING "C++ flags")
set(CMAKE_CXX_FLAGS "${CPPFLAGS} ${CMAKE_C_FLAGS} -std=gnu++11" CACHE STRING "C++ flags")
set(CMAKE_EXE_LINKER_FLAGS "-specs=${DEVKITPRO}/libnx/switch.specs -g -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE")

set(CMAKE_FIND_ROOT_PATH ${DEVKITPRO}/devkitA64 ${DEVKITPRO}/libnx ${DEVKITPRO}/portlibs/switch)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${DEVKITPRO}/portlibs/switch/lib/cmake)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Shared libs not available")
link_directories(${DEVKITPRO}/libnx/lib ${DEVKITPRO}/portlibs/switch/lib)