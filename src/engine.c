// engine.c
#include "src/common.h"
#include "src/engine.h"
//---------------------
#include "mem/allocator.h"
#include "src/font.h"
#include "src/input.h"
#include "src/maths.h"
#include "src/model.h"
#include "src/scene.h"
#include "src/ticker.h"
#include "src/transform.h"
#include "src/render/debugdraw.h"
#include "src/render/render.h"

// Lua Libraries
#include <lauxlib.h>
#include <lualib.h>

// GLFW Libraries
#include <GL/glfw.h>

// System libraries
#include <sys/time.h>

// *** Static Hacks
scene* theScene = NULL;
engine* static_engine_hack;
int w = 640, h = 480;

float camX = 0.f;
float camY = 0.f;

/*
 *
 *  Test Functions
 *
 */

void test_engine_tick(engine* e, float dt) {
//	scene_tick(theScene, dt);
}

void test_engine_init() {
	theScene = scene_createScene();
	test_scene_init(theScene);
}

/*
 *
 *  Engine Methods
 *
 */

// tick - process a frame of game update
void engine_tick( engine* e ) {
	float dt = timer_getDelta( e->timer );

	// TEST
	test_engine_tick( e, dt );
	// end TEST

	input_tick( e->input, dt );
	scene_tick( theScene, dt );

	if ( e->onTick && luaCallback_enabled( e->onTick ) ) {
//		printf("Calling engine::onTick handler: %s\n", e->onTick->func);
		LUA_CALL( e->lua, e->onTick->func );
	}

}

// Static wrapper
void handleKeyPress(uchar key, int x, int y) {
	engine_handleKeyPress(static_engine_hack, key, x, y);
}

// Handle a key press from the user
void engine_handleKeyPress(engine* e, uchar key, int x, int y) {
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

// Handle a window resize
// Set the camera perspective and tell OpenGL how to convert 
// from coordinates to pixel values
void handleResize(int w, int h) {
	/*
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
	*/
	render_set3D( w, h );
}

float angle = 340.f;

float depth = 8.f;

// Initialise the Lua subsystem so that it is ready for use
void engine_initLua(engine* e, int argc, char** argv) {
	e->lua = lua_open();
	luaL_openlibs(e->lua);	// Load the Lua libs into our lua state
	if (luaL_loadfile(e->lua, "src/lua/main.lua") || lua_pcall(e->lua, 0, 0, 0))
		printf("Error loading lua!\n");

	lua_registerFunction(e->lua, LUA_registerCallback, "registerEventHandler");

	LUA_CALL(e->lua, "init");
}

// Create a new engine
engine* engine_create() {
	engine* e = mem_alloc(sizeof(engine));
	e->timer = mem_alloc(sizeof(frame_timer));
	e->callbacks = luaInterface_create();
	e->onTick = luaInterface_addCallback(e->callbacks, "onTick");
	e->input = input_create();
	return e;
}

// Initialise the engine
void engine_init(engine* e, int argc, char** argv) {

	timer_init(e->timer);
	

	// *** Initialise OpenGL
	render_init(e, argc, argv);

	// *** Start up Core Systems
	font_init();

	// *** Initialise Lua
	engine_initLua(e, argc, argv);
	luaInterface_registerCallback(e->callbacks, "onTick", "tick");

	// TEST
	test_engine_init();
}

// Initialises the application
void init(int argc, char** argv) {

	// *** Initialise Memory
	mem_init(argc, argv);

	// *** Initialise Engine
	engine* e = engine_create();
	engine_init(e, argc, argv);
	static_engine_hack = e;

}

// terminateLua - terminates the Lua interpreter
void engine_terminateLua(engine* e) {
	lua_close(e->lua);
}

// terminate - terminates (De-initialises) the engine
void engine_terminate(engine* e) {
	// *** clean up Lua
	engine_terminateLua(e);

	// *** clean up Renderer
	render_terminate();

	exit(0);
}

// static_run
void run() {
	engine_run( static_engine_hack );
}

// run - executes the main loop of the engine
void engine_run(engine* e) {
//	TextureLibrary* textures = texture_library_create();
	int running = true;
	handleResize(640, 480);	// Call once to init
	while (running) {
		engine_tick(e);
		render_set3D( w, h );
		render( theScene );

		running = !input_keyPressed(e->input, KEY_ESC) && glfwGetWindowParam(GLFW_OPENED);

		if (input_keyHeld( e->input, KEY_UP)) {
			depth += 0.01f;
			camY += 0.01f;
		}
		if (input_keyHeld( e->input, KEY_DOWN)) {
			depth -= 0.01f;
			camY -= 0.01f;
		}
		if (input_keyHeld( e->input, KEY_LEFT)) {
			camX -= 0.01f;
		}
		if (input_keyHeld( e->input, KEY_RIGHT)) {
			camX += 0.01f;
		}

		static int mouseX = 0;
		static int mouseY = 0;
		int x, y;
		float mouseScale = 0.01f;

		glfwGetMousePos(&x, &y);
		if ( glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) ) {
			camX -= (float)(x - mouseX) * mouseScale;	
			camY += (float)(y - mouseY) * mouseScale;	
		}
		mouseX = x;
		mouseY = y;

		scene_setCamera(theScene, camX, camY, 10.f, 1.f);
	}
}

