// input.h
#pragma once

#include "input/keyboard.h"
#include "input/mouse.h"
#include "vtime.h"

// *** General input defines
#define INPUT_DATA_FRAMES 2

#ifdef ANDROID
#define TOUCH
#endif

#ifdef TOUCH
#include "input/touch.h"
#endif // TOUCH

// *** Main input struct

// Keys stored as a packed bitmask
// ie. each byte stores 8 flags for 8 keys respectively
typedef struct input_data_s {
	key_array keys;
	mouse _mouse;
#ifdef TOUCH
	touchPanel touch;
#endif // TOUCH
} input_data;

struct input_s {
	int active;	// Index into the data array, alternates between 0 and 1
	input_data data[INPUT_DATA_FRAMES]; // This frame, last frame - switch on every frame
	int keybinds[INPUT_MAX_KEYBINDS];
#ifdef TOUCH
	touchPanel touch;
#endif // TOUCH
	int w;
	int h;
	frame_timer* timer;
};

// Constructor
input* input_create( frame_timer* t );

// tick the input, recording this frames input data from devices
void input_tick( input* in, float dt );

void input_setWindowSize( input* in, int w, int h );

// *** Unit Test
void test_input();
