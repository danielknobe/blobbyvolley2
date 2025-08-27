set(CMAKE_CXX_STANDARD 11)

set (blobnet_SRC
	layer/Http.cpp layer/Http.hpp
	layer/HttpRequestHeader.cpp layer/HttpRequestHeader.hpp
	)

add_library(blobnet STATIC ${blobnet_SRC})	
target_link_libraries(blobnet PUBLIC raknet::raknet)
target_include_directories(blobnet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(blobnet::blobnet ALIAS blobnet)