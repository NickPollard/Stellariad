// Lua.c
#include "common.h"
#include "lua.h"
//---------------------
#include "engine.h"
#include "lua/lua_library.h"
#include "maths/quaternion.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "system/file.h"
#include "system/string.h"
#include "render/render.h"

#define kLuaMaxVectors 64
vector lua_vectors[kLuaMaxVectors];
int lua_vector_count = 0;
#define kLuaMaxQuaternions 64
quaternion lua_quaternions[kLuaMaxQuaternions];
int lua_quaternion_count = 0;
#define kLuaMaxMatrices 64
matrix lua_matrices[kLuaMaxMatrices];
int lua_matrix_count = 0;

lua_State* active_lua_state = NULL;

const char* game_lua_path = NULL;

void lua_keycodes( lua_State* l );

// *** Helpers ***

void lua_setActiveState( lua_State* l ) {
	active_lua_state = l;
}

void luaAssert( lua_State* l, bool condition ) {
	if ( !condition ) {
		lua_stacktrace( l );
		vAssert( condition );
	}
}

void lua_assertnumber( lua_State* l, int index ) {
	luaAssert( l, lua_isnumber( l, index ));
}
void* lua_toptr( lua_State* l, int index ) {
	lua_assertnumber( l, index );
	uintptr_t ptr = lua_tonumber( l, index );
	return (void*)ptr;
}

// ***

void lua_staticInit() {
	// the vector count starts at 0, is reset to 0 every frame
	// this is for storing temporary vectors
	lua_vector_count = 0;
}

void lua_preTick( lua_State* l, float dt ) {
	// reset vectors at the start of every tick
	lua_vector_count = 0;

	// Send the dt value to LUA for it to use
	lua_pushnumber( l, dt );
	lua_setglobal( l, "dt" ); // store the table in the 'key' global variable
}

// Is the luaCallback currently enabled?
bool luaCallback_enabled(luaCallback* l) {
	return l->enabled;
}

// Add a callback to the interface
luaCallback* luaInterface_addCallback(luaInterface* i, const char* name) {
	luaCallback* l = &i->callbackArray[i->callbackCount++];
	l->name = name;
	return l;
}

// Create a luaInterface
luaInterface* luaInterface_create() {
	luaInterface* i = mem_alloc(sizeof(luaInterface));
	i->callbackCount = 0;
	memset(&i->callbackArray[0], 0, sizeof(luaCallback) * MAX_CALLBACKS);
	return i;
}

// Find a luaCallback
luaCallback* luaInterface_findCallback(luaInterface* interface, const char* name) {
	for (int i = 0; i < MAX_CALLBACKS; i++) {
		if (strcmp(interface->callbackArray[i].name, name) == 0)
			return &interface->callbackArray[i];
	}
	return NULL;
}

// Register a luaCallback
void luaInterface_registerCallback(luaInterface* i, const char* name, const char* func) {
	luaCallback* callBack = luaInterface_findCallback(i, name);
	callBack->func = func;
	callBack->enabled = true;
}


// ***


int LUA_registerCallback( lua_State* l ) {
	(void)l;
	printf("Register callback!\n");
	return 0;
}

void lua_pushptr( lua_State* l, void* ptr ) {
	uintptr_t pointer = (uintptr_t)ptr;
	lua_pushnumber( l, (double)pointer );
}

void lua_retrieve( lua_State* l, int ref ) {
	lua_rawgeti( l, LUA_REGISTRYINDEX, ref );
}

int lua_store( lua_State* l ) {
	return luaL_ref( l, LUA_REGISTRYINDEX );
}

void lua_runFunc( lua_State* l, int ref, int args ) {
	lua_retrieve( l, ref );
	int err = lua_pcall( l, args, 0, 0 );
	if ( err != 0 ) {
		printf( "LUA ERROR: ErrorNum: %d.\n", err );
		vAssert( 0 );
	}
}

vector lua_tovector3( lua_State* l, int i ) {
	return Vector( lua_tonumber( l, i ), lua_tonumber( l, i+1 ), lua_tonumber( l, i+2 ), 0.f );
}

vector* lua_createVector( ) {
	if ( !(lua_vector_count < kLuaMaxVectors) )
		lua_vector_count = 0;
	assert( lua_vector_count < kLuaMaxVectors );
	vector* v = &lua_vectors[lua_vector_count++];
	return v;
}

matrix* lua_createMatrix() {
	if ( lua_matrix_count >= kLuaMaxMatrices )
		lua_matrix_count = 0;
	vAssert( lua_matrix_count < kLuaMaxMatrices );
	matrix* m = &lua_matrices[lua_matrix_count++];
	return m;
}

quaternion* lua_createQuaternion( ) {
	if ( !(lua_quaternion_count < kLuaMaxQuaternions) )
		lua_quaternion_count = 0;
	assert( lua_quaternion_count < kLuaMaxQuaternions );
	quaternion* q = &lua_quaternions[lua_quaternion_count++];
	return q;
}

void lua_setConstant_bool( lua_State* l, const char* name, bool b ) {
	lua_pushboolean( l, b );
	lua_setglobal( l, name ); // Store in the global variable named <name>
}

