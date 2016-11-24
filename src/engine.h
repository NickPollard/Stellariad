// engine.h
#pragma once

#include "base/list.h"
#include "vlua.h"
#include "ticker.h"
#include "vtime.h"

#include "concurrent/task.h"

#ifdef ANDROID
// Android Libraries
#include <android/log.h>
#include <android_native_app_glue.h>
// EGL
#include <EGL/egl.h>
#endif

#ifdef LINUX_X
#include <X11/Xlib.h>
#endif // LINUX_X

// Lua Libraries
#ifdef __cplusplus
extern "C" 
{
#endif
#include <lua.h>
#ifdef __cplusplus
}
#endif

#define DEBUG_LUA false

#if 0
#define GRAPH_GPU_FPS
#define GRAPH_FPS
#endif

using brando::concurrent::Executor;
using brando::concurrent::ThreadPoolExecutor;

// TODO - remove this! No static scene
extern scene* theScene;

extern int threadsignal_render;

DEF_LIST(delegate)
#define kDefaultDelegateSize 16

#ifdef LINUX_X
// TODO - move out of Engine
struct xwindow_s {
	Display* display;
	Window window;
	bool open;
};

extern xwindow xwindow_main;
#endif

struct engine {
  engine() : executor(new ThreadPoolExecutor(1)) {}

	// *** General
	frame_timer* timer;
	int	frame_counter;
	input* _input;
	bool paused;

	// *** Lua
  // TODO - move into one Lua struct?
	lua_State* lua;          //!< Engine wide persistant lua state
	luaInterface* callbacks; //!< Lua Interface for callbacks from the engine
	luaCallback* onTick;     //!< OnTick event handler

  // TODO - write new templated delegate
	delegatelist* tickers;	
	delegatelist* post_tickers;	
	delegatelist* renders;
	delegatelist* inputs;

	debugtextframe* debugtext;

  Executor& ex();

  // TODO Do we need both of these?
	bool running;
	bool active; // seems to come from android only?
#ifdef ANDROID
	struct android_app* app;
#endif

  private:
    unique_ptr<Executor> executor;
};

extern engine* static_engine_hack;

/*
 *
 *  Static Functions
 *
 */

// init - initialises the application
void init(int argc, char** argv);

/*
 *
 *  Engine Methods
 *
 */

// Create an engine
engine* engine_create();

// Initialise the engine
void engine_init(engine* e, int argc, char** argv);

// Initialise the OpenGL subsystem so it is ready for use
//void engine_initOpenGL(engine* e, int argc, char** argv);

// Initialise the Lua subsystem so that it is ready for use
//void engine_initLua(engine* e, int argc, char** argv);

// terminate - de-initialises the engine
void engine_terminate(engine* e);

// terminateLua - de-initialises the Lua interpreter
//void engine_terminateLua(engine* e);

// run - executes the main loop of the application
void engine_run(engine* e);

// Tick the engine, processing a frame of game update
void engine_tick(engine* e);

// Handle a key press from the user
void engine_handleKeyPress(engine* e, uchar key, int x, int y);

// Look for a delegate of the right type to add this entity too
// If one is not found, create one
void startTick( engine* e, void* entity, tickfunc tick );
void startPostTick( engine* e, void* entity, tickfunc tick );

void startInput( engine* e, void* entity, inputfunc input );

void engine_addRender( engine* e, void* entity, renderfunc render );

void engine_removeRender( engine* e, void* entity, renderfunc render );
void stopTick( engine* e, void* entity, tickfunc tick );
void stopPostTick( engine* e, void* entity, tickfunc tick );

window engine_mainWindow( engine* e );
