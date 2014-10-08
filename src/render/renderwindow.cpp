// render_window.c
#include "common.h"
#include "renderwindow.h"
//-----------------------
#include "render/render.h"

#ifdef LINUX_X
#include <render/render_linux.h>
#endif
#ifdef ANDROID
#include <render/render_android.h>
#endif

#ifdef RENDER_OPENGL_ES
	#define kGlType EGL_OPENGL_ES2_BIT
	#define kGlClientVersion	EGL_CONTEXT_CLIENT_VERSION, 2, // Ask for a GLES2 context	
#endif
#ifdef RENDER_OPENGL
	#define kGlType EGL_OPENGL_BIT
	#define kGlClientVersion
#endif

EGLConfig eglConfig( EGLDisplay display, const EGLint* attribs ) {
	EGLConfig config;
	EGLint numConfigs;
    EGLBoolean r = eglChooseConfig( display, attribs, &config, 1, &numConfigs );
	vAssert( r == EGL_TRUE );
	return config;
}

void eglInit( EGLDisplay display ) {
	EGLint minor = 0, major = 0;
    EGLBoolean result = eglInitialize( display, &major, &minor );
	vAssert( result == EGL_TRUE );
}

void eglActivateApi( EGLint api ) {
	EGLBoolean r = eglBindAPI( api );
	vAssert( r == EGL_TRUE );
}

int eglGetWindowWidth( window* w ) {
	int width;
    eglQuerySurface( w->display, w->surface, EGL_WIDTH, &width );
	return width;
}
int eglGetWindowHeight( window* w ) {
	int height;
    eglQuerySurface( w->display, w->surface, EGL_HEIGHT, &height );
	return height;
}

// Tear down the EGL context currently associated with the display.
void render_destroyWindow( window* w ) {
    if ( w->display != EGL_NO_DISPLAY ) {
        eglMakeCurrent( w->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
        if ( w->context != EGL_NO_CONTEXT ) eglDestroyContext( w->display, w->context );
        if ( w->surface != EGL_NO_SURFACE) eglDestroySurface( w->display, w->surface );
        eglTerminate( w->display );
    }
    w->display = EGL_NO_DISPLAY;
    w->context = EGL_NO_CONTEXT;
    w->surface = EGL_NO_SURFACE;
}

EGLNativeDisplayType defaultOSDisplay() {
	IF_LINUX( return XOpenDisplay(NULL); )
	IF_ANDROID( return EGL_DEFAULT_DISPLAY; )
}

void render_createWindow( void* app, window* w ) {
	(void)app;
    /* Initialize OpenGL and EGL
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows */
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
		EGL_DEPTH_SIZE, 8,
		EGL_RENDERABLE_TYPE, kGlType,
        EGL_NONE
    };

	const EGLint context_attribs[] = {
		kGlClientVersion
		EGL_NONE
	};

    w->display = eglGetDisplay( defaultOSDisplay() );
	eglInit( w->display );

   	EGLConfig config = eglConfig( w->display, attribs ); // We are just picking the first config that matches our requirements

	// We need to create a window first, outside EGL
	EGLNativeWindowType native_win = 
		IF_LINUX( os_createWindow( w, "Skies of Antares" ); )
		IF_ANDROID(	os_createWindow( &w->display, &config, (AndroidApp*)app ); )

    w->surface = eglCreateWindowSurface( w->display, config, native_win, NULL );
	if ( w->surface == EGL_NO_SURFACE ) {
		printf( "Unable to create EGL surface (eglError: %d)\n", eglGetError() );
		vAssert( 0 );
	}
	eglActivateApi( kGlApi );
  
    w->context = eglCreateContext( w->display, config, NULL, context_attribs );

    EGLBoolean result = eglMakeCurrent( w->display, w->surface, w->surface, w->context);
	vAssert( result == EGL_TRUE );

	w->width = eglGetWindowWidth( w );
	w->height = eglGetWindowHeight( w );
}


