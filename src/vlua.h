#pragma once

#include "maths/mathstypes.h"

// Lua Libraries
#ifdef __cplusplus
extern "C" 
{
#endif
#include <lauxlib.h>
#include <lualib.h>
#ifdef __cplusplus
}
#endif

#define LUA_DEBUG false

#if LUA_DEBUG
#define LUA_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define LUA_DEBUG_PRINT(...)
#endif

#define LUA_CALL(lua, func)					\
	lua_getglobal(lua, func);				\
	lua_pcall(lua,	/* args */			0,	\
					/* returns */		0,	\
					/* error handler */ 0);

#define MAX_CALLBACKS 8

// Lua Callback wrapper for Lua event handlers
struct luaCallback_s {
	int enabled;
	const char* name;
	const char* func;
};

typedef struct luaCallback_s luaCallback;

/*
 *	luaInterface - wrapper class for Lua Callbacks
 *  Every class that wants to use Lua callbacks has one of these
 *  This is what is passed to the lua state for control of callbacks
 */
struct luaInterface_s {
	int	callbackCount;
	luaCallback callbackArray[MAX_CALLBACKS];
};

typedef struct luaInterface_s luaInterface;

void lua_preTick( lua_State* l, float dt );

// Is the luaCallback currently enabled?
bool luaCallback_enabled(luaCallback* l);

// Add a callback to the interface
luaCallback* luaInterface_addCallback(luaInterface* i, const char* name);

// Create a luaInterface
luaInterface* luaInterface_create();

// Find a luaCallback
luaCallback* luaInterface_findCallback(luaInterface* interface, const char* name);

// Register a luaCallback
void luaInterface_registerCallback(luaInterface* i, const char* name, const char* func);

int LUA_registerCallback(lua_State* l);

void lua_registerFunction(lua_State* l, lua_CFunction func, const char* name);

// Create a Lua State and load it's initial contents from <filename>
lua_State* vlua_create( engine* e, const char* filename );

// Sets the engine scene used by this lua_state
void lua_setScene( lua_State* l, scene* s );

// Sets a field for the table that is assumed to be on the top of the Lua stack
void lua_setfieldi( lua_State* l, const char* key, int value );

// Sets a field for the table that is assumed to be on the top of the Lua stack
void lua_setfieldf( lua_State* l, const char* key, float value );

// Create a vector from 3 consecutive number lua arguments
vector lua_tovector3( lua_State* l, int i );

void lua_pushptr( lua_State* l, void* ptr );
void lua_retrieve( lua_State* l, int ref );
int lua_store( lua_State* l );
void* lua_toptr( lua_State* l, int index );
void lua_assertnumber( lua_State* l, int index );
void luaAssert( lua_State* l, bool condition );
vector* lua_createVector( );
matrix* lua_createMatrix();
quaternion* lua_createQuaternion( );
void lua_setConstant_bool( lua_State* l, const char* name, bool b );

// Set the lua path to look for required modules
void lua_setGameLuaPath( const char* path );

// Dump the lua call stack
int lua_stacktrace( lua_State* l );

// Handle a just-caught lua error
int lua_errorHandler( lua_State* l );
