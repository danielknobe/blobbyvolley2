set (blobnet_SRC
	layer/Http.cpp layer/Http.hpp
	)

find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

add_library(blobnet STATIC ${blobnet_SRC})	
