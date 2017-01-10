// engine.c
#include "src/common.h"
#include "src/engine.h"
//---------------------
#include "collision/collision.h"
#include "dynamicfog.h"
#include "font.h"
#include "future.h"
#include "input.h"
#include "log.h"
#include "lua.h"
#include "maths/maths.h"
#include "model.h"
#include "noise.h"
#include "particle.h"
#include "ribbon.h"
#include "scene.h"
#include "skybox.h"
#include "transform.h"
#include "worker.h"
#include "camera/flycam.h"
#include "debug/debug.h"
#include "debug/debugtext.h"
#include "debug/debuggraph.h"
#include "input/keyboard.h"
#include "mem/allocator.h"
#include "render/debugdraw.h"
#include "render/modelinstance.h"
#include "render/render.h"
#include "render/texture.h"
#include "script/lisp.h"
#include "system/file.h"
#include "system/string.h"
#include "system/thread.h"
#include "terrain/marching.h"
#include "terrain/canyon_terrain.h"

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

// System Libraries
#include <stdlib.h>

// TODO - move to new Delegate System
IMPLEMENT_LIST(delegate)

#define kNumWorkerThreads 4

// System libraries

// TODO - move this somewhere better
// *** Static Hacks
scene* theScene = NULL;

// Function Declarations
void engine_tickTickers( engine* e, float dt );
void engine_tickPostTickers( engine* e, float dt );
void engine_renderRenders( engine* e );
void engine_inputInputs( engine* e );

/*
 *
 *  Test Functions
 *
 */

#ifdef GRAPH_FPS
graph* fpsgraph; 
graphData* fpsdata;
frame_timer* fps_timer;
#endif // GRAPH_FPS

/*
 *
 *  Engine Methods
 *
 */

void engine_input( engine* e ) {
	scene_input( e->_scene, e->_input );
	engine_inputInputs( e );
}

float frame_times[30];

// TODO - move to Lua and abstract
// luaCall "ontick" engine
void lua_tick( engine* e, float dt ) {
	if ( e->onTick && luaCallback_enabled( e->onTick ) ) {
		lua_setActiveState( e->lua );
#if DEBUG_LUA
		printf("Calling engine::onTick handler: %s\n", e->onTick->func);
#endif
    // TODO - abstract into a templated call-lua function
		lua_pushcfunction( e->lua, lua_errorHandler );
		lua_getglobal( e->lua, e->onTick->func );				
		lua_pushnumber( e->lua, dt );
    const int numArgs = 1;
    const int numReturns = 0;
    const int errorHandler = -3;
		int err = lua_pcall( e->lua, numArgs, numReturns, errorHandler);

		if ( err != 0 ) {
			printError( "LUA ERROR: ErrorNum: %d.", err );
			printError( "%s", lua_tostring( e->lua, -1 ));
			lua_stacktrace( e->lua );
			vAssert( 0 );
		}
		lua_setActiveState( NULL );
	}
}

void engine_countFrames(float dt) {
	float time = 0.f;
	for ( int i = 0; i < 29; i++ ) {
		frame_times[i] = frame_times[i+1];
		time += frame_times[i];
	}
	frame_times[29] = dt;
	time += dt;
	time = time / 30.f;
	//printf( "frame time: %.4f, fps: %.2f\n", time, 1.f/time );
}

// tick - process a frame of game update
void engine_tick( engine* e ) {
	++e->frame_counter;
	float real_dt = timer_getDelta( e->timer );
	float dt = e->paused ? 0.f : real_dt;
  engine_countFrames( dt );

	debugdraw_preTick( dt );
	lua_preTick( e->lua, dt );

	input_tick( e->_input, dt );
  lua_tick( e, dt );

	futures_tick( dt ); // TODO replace this when executor is merged

  // TODO - collision move to Brando execution?
	collision_processResults( e->frame_counter, dt );
	// Memory barrier?
	collision_queueWorkerTick( e->frame_counter+1, dt );

	engine_tickTickers( e, dt );

	// This happens last, as transform concatenation needs to take into account every other input
	scene_tick( e->_scene, dt );
	
	engine_tickPostTickers( e, dt );
}

