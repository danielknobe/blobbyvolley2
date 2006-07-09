tinyxml_SOURCES = \
	tinyxml/tinystr.cpp \
	tinyxml/tinyxml.cpp \
	tinyxml/tinyxmlerror.cpp \
	tinyxml/tinyxmlparser.cpp \
	tinyxml/tinystr.h \
	tinyxml/tinyxml.h

bin_PROGRAMS = blobby

blobby_SOURCES = \
	main.cpp \
	InputSource.h LocalInputSource.h \
	InputManager.h InputManager.cpp \
	NetworkPacket.h \
	NetworkManager.h NetworkManager.cpp \
	NetworkServer.h NetworkServer.cpp \
	NetworkClient.h NetworkClient.cpp \
	PhysicWorld.h PhysicWorld.cpp \
	RenderManager.h RenderManager.cpp \
	RenderManagerSDL.h RenderManagerSDL.cpp \
	RenderManagerGP2X.h RenderManagerGP2X.cpp \
	RenderManagerGL2D.h RenderManagerGL2D.cpp \
	SoundManager.h SoundManager.cpp \
	Vector.h \
	UserConfig.cpp UserConfig.h \
	$(tinyxml_SOURCES)
