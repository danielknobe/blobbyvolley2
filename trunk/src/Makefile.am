SUBDIRS = lua tinyxml

AM_CXXFLAGS = -DTIXML_USE_STL=1 -DGAMEDATADIR=\"$(GAMEDATADIR)\" -DHAVE_LIBGL=$(HAVE_LIBGL) -Wall
AM_LDFLAGS = -Llua -Ltinyxml

bin_PROGRAMS = blobby

blobby_LDADD = -llua -ltinyxml $(GL_LIBS)

blobby_SOURCES = \
	DuelMatch.h DuelMatch.cpp \
	main.cpp \
	InputDevice.h InputDevice.cpp \
	InputSource.h InputSource.cpp \
	LocalInputSource.h \
	IMGUI.h IMGUI.cpp \
	InputManager.h InputManager.cpp \
	Global.h \
	NetworkPacket.h \
	NetworkManager.h NetworkManager.cpp \
	NetworkServer.h NetworkServer.cpp \
	NetworkClient.h NetworkClient.cpp \
	PhysicWorld.h PhysicWorld.cpp \
	RenderManager.h RenderManager.cpp \
	RenderManagerSDL.h RenderManagerSDL.cpp \
	RenderManagerGP2X.h RenderManagerGP2X.cpp \
	RenderManagerGL2D.h RenderManagerGL2D.cpp \
	ReplayInputSource.h ReplayInputSource.cpp \
	ReplayRecorder.h ReplayRecorder.cpp \
	ScriptedInputSource.h ScriptedInputSource.cpp \
	SpeedController.h SpeedController.cpp \
	State.h State.cpp\
	SoundManager.h SoundManager.cpp \
	Vector.h \
	UserConfig.cpp UserConfig.h