// Handle a key press from the user
void engine_handleKeyPress(engine* e, uchar key, int x, int y) {
	(void)x;
	(void)y;
	// Lua Test
	lua_getglobal(e->lua, "handleKeyPress");
	lua_pushnumber(e->lua, (int)key);
	lua_pcall(e->lua,	/* args */			1,
						/* returns */		1,
						/* error handler */ 0);
	int ret = lua_tonumber(e->lua, -1);
	printf("Lua says %d!\n", ret);

	switch (key) {
		case 27: // Escape key
			engine_terminate(e); // Exit the program
	}
}


// Initialise the Lua subsystem so that it is ready for use
void engine_initLua(engine* e, const char* script, int argc, char** argv) {
	(void)argc;
	(void)argv;
	lua_setGameLuaPath( "SpaceSim/lua/" ); // TODO - instead this is inserted in android lua load
	e->lua = vlua_create( e, script );
}

// Create a new engine
engine* engine_create() {
	//engine* e = (engine*)mem_alloc(sizeof(engine));
	void* eSpace = mem_alloc(sizeof(engine));
	engine* e = new (eSpace) engine(); // placement new
	//memset( e, 0, sizeof( engine ));
  // TODO - move to constructor
	e->timer = (frame_timer*)mem_alloc(sizeof(frame_timer));
	e->callbacks = luaInterface_create();
	e->onTick = luaInterface_addCallback(e->callbacks, "onTick");
	e->_input = input_create( e->timer );
	e->frame_counter = -1;
#ifdef LINUX_X
  e->xwin = { NULL, 0x0, false };
#endif
	return e;
}

Executor& engine::ex() { return *executor; }

// Initialise the engine
void engine_init(engine* e, int argc, char** argv) {
	e->alive = true;
	e->paused = false;

	timer_init(e->timer);

	// *** Init System
	rand_init();
	vfile_systemInit();

	// *** Initialise OpenGL
	// On Android this is done in response to window events : see Android.c
#ifndef ANDROID
	// Start render thread & wait for initialization to finish
	vthread_create( render_renderThreadFunc, (void*)e );
	while ( !render_initialised ) vthread_waitCondition( finished_render );
#endif // ANDROID

	// *** Start up Core Systems
	noise_staticInit();
	particle_init();
	ribbonEmitter_staticInit();
	particle_staticInit();
	//font_init();

	// *** Initialise Lua
	const char* script_file = "SpaceSim/lua/main.lua";
	if ( argc > 1 ) {
		printf( "Setting script file: %s\n", argv[1] );
		script_file = argv[1];
	}
	engine_initLua(e, script_file, argc, argv);
	luaInterface_registerCallback(e->callbacks, "onTick", "tick");

	// *** Canyon
	canyonTerrain_staticInit();

	for ( int i = 0; i < kNumWorkerThreads; ++i ) {
		vthread_create( worker_threadFunc, NULL );
	}

	e->_scene = scene_create( e );
	lua_setScene( e->lua, e->_scene );
  theScene = e->_scene; // TODO remove

#ifdef GRAPH_FPS
#define kMaxFPSFrames 512
	fpsdata = graphData_new( kMaxFPSFrames );
	vector graph_green = Vector( 0.2f, 0.8f, 0.2f, 1.f );
	fpsgraph = graph_new( fpsdata, 100, 100, 480, 240, (float)kMaxFPSFrames, 0.033f, graph_green );
	fps_timer = vtimer_create();
#endif //GRAPH_FPS
}

// Initialises the application
// Static Initialization should happen here
void init(int argc, char** argv) {
	// *** Initialise Memory
	mem_init( argc, argv );
	string_staticInit();
	// Pools
	transform_initPool();
	modelInstance_initPool();

	// *** Init Logging
	// TODO - date/time naming
	log_init( "log.txt" );

	// *** Static Module initialization
	scene_initStatic();
	lisp_init();
	collision_init();
}

