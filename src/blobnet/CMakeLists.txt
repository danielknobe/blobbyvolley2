set(CMAKE_CXX_STANDARD 11)

set (blobnet_SRC
	layer/Http.cpp layer/Http.hpp
	)

add_library(blobnet STATIC ${blobnet_SRC})	
