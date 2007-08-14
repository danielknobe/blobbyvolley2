add_definitions(-DTIXML_USE_STL)

set(tinyxml_SRC
	tinystr.cpp tinystr.h
	tinyxml.cpp tinyxml.h
	tinyxmlerror.cpp
	tinyxmlparser.cpp
	)

add_library(tinyxml STATIC ${tinyxml_SRC})
