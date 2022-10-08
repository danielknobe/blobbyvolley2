enable_testing()

set(CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(PhysFS REQUIRED)
find_package(SDL2 REQUIRED)

if ("${SDL2_LIBRARIES}" STREQUAL "")
	set(SDL2_LIBRARIES "SDL2::SDL2")
endif ("${SDL2_LIBRARIES}" STREQUAL "")

add_executable(blobbytest GenericIOTest.cpp FileTest.cpp)

target_include_directories(blobbytest PRIVATE ${Boost_INCLUDE_DIR} ${PHYSFS_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS} ../src)
target_compile_definitions(blobbytest PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(blobbytest PRIVATE ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} blobby-core)
