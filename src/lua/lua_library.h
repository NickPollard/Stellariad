// lua_library.h
#pragma once
#ifdef __cplusplus
extern "C" 
{
#endif
#include "lua.h"
#ifdef __cplusplus
}
#endif

// Import the lua library into a lua state
void luaLibrary_import( lua_State* l );
