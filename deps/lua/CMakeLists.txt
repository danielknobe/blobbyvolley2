set (lua_SRC
	lapi.c lapi.h
	lauxlib.c lauxlib.h
	lbaselib.c
	lbitlib.c
	lcode.c lcode.h
	lcorolib.c
	lctype.c lctype.h
	ldblib.c
	ldebug.c ldebug.h
	ldo.c ldo.h
	ldump.c
	lfunc.c lfunc.h
	lgc.c lgc.h
	linit.c
	liolib.c
	llex.c llex.h
	llimits.h
	lmathlib.c
	lmem.c lmem.h
	loadlib.c
	lobject.c lobject.h
	lopcodes.c lopcodes.h
#	loslib.c
	lparser.c lparser.h
	lprefix.h
	lstate.c lstate.h
	lstring.c lstring.h
	lstrlib.c
	ltable.c ltable.h
	ltablib.c
	ltm.c ltm.h
#	lua.c lua.h lua.hpp
#	luac.c
	luaconf.h
	lualib.h
	lundump.c lundump.h
	lutf8lib.c
	lvm.c lvm.h
	lzio.c lzio.h
	)

add_library(lua STATIC ${lua_SRC})
add_library(lua::lua ALIAS lua)
target_include_directories(lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")