SUBDIRS = lua tinyxml raknet

WANTED_WARNING = -Wall

AM_CXXFLAGS = -DTIXML_USE_STL=1 -DGAMEDATADIR=\"$(GAMEDATADIR)\" -DHAVE_LIBGL=$(HAVE_LIBGL)
AM_LDFLAGS = -Llua -Ltinyxml -Lraknet
common_LDADD = -ltinyxml -lraknet

bin_PROGRAMS = blobby blobby-server
blobby_LDADD = $(common_LDADD) $(GL_LIBS) -llua
blobby_server_LDADD = $(common_LDADD)

common_SOURCES = \
	InputSource.h InputSource.cpp \
	Global.h \
	NetworkPacket.h \
	PhysicWorld.h PhysicWorld.cpp \
	SpeedController.h SpeedController.cpp \
	Vector.h \
	UserConfig.cpp UserConfig.h

blobby_SOURCES = $(common_SOURCES) \
	DuelMatch.h DuelMatch.cpp \
	main.cpp \
	InputDevice.h InputDevice.cpp \
	LocalInputSource.h \
	IMGUI.h IMGUI.cpp \
	InputManager.h InputManager.cpp \
	RenderManager.h RenderManager.cpp \
	RenderManagerSDL.h RenderManagerSDL.cpp \
	RenderManagerGP2X.h RenderManagerGP2X.cpp \
	RenderManagerGL2D.h RenderManagerGL2D.cpp \
	ReplayInputSource.h ReplayInputSource.cpp \
	ReplayRecorder.h ReplayRecorder.cpp \
	ScriptedInputSource.h ScriptedInputSource.cpp \
	State.h State.cpp\
	SoundManager.h SoundManager.cpp

blobby_server_SOURCES = $(common_SOURCES) \
	DedicatedServer.h DedicatedServer.cpp
