// input.c

#include "common.h"
#include "input.h"
//---------------------
#include "engine.h"
#include "maths/maths.h"
#include "test.h"
#include "input/keyboard.h"
#include "mem/allocator.h"

#ifdef LINUX_X
#include <X11/Xlib.h>
#endif // LINUX_X

// *** Static data

int input_bindCount = 0;
int input_keybinds[INPUT_MAX_KEYBINDS];

input* input_create( frame_timer* t ) {
	input* in = (input*)mem_alloc( sizeof( input ));
	memset( in, 0, sizeof( input));
	in->active = 0;
	memset( in->data, 0, sizeof( input_data ) * INPUT_DATA_FRAMES );
	memcpy( in->keybinds, input_keybinds, INPUT_MAX_KEYBINDS ); // init the keybinds to defaults
	in->timer = t;
#ifdef TOUCH
	touchPanel_init( &in->touch );
	touchPanel_init( &in->data[0].touch );
	touchPanel_init( &in->data[1].touch );
#endif // TOUCH
	return in;
}
//
// *** Keybinds
//

int input_registerKeybind( ) {
	return input_bindCount++;
}

// Set a keybind for the given input setup only, overwriting the default
void input_setKeyBind( input* in, keybind bind, int key ) {
	in->keybinds[bind] = key;
}

// Set a default keybind. This will be copied into any input that is created after
void input_setDefaultKeyBind( keybind bind, int key ) {
	input_keybinds[bind] = key;
}

// Keybind varients of the key functions
int input_keybindPressed( input* in, keybind bind ) {
	return input_keyPressed( in, (enum key)in->keybinds[bind] );
}
int input_keybindHeld( input* in, keybind bind ) {
	return input_keyHeld( in, (enum key)in->keybinds[bind] );
}
int input_keybindReleased( input* in, keybind bind ) {
	return input_keyReleased( in, (enum key)in->keybinds[bind] );
}
int input_keybindWasHeld( input* in, keybind bind ) {
	return input_keyWasHeld( in, (enum key)in->keybinds[bind] );
}


int getKeyInternal( int key ) {
	(void)key;
	return 0x0;
}

void input_setWindowSize( input* in, int w, int h ) {
	in->w = w;
	in->h = h;
}

// tick the input, recording this frames input data from devices
void input_tick( input* in, float dt ) {
	(void)dt;
	in->active = in->active ^ 0x1;		// flip the key buffers (we effectively double buffer the key state)

	input_keyboardTick( in, dt );
	input_mouseTick( in, dt );

#ifdef TOUCH
	input_touchTick( in, dt );
#endif
}

#if UNIT_TEST
void test_input() {
	printf( "--- Beginning Unit Test: Input System ---\n" );
	test_keyboard();
}
#endif // UNIT_TEST