// terminateLua - terminates the Lua interpreter
void engine_terminateLua(engine* e) {
	lua_close(e->lua);
}

// terminate - terminates (De-initialises) the engine
void engine_terminate(engine* e) {
	engine_terminateLua(e);
	render_terminate();
	exit(0);
}

void engine_waitForRenderThread() {
	vthread_waitCondition( finished_render );
}

bool shouldRender( engine* e ) {
  (void)e;
#ifdef ANDROID
	return window_main.context != 0;
#else
  return true;
#endif // ANDROID
}

// TODO - refactor this to be per-window?
void engine_render( engine* e ) {
	if ( shouldRender( e )) {
		render( &window_main, e->_scene );
		engine_renderRenders( e );
		skybox_render( NULL );

		// test
		test_marching_draw();

#ifdef GRAPH_FPS
		graph_render( fpsgraph );
#endif // GRAPH_FPS
	}

	// Allow the render thread to start
	vthread_signalCondition( start_render );
}

// TODO - move to OS
#ifdef LINUX_X
void engine_xwindowPollEvents( engine* e ) {
	XEvent event;
	if ( XPending( e->xwin.display ) > 0 ) {
		XNextEvent( e->xwin.display, &event );
		if ( event.type == DestroyNotify ) {
			// This message means we received our own message to destroy the window, so we clean up
			e->xwin.open = false;
		}
		if ( event.type == ClientMessage ) {
			// This message means the user has asked to close the window, so we send the message
			XDestroyWindow( e->xwin.display, e->xwin.window );
		}
		if ( event.type == KeyPress ) {
			input_xKeyPress( event.xkey.keycode );
		}
		if ( event.type == KeyRelease ) {
			input_xKeyRelease( event.xkey.keycode );
		}
	}

	(void)e;
}
#endif // LINUX_X

// TODO - move to OS
#ifdef ANDROID
void engine_androidPollEvents( engine* e ) {       
//	printf( "Polling for android events." );
	// Read all pending events.
	int ident;
	int events;
	struct android_poll_source* source;

	// If not animating, we will block forever waiting for events.
	// If animating, we loop until all events are read, then continue
	// to draw the next frame of animation.
	while (( ident = ALooper_pollAll(0, NULL, &events, (void**)&source )) >= 0) {
		// Process this event.
		if (source != NULL) {
			source->process( e->app, source );
		}

		// Check if we are exiting.
		if ( e->app->destroyRequested != 0 ) {
			e->alive = false;
			return;
		}
	}
}
#endif // ANDROID

bool os_pollEvents( engine* e ) {
#ifdef ANDROID
  engine_androidPollEvents( e );
	return e->active;
#endif 

#ifdef LINUX_X
	engine_xwindowPollEvents( e );
#endif
  return true;
}

// TODO - pull out graph/timer/debug code to simplify this loop
// run - executes the main loop of the engine
void engine_run( engine* e ) {
	// We need to ensure we can run the first time without waiting for render
	vthread_signalCondition( finished_render );
	
	while ( e->alive ) {
    bool active = os_pollEvents( e );
		if ( active ) {
			engine_input( e );
			engine_tick( e );
#ifdef GRAPH_FPS
			float dt = timer_getDelta( fps_timer );
			static int framecount = 0;
			++framecount;
			graphData_append( fpsdata, (float)framecount, dt );
//			printf( "CPU Delta %6.5f\n", dt );
//			printf( "CPU fps %6.5f\n", 1.0/dt );

#endif // GRAPH_FPS
			engine_waitForRenderThread();
#ifdef GRAPH_FPS
			timer_getDelta( fps_timer );
#endif
			engine_render( e );
			e->alive = e->alive && !input_keyPressed( e->_input, KEY_ESC );
		}
		bool window_open = true;
#ifdef LINUX_X
		window_open = e->xwin.open;
#endif // LINUX_X
		e->alive = e->alive && window_open;

		// Make sure we don't have too many tasks outstanding - ideally this should never get this high
		// Prevents an eventual overflow assertion
		/*
		while ( worker_task_count > 400 ) {
			usleep( 2 );
		}
		*/

	}
}

