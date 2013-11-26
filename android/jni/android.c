#include <jni.h>
#include <errno.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

// *** Vitae Includes
#include "common.h"
#include "engine.h"
#include "input.h"
#include "particle.h"
#include "mem/allocator.h"
#include "render/render.h"
#include "script/lisp.h"
#include "system/file.h"
#include "system/hash.h"
#include "system/string.h"

// ###################################

#if UNIT_TEST
void runTests() {
	// Memory Tests
	test_allocator();

	test_hash();

	// System Tests
	//test_lisp();

	test_maths();

	test_property();

	test_string();
}
#endif // UNIT_TEST

// Just the current frame in the display.
static void draw_frame( window* w ) {
	printf( "ANDROID: beginning drawing frame." );
    if ( w->display == NULL ) {
        // No display.
        return;
    }

	printf( "ANDROID: drawing frame." );
    // Just fill the screen with a color.
    glClearColor( 0.5f, 1.f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers( w->display, w->surface );
}

void android_exit() {
#ifdef ANDROID_PROFILING
	// Cleanup the profiling library
	moncleanup();
#endif // ANDROID_PROFILING

	exit( EXIT_SUCCESS );
}

void app_exit( struct android_app* app ) {
	// The window is being hidden or closed, clean it up.
	engine* e = app->userData;
	render_destroyWindow( &window_main );
	e->active = false;
	android_exit();
}

// Process the next main command.
static void handle_cmd( struct android_app* app, int32_t cmd ) {
    switch (cmd) {
		/*
        case APP_CMD_SAVE_STATE:
			*/
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
			printf( "ANDROID: init window." );
            if ( app->window != NULL ) {
				printf( "ANDROID: init EGL." );
				vAssert( app->userData );
				engine* e = app->userData;

				// Spawn the renderer thread here
				vthread render_thread = vthread_create( render_renderThreadFunc, (void*)app );
				(void)render_thread;
				// Wait for initialization to finish
				while ( !render_initialised ) {
					sleep( 1 );
				}
				printf( "MAIN THREAD: Renderer has been initialised.\n" );

				// Set the touch window boundaries in the input
				input_setWindowSize( e->input, window_main.width, window_main.height );
				e->active = true;
				//render_init();
                //draw_frame( &window_main );
            }
            break;
        case APP_CMD_TERM_WINDOW:
			{
				printf( "ANDROID: term window." );
				app_exit( app );
				break;
			}
		case APP_CMD_GAINED_FOCUS:
			{
				printf( "ANDROID: gain focus." );
				break;
			}
		case APP_CMD_LOST_FOCUS:
			{
				printf( "ANDROID: lost focus." );
				app_exit( app );
				break;
			}
	}
}

int android_motionAction_getActionCode( int motion_action ) {
	return motion_action & AMOTION_EVENT_ACTION_MASK;
}

int android_motionAction_getPointerIndex( int motion_action ) {
	return ( motion_action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK ) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
}

// Process the next input event.
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    engine* e = app->userData;
	vAssert( e );
	vAssert( e->input );
	// AInputEvent_getSource() == AINPUT_SOURCE_TOUCHSCREEN
    if ( AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION ) {
		int motion_action = AMotionEvent_getAction( event );
		int action_code = android_motionAction_getActionCode( motion_action );
		int x = AMotionEvent_getX( event, 0 );
		int y = AMotionEvent_getY( event, 0 );


		enum touchAction action;
		switch( action_code )
		{
			case AMOTION_EVENT_ACTION_UP:
				action = kTouchUp;
				break;
			case AMOTION_EVENT_ACTION_DOWN:
				action = kTouchDown;
				break;
			case AMOTION_EVENT_ACTION_MOVE:
				action = kTouchMove;		
				break;
			case AMOTION_EVENT_ACTION_POINTER_UP:
				action = kTouchUp;
				break;
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				action = kTouchDown;
				break;
			default:
				action = kTouchUnknown;
				break;
		}
		
		// Differentiate between events for a single pointer and for multiple pointers
		switch ( action_code ) {
			case AMOTION_EVENT_ACTION_POINTER_UP:
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				{
					int i = android_motionAction_getPointerIndex( motion_action );
					int pointer_x = AMotionEvent_getX( event, i );
					int pointer_y = AMotionEvent_getY( event, i );
					int uid = AMotionEvent_getPointerId( event, i );
					input_registerTouch( e->input, uid, pointer_x, pointer_y, action );
					//printf( "Pointer action %d. uid %d.\n", action_code, uid );
				}
				break;
			default:
				{
					int pointer_count = AMotionEvent_getPointerCount( event );
					for ( int i = 0; i < pointer_count; ++i ) {
						int pointer_x = AMotionEvent_getX( event, i );
						int pointer_y = AMotionEvent_getY( event, i );
						int uid = AMotionEvent_getPointerId( event, i );
						input_registerTouch( e->input, uid, pointer_x, pointer_y, action );
					}
				}
				break;
		}
		/*
		if ( action_code == AMOTION_EVENT_ACTION_UP )
			printf( "Touch Action Up\n" );
		if ( action_code == AMOTION_EVENT_ACTION_CANCEL )
			printf( "Touch Action Cancel\n" );

		printf( "touch_event %d\n", action_code );
		*/
	/*	
		for ( int i = 0; i < pointer_count; ++i ) {
			int pointer_x = AMotionEvent_getX( event, i );
			int pointer_y = AMotionEvent_getY( event, i );
			int id = AMotionEvent_getPointerId( event, i );
			printf( "-- pointer index %d, id %d ( %d %d )\n", i, id, pointer_x, pointer_y );
		}
		*/

/*
		int pointer_index = 0;
		if ( action_code == AMOTION_EVENT_ACTION_POINTER_DOWN ||
			 action_code == AMOTION_EVENT_ACTION_POINTER_UP ) {
			pointer_index = android_motionAction_getPointerIndex( motion_action );
		}

		int uid = AMotionEvent_getPointerId( event, pointer_index );
		input_registerTouch( e->input, uid, x, y, action );
		*/

        return 0;
    }
    return 0;
}

void android_init( struct android_app* app ) {
	printf("Loading Vitae.\n");

#ifdef ANDROID_PROFILING
	// Initialise the profiling library
	monstartup("libvitae.so");
#endif // ANDROID_PROFILING

	// *** Initialise Engine
	// already created
	engine* e = engine_create();
	e->app = app;
	e->active = false;
	app->userData = e;
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main( struct android_app* app ) {
    // Make sure glue isn't stripped.
    app_dummy();

    app->onAppCmd = handle_cmd;
	app->onInputEvent = handle_input;
	app->userData = NULL;

	// Static init - includes Memory init
	init( 0, NULL );

	android_init( app );

	printf( "ANDROID: Waiting to init EGL.\n" );

	// We want to wait until we have an EGL context before we kick off
	// This is so that OpenGL can init correctly
	while ( window_main.context == 0 ) {
		//		sleep( 1 );
		int ident;
		int events;
		struct android_poll_source* source;


		while (( ident = ALooper_pollAll(0, NULL, &events, (void**)&source )) >= 0) {
			// Process this event.
			if (source != NULL) {
				source->process( app, source );
			}
		}
	}
	printf( "ANDROID: EGL initialised, away we go!" );

	// Android doesn't have commandline args
	int argc = 0;
	char** argv = NULL;
	// Can't init engine until EGL is initialised
	engine_init( app->userData, argc, argv );

#if UNIT_TEST
	runTests();
#endif

	engine_run( app->userData );
}
