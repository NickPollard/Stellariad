//render_linux.c
#include "common.h"
#include "render_linux.h"
//-----------------------
#include "input/keyboard.h"
#ifdef LINUX_X
#include "render/render.h"

EGLNativeWindowType os_createWindow( window* w, const char* title ) {
	Display* display = XOpenDisplay(NULL); // Get the XServer display

	int x = 0, y = 0, border_width = 0;
	int white_color = XWhitePixel( display, 0 );
	int black_color = XBlackPixel( display, 0 );

	// Create the window
	Window window = XCreateSimpleWindow( display, DefaultRootWindow( display ), 
			x, y, 
			w->width, w->height,
		   	border_width, 
			black_color, black_color );

	// We want to get MapNotify events
	XSelectInput( display, window, ButtonPressMask|KeyPressMask|KeyReleaseMask|KeymapStateMask|StructureNotifyMask );

	// Setup client messaging to receive a client delete message
	Atom wm_delete=XInternAtom( display, "WM_DELETE_WINDOW", true );
	XSetWMProtocols( display, window, &wm_delete, 1 );

	GC gc = XCreateGC( display, window, 0, NULL );
	XSetForeground( display, gc, white_color );

	XStoreName( display, window, title );
	XMapWindow( display, window );

	// Wait for the MapNotify event
	while (true) {
		XEvent e;
		XNextEvent( display, &e );
		if ( e.type == MapNotify ) break;
	}

	xwindow_main.display = display;
	xwindow_main.window = window;
	xwindow_main.open = true;

	// TODO - this shouldn't happen here.
	// Window creation needs to be moved out of Render
	input_initKeyCodes( &xwindow_main );

	return window;
}

#endif // LINUX_S