/*
 *
 * TODO - move and reimplement this section - Delegates
 *
 */

void engine_tickDelegateList( delegatelist* d, engine* e, float dt ) {
	while (d != NULL) {
		assert( d->head );	// Should never be NULL heads
		delegate_tick( d->head, dt, e ); // tick the whole of this delegate
		d = d->tail;
	}
}

// Run through all the delegates, ticking each of them
void engine_tickTickers( engine* e, float dt ) {
	//engine_tickDelegateList( e->tickers, e, dt );
  e->_tickers.call(dt, e);
  for (auto& el : e->toStopTick)
    e->_tickers.remove(el.first, el.second);
  e->toStopTick.clear();
}

// Run through all the post-tick delegates, ticking each of them
void engine_tickPostTickers( engine* e, float dt ) {
	//engine_tickDelegateList( e->post_tickers, e, dt );
  e->_postTickers.call(dt, e);
}

void engine_renderRenders( engine* e ) {
	e->_renders.call(e->_scene);
}

// Process all inputs
void engine_inputInputs( engine* e ) {
  e->_inputs.call(e->_input);
}

// Search the list of delegates for one with the matching tick func
// and space to add a new entry
delegate* engine_findDelegate( delegatelist* d, void* func ) {
	while ( d != NULL ) {
		if ( d->head->tick == func && !delegate_isFull(d->head) ) {
			return d->head;
		}
		d = d->tail;
	}
	return NULL;
}

// Add a new delegate to the delegatelist
// (a delegate is a list of entities to all receive the same tick function)
// (the delegatelist contains all the delegates, ie. for each different tick function)
delegate* engine_addDelegate( delegatelist** d, void* func ) {
	delegatelist* dl = *d;
	if ( !*d ) {
		*d = delegatelist_create();
		dl = *d;
	}
	else {
		while ( dl->tail != NULL)
			dl = dl->tail;
		dl->tail = delegatelist_create();
		dl = dl->tail;
	}
	dl->tail = NULL;
	dl->head = delegate_create( func, kDefaultDelegateSize );
	return dl->head;
}

void engine_addTicker( delegatelist** d_list, void* entity, tickfunc tick ) {
	delegate* d = engine_findDelegate( *d_list, (void*)tick );
	if ( !d )
		d = engine_addDelegate( d_list, (void*)tick );
	delegate_add( d, entity);
}
// Look for a delegate of the right type to add this entity too
// If one is not found, create one

void engine_removeDelegateEntry( delegatelist* d, void* entity, void* delegate_func ) {
	delegatelist* d_list = d;
	while ( d_list ) {
		if ( d_list->head->tick == delegate_func ) {
			delegate_remove( d_list->head, entity );
		}
		d_list = d_list->tail;
	}
}

void startTick( engine* e, void* entity, tickfunc tick ) {
  // TODO - ensure ticks can't be removed during tick (e.g. not re-entrant, defer kill messages)
	//engine_addTicker( &e->tickers, entity, tick );
  e->_tickers.add(entity, tick);
}

void startPostTick( engine* e, void* entity, tickfunc tick ) {
	//engine_addTicker( &e->post_tickers, entity, tick );
  e->_postTickers.add(entity, tick);
}

void stopTick( engine* e, void* entity, tickfunc tick ) {
  //e->_tickers.remove(entity, tick);
  e->toStopTick.push_front(make_pair(entity, tick));
}

void stopPostTick( engine* e, void* entity, tickfunc tick ) {
	//engine_removeDelegateEntry( e->post_tickers, entity, (void*)tick );
  e->_postTickers.remove(entity, tick);
}

void startInput( engine* e, void* entity, inputfunc in ) {
	e->_inputs.add<void*>(entity, in);
}

void engine_addRender( engine* e, void* entity, renderfunc render ) {
	e->_renders.add<void*>(entity, render);
}

void engine_removeRender( engine* e, void* entity, renderfunc render ) {
	e->_renders.remove(entity, render);
}

window engine_mainWindow( engine* e ) {
	(void)e;
	return window_main;
}
