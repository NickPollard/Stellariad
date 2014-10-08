#pragma once
#ifdef __cplusplus
extern "C" 
{
#endif
#include <lualib.h>
#ifdef __cplusplus
}
#endif

void lua_setActiveState( lua_State* l );
void lua_activeStateStack();
