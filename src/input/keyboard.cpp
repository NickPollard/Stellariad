// keyboard.c
#include "common.h"
#include "keyboard.h"
//---------------------
#include "engine.h"
#include "input.h"
#include "test.h"

#ifdef LINUX_X
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif // LINUX_X

//
// *** keyboard
//

int key_codes[kMaxKeyCodes];

#ifdef LINUX_X
key_array x_key_array;
#endif

// *** Internal private functions, where they key has been translated into a keycode
int keyCode( enum key k ) {
	if ( k >= kMaxKeyCodes )
		printf( "ERROR: Requesting keycode %d\n", k );
	vAssert( k < kMaxKeyCodes );
	return key_codes[k];
}

int keyHeldInternal( input* in, int key_code ) {
	return in->data[in->active].keys.keys[key_code / 8] & (0x1 << (key_code % 8));
}
int keyWasHeldInternal( input* in, int key_code ) {
	return in->data[1 - in->active].keys.keys[key_code / 8] & (0x1 << (key_code % 8));
}

// *** External keyboard accessors

// Is the key held down this frame? Regardless of previous state
int input_keyHeld( input* in, enum key k ) {
	return keyHeldInternal( in, keyCode( k ));
}

// Was the key held down last frame? Regardless of previous state
int input_keyWasHeld( input* in, enum key k ) {
	return keyWasHeldInternal( in, keyCode( k ));
}

// Was the key first pressed this frame? ie. It is depressed now, but was not last frame
int input_keyPressed( input* in, enum key k ) {
	int key_code = keyCode( k );
	return keyHeldInternal( in, key_code ) && !keyWasHeldInternal( in, key_code );
}

// Was the key first released this frame? ie. It is not depressed now, but was last frame
int input_keyReleased( input* in, enum key k ) {
	int key_code = keyCode( k );
	return !keyHeldInternal( in, key_code) && keyWasHeldInternal( in, key_code );
}

void keyArray_set( key_array* keys, int key, int state ) {
	// only two permissible options for state
	vAssert( state == 0x0 || state == 0x1 );
	if ( state )
		keys->keys[ key / 8 ] |= ( 0x1 << ( key % 8 ));
	else
		keys->keys[ key / 8 ] &= ~( 0x1 << ( key % 8 ));

}

int keyArray_get( key_array* keys, int key ) {
	return (keys->keys[ key / 8 ] & ( 0x1 << ( key % 8 ))) != 0;
}

#ifdef LINUX_X
void input_xKeyPress( int key ) {
	keyArray_set( &x_key_array, key, 1);
}
void input_xKeyRelease( int key ) {
	keyArray_set( &x_key_array, key, 0);
}
#endif // LINUX_X

void input_keyboardTick( input* in, float dt ) {
#ifdef LINUX_X
	memcpy( &in->data[in->active].keys, &x_key_array, sizeof( key_array ));
#endif // LINUX_X
	(void)in;
	(void)dt;
}

#ifdef LINUX_X
void input_initKeyCodes( xwindow* xwin ) {
	key_codes[KEY_ESC] = XKeysymToKeycode( xwin->display, XK_Escape );
	key_codes[KEY_UP] = XKeysymToKeycode( xwin->display, XK_Up );
	key_codes[KEY_DOWN] = XKeysymToKeycode( xwin->display, XK_Down );
	key_codes[KEY_LEFT] = XKeysymToKeycode( xwin->display, XK_Left );
	key_codes[KEY_RIGHT] = XKeysymToKeycode( xwin->display, XK_Right );
	
	key_codes[KEY_A] = XKeysymToKeycode( xwin->display, XK_A );
	key_codes[KEY_B] = XKeysymToKeycode( xwin->display, XK_B );
	key_codes[KEY_C] = XKeysymToKeycode( xwin->display, XK_C );
	key_codes[KEY_D] = XKeysymToKeycode( xwin->display, XK_D );
	key_codes[KEY_E] = XKeysymToKeycode( xwin->display, XK_E );
	key_codes[KEY_L] = XKeysymToKeycode( xwin->display, XK_L );
	key_codes[KEY_P] = XKeysymToKeycode( xwin->display, XK_P );
	key_codes[KEY_Q] = XKeysymToKeycode( xwin->display, XK_Q );
	key_codes[KEY_S] = XKeysymToKeycode( xwin->display, XK_S );
	key_codes[KEY_T] = XKeysymToKeycode( xwin->display, XK_T );
	key_codes[KEY_W] = XKeysymToKeycode( xwin->display, XK_W );
	key_codes[KEY_SHIFT] = XKeysymToKeycode( xwin->display, XK_Shift_L );
	
	
	key_codes[KEY_SPACE] = XKeysymToKeycode( xwin->display, XK_space );
}
#else
void input_initKeyCodes() {
}
#endif // LINUX_X

#if UNIT_TEST
void test_keyboard() {
	// Key array test
	key_array data;
	memset( &data, 0, sizeof( key_array ));

	// Test setting key arrays
	for ( int key = 0; key < 256; ++key ) {
		vAssert( keyArray_get( &data, key ) == 0 );
		keyArray_set( &data, key, 1 );
		vAssert( keyArray_get( &data, key ) == 1)
		keyArray_set( &data, key, 0 );
		vAssert( keyArray_get( &data, key ) == 0);
	}
	test( true, "Read and Wrote to all keys in key array successfully.", NULL );
}
#endif // UNIT_TEST