void lua_setConstant_ptr( lua_State* l, const char* name, void* ptr ) {
	lua_pushptr( l, ptr );
	lua_setglobal( l, name ); // Store in the global variable named <name>
}

void lua_setConstant_int( lua_State* l, const char* name, int num ) {
	lua_pushnumber( l, (double)num );
	lua_setglobal( l, name ); // Store in the global variable named <name>
}

void lua_setConstant_string( lua_State* l, const char* name, const char* string ) {
	lua_pushstring( l, string );
	lua_setglobal( l, name );
}
// ***


void lua_registerFunction( lua_State* l, lua_CFunction func, const char* name ) {
    lua_pushcfunction( l, func );
    lua_setglobal( l, name );
}

#ifdef ANDROID
int LUA_androidOpen( lua_State* l ) {
	char filename[kMaxPath];
	const char* modulename = lua_tostring( l, 1 );
	sprintf( filename, "%s%s.lua", game_lua_path, modulename );
	printf( "Android loading Lua file \"%s\"\n", filename );
	int length = -1;
	const char* buffer = vfile_contents( filename, &length );
	if ( luaL_loadbuffer( l, buffer, length, filename )) {
		printf("Error: Failed loading lua from file %s!\n", filename );
		vAssert( 0 );
	}

	/*
	int err = lua_pcall( l, 0, 0, 0 );
	if ( err != 0 ) {
		printf( "LUA ERROR: ErrorNum: %d.\n", err );
		vAssert( 0 );
	}
	*/
	return 1; // The module
}

// Set up lua loader for android
// package.loaders[5] = android_loader
void lua_setupAndroidLoader( lua_State* l ) {
	// Get the package.loaders table
	lua_getglobal( l, "package" );
	lua_getfield( l, -1, "loaders" );

	// Set the [5] value
	int key = 5;
	lua_pushinteger( l, key );
	lua_pushcfunction( l, LUA_androidOpen );
	lua_settable( l, -3 ); // The table is at -3, as we have the key and the value on top
}
#endif // ANDROID

void lua_setPackagePath( lua_State* l, const char* path ) {
	lua_getglobal(l, "package");
	lua_pushstring(l, path);
	lua_setfield(l, -2, "path");
	lua_pop(l, 1); // remove package table
}

// Create a Lua l and load it's initial contents from <filename>
lua_State* vlua_create( engine* e, const char* filename ) {
	lua_State* l = lua_open();

	luaL_openlibs( l );	// Load the Lua libs into our lua l

#ifdef ANDROID
	lua_setupAndroidLoader( l );
#endif // ANDROID
	lua_setPackagePath( l, "./SpaceSim/lua/?.lua;./SpaceSim/lua/compiled/?.lua" );

	// We now use luaL_loadbuffer rather than luaL_loadfile as on Android we need
	// to go through Libzip to get the data
	size_t length;
	const char* buffer = vfile_contents( filename, &length );
	if ( luaL_loadbuffer( l, buffer, length, filename )) {
		printf("Error: Failed loading lua from file %s!\n", filename );
		printf( "%s\n", lua_tostring( l, -1 ));
		vAssert( 0 );
	}

	luaLibrary_import( l );

	lua_setConstant_ptr( l, "engine", e );
	lua_setConstant_int( l, "screen_width", engine_mainWindow( e ).width );
	lua_setConstant_int( l, "screen_height", engine_mainWindow( e ).height );
	lua_setConstant_ptr( l, "input", e->input );

	int err = lua_pcall( l, 0, 0, 0 );
	if ( err != 0 ) {
		printf( "LUA ERROR: ErrorNum: %d.\n", err );
		printf( "%s\n", lua_tostring( l, -1 ));
		vAssert( 0 );
	}

	// *** Always call init
	LUA_CALL( l, "init" );

	return l;
}

void lua_setScene( lua_State* l, scene* s ) {
	lua_setConstant_ptr( l, "scene", s );
}

// Sets a field for the table that is assumed to be on the top of the Lua stack
void lua_setfieldi( lua_State* l, const char* key, int value ) {
	lua_pushstring( l, key );
	lua_pushnumber( l, (double)value );
	lua_settable( l, -3 ); // The table is at -3, as we have the key and the value on top
}
// Sets a field for the table that is assumed to be on the top of the Lua stack
void lua_setfieldf( lua_State* l, const char* key, float value ) {
	lua_pushstring( l, key );
	lua_pushnumber( l, (double)value );
	lua_settable( l, -3 ); // The table is at -3, as we have the key and the value on top
}

void lua_setGameLuaPath( const char* path ) {
	game_lua_path = string_createCopy( path );
}

void lua_stacktrace( lua_State* l ) {
	printf( "#### Lua Stacktrace ####\n" );
	if ( l ) {
		lua_Debug debug_entry;
		int depth = 0;
		while (lua_getstack( l, depth, &debug_entry )) {
			int status = lua_getinfo( l, "Sln", &debug_entry );
			vAssert(status);
			printf( "%s(%d): %s\n", debug_entry.short_src, debug_entry.currentline, debug_entry.name ? debug_entry.name : "?" );
			depth++;
		}
	}
	printf( "########################\n" );
}

void lua_activeStateStack() {
	lua_stacktrace( active_lua_state );
}
