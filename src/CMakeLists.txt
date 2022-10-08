set(APP_NAME "Blobby Volley 2")
set(APP_AUTHOR "")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# CMakeVersion 3.17
# https://cmake.org/cmake/help/latest/release/3.17.html#variables
set(TEST_COMPILE_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_compile_main.cpp)
set(TEST_COMPILE_TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/test_compile.cpp.in)

add_custom_target(test-headers-all)

function(MakeHeaderTests TargetName HeaderDirectory)
	# Header Checks
	# Create a test target, consisting of one cpp file for each header, where the cpp file does nothing but
	# include its header. If this compiles, then all our headers are self-contained.

	## gather up the header files
	file(GLOB_RECURSE HEADERS RELATIVE ${HeaderDirectory} *.h)
	# create a new target for this test. We use a static library (instead of an executable), because then
	# we don't have to worry about things actually linking.
	add_library(${TargetName} STATIC EXCLUDE_FROM_ALL ${TEST_COMPILE_MAIN})

	foreach(HEADER ${HEADERS})
		# construct a new file name based on the headers name
		string(MAKE_C_IDENTIFIER "${HEADER}" HEADER_TEST_FILE)
		set(TEST_COMPILE_FILE "${CMAKE_CURRENT_BINARY_DIR}/test_compile/${TARGET_NAME}/${HEADER_TEST_FILE}.cpp")

		# create the test file and add to the target sources
		configure_file(${TEST_COMPILE_TEMPLATE} ${TEST_COMPILE_FILE} @ONLY)
		target_sources(${TargetName} PUBLIC ${TEST_COMPILE_FILE})
	endforeach()

	add_dependencies(test-headers-all ${TargetName})
endfunction()

find_package(Boost REQUIRED)
add_subdirectory(raknet)
add_subdirectory(blobnet)

if (SWITCH)
	find_library(NX nx)
endif (SWITCH)

include_directories(
	${Boost_INCLUDE_DIR}
)

add_subdirectory(blobby-core)
add_subdirectory(blobby-client)

if (SWITCH)
	add_executable(blobby.elf main.cpp)
	target_link_libraries(blobby.elf blobby-client ${DEVKITPRO}/portlibs/switch/lib/libEGL.a ${DEVKITPRO}/portlibs/switch/lib/libdrm_nouveau.a ${DEVKITPRO}/portlibs/switch/lib/libglapi.a ${DEVKITPRO}/libnx/lib/libnx.a)
	add_custom_command(
		OUTPUT blobby.nro
		COMMAND ${DEVKITPRO}/tools/bin/elf2nro blobby.elf blobby.nro --icon=${CMAKE_SOURCE_DIR}/data/Icon.jpg --nacp=blobby.nacp
		DEPENDS blobby.elf blobby.nacp)
	add_custom_command(
		OUTPUT blobby.nacp
		COMMAND ${DEVKITPRO}/tools/bin/nacptool --create "${APP_NAME}" "${APP_AUTHOR}" "${PROJECT_VERSION}" VERBATIM blobby.nacp
	)
	add_custom_target(blobby ALL DEPENDS blobby.nro)
else (SWITCH)
	add_executable(blobby main.cpp)
	target_link_libraries(blobby PRIVATE blobby-client SDL2::SDL2main)
endif (SWITCH)


if (UNIX)
	find_package(Threads REQUIRED)
	add_executable(blobby-server servermain.cpp)
	target_link_libraries(blobby-server blobby-core Threads::Threads)
	add_executable(blobby-runtest EXCLUDE_FROM_ALL runtest.cpp)
	target_link_libraries(blobby-runtest PRIVATE blobby-client SDL2::SDL2main)
endif (UNIX)

if (WIN32)
	set_target_properties(blobby PROPERTIES LINK_FLAGS "-mwindows") # disable the console window
	install(TARGETS blobby DESTINATION .)
elseif (UNIX)
	install(TARGETS blobby blobby-server DESTINATION bin)
elseif (SWITCH)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/blobby.nro DESTINATION .)
endif ()
