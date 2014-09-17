// keyboard.h
#pragma once

/*
	Keyboard input

	Keyboard input goes through several stages. We have concepts of:
	Key - e.g. 'Escape', a conceptual key that is one of the keys we expect to find on the
		keyboard.
	KeyCode - e.g. '0x9', the physical signal we get from the keyboard, which happens to be the
		keycode for Escape on my keyboard under Ubuntu.
	KeyBinding - e.g. 'Exit Game', a control that we want to be accessible from the keyboard which
		the user is free to rebind to a different key if they so choose.

	So, for example if we want to use escape to quit the game, what we actually want is to set up
	an 'Exit Game' keybind, so that the user can keybind if they so want. 
	This keybind is then set by default to the 'Escape' key.
	The input system then interrogates the system to find out what keyCode - e.g. 0x9 - we expect
	from the keyboard if the escape key is pressed. Due to different layouts, languages etc. this
	is not guaranteed to be the same key.

	So From a high level downwards:

	KeyBinding ( 'exit' )
	  |
	  |
	  |
	 Key ( 'escape' )
	  |
	  |
	  |
	KeyCode ( 0x9 )
   */

enum key {
	KEY_ESC,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_SHIFT,
	KEY_SPACE,
	kMaxKeyCodes
};

#define KEY_COUNT 512
#define INPUT_KEYDATA_SIZE KEY_COUNT / sizeof ( char )

// *** keybind defines
typedef int keybind;
#define INPUT_MAX_KEYBINDS 128

typedef struct key_array_s {
	char keys[INPUT_KEYDATA_SIZE];
} key_array;

// *** Keybinds
int input_registerKeybind( );

// Set a keybind for the given input setup only, overwriting the default
void input_setKeyBind( input* in, keybind bind, int key );

// Set a default keybind. This will be copied into any input that is created after
void input_setDefaultKeyBind( keybind bind, int key );

/*
	Held - the key is currently held
	WasHeld - the key was held the last frame
	Pressed - the key is currently held, but was not held last frame (i.e. key down)
	Released - they key was currently held, but is no longer (i.e. key up)
   */
int input_keyHeld( input* i, enum key k );
int input_keyWasHeld( input* i, enum key k );
int input_keyPressed( input* i, enum key k );
int input_keyReleased( input* i, enum key k );

// Keybind varients of the key functions
int input_keybindPressed( input* in, int keybind );
int input_keybindHeld( input* in, int keybind );
int input_keybindReleased( input* in, int keybind );
int input_keybindWasHeld( input* in, int keybind );

#ifdef LINUX_X
void input_initKeyCodes( xwindow* xwin );
#else
void input_initKeyCodes();
#endif // LINUX_X

#ifdef LINUX_X
void input_xKeyPress( int key );
void input_xKeyRelease( int key );
#endif // LINUX_X

void input_keyboardTick( input* in, float dt );

#if UNIT_TEST
void test_keyboard();
#endif // UNIT_TEST
