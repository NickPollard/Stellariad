// mouse.c
#include "common.h"
#include "mouse.h"
//---------------------
#include "input.h"

// *** Mouse Accessors
bool input_mouseHeld( input* in, int button ) {
	return in->data[in->active]._mouse.buttons & (0x1 << button);
}

bool input_mouseWasHeld( input* in, int button ) {
	return in->data[in->active]._mouse.buttons & (0x1 << button);
}

bool input_mousePressed( input* in, int button ) {
	return input_mouseHeld( in, button ) && !input_mouseWasHeld( in, button );
}

bool input_mouseReleased( input* in, int button ) {
	return !input_mouseHeld( in, button ) && input_mouseWasHeld( in, button );
}

void input_mousePosition( input* in, int* x, int* y ) {
	*x = in->data[in->active]._mouse.x;
	*y = in->data[in->active]._mouse.y;
}

void input_mouseMove( input* in, int* x, int* y ) {
	*x = in->data[in->active]._mouse.x - in->data[1 - in->active]._mouse.x;
	*y = in->data[in->active]._mouse.y - in->data[1 - in->active]._mouse.y;
}

void input_mouseDrag( input* in, int button, int* x, int* y ) {
	if ( input_mouseHeld( in, button )) {
		input_mouseMove( in, x, y );
	}
	else {
		*x = 0;
		*y = 0;
	}
}



// *** Get the current hardware state of the mouse
int getMouseButton( int button ) {
	(void)button;
	return 0x0;
}
void getMousePos( int* x, int* y ) {
	*x = 0;
	*y = 0;
}


// Update the mouse system
void input_mouseTick( input* in, float dt ) {
#ifndef ANDROID
	// Store current state of mouse
	in->data[in->active]._mouse.buttons = 0x0;
	for ( int i = 0; i < 2; i++ )
		in->data[in->active]._mouse.buttons |= ( 0x1 & getMouseButton( i )) << i;

	getMousePos( &in->data[in->active]._mouse.x, &in->data[in->active]._mouse.y );
#endif // ANDROID
	(void)in;
	(void)dt;
}
