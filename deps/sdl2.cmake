find_package(SDL2 REQUIRED)
find_package(SDL2_image)

if(NOT TARGET SDL2::SDL2)
    message(STATUS "Could not find SDL2 imported library, defining our own")
    add_library(SDL2 INTERFACE)

    # sdl additional dependencies
    if (CMAKE_SYSTEM_NAME STREQUAL iOS)
        set(SDL2_LIBRARIES ${SDL2_LIBRARIES}
                "-framework AudioToolbox"
                "-framework AVFoundation"
                "-framework CoreAudio"
                "-framework CoreAudioKit"
                "-framework CoreGraphics"
                "-framework CoreMotion"
                "-framework Foundation"
                "-framework GameController"
                "-framework MediaPlayer"
                "-framework OpenGLES"
                "-framework QuartzCore"
                "-framework UIKit")
    endif (CMAKE_SYSTEM_NAME STREQUAL iOS)

    target_include_directories(SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(SDL2 INTERFACE ${SDL2_LIBRARIES})

    add_library(SDL2::SDL2 ALIAS SDL2)
else()
    message(STATUS "Found imported target SDL2::SDL2")
endif()

# https://github.com/libsdl-org/SDL/issues/6119
if(NOT TARGET SDL2::SDL2main)
    add_library(SDL2::SDL2main INTERFACE IMPORTED)
endif()
