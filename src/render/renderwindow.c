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
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
#ifdef RENDER_OPENGL_ES
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
			EGL_DEPTH_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE
    };

	// Ask for a GLES2 context	
	const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2, 
		EGL_NONE
	};
#endif // OPENGL_ES
#ifdef RENDER_OPENGL
	const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
			EGL_DEPTH_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, // We want OpenGL, not OpenGL_ES
            EGL_NONE
    };
	// Don't need, as we're not using OpenGL_ES
	const EGLint context_attribs[] = {
		EGL_NONE
	};
#endif // RENDER_OPENGL

    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay( defaultOSDisplay() );
	EGLint minor = 0, major = 0;
    EGLBoolean result = eglInitialize( display, &major, &minor );
	vAssert( result == EGL_TRUE );

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
	printf( "EGL Choosing Config.\n" );
    eglChooseConfig( display, attribs, &config, 1, &numConfigs );
	vAssert( result == EGL_TRUE );

	// We need to create a window first, outside EGL
#ifdef ANDROID
    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    EGLint egl_visual_id;
    eglGetConfigAttrib( display, config, EGL_NATIVE_VISUAL_ID, &egl_visual_id );
    ANativeWindow_setBuffersGeometry( ((struct android_app*)app)->window, 0, 0, egl_visual_id );
	EGLNativeWindowType native_win = ((struct android_app*)app)->window;

#endif
#ifdef LINUX_X
	(void)app;
	EGLNativeWindowType native_win = os_createWindow( w, "Skies of Antares" );
#endif

	printf( "EGL Creating Surface.\n" );
    surface = eglCreateWindowSurface( display, config, native_win, NULL );
	if ( surface == EGL_NO_SURFACE ) {
		printf( "Unable to create EGL surface (eglError: %d)\n", eglGetError() );
		vAssert( 0 );
	}
	result = eglBindAPI( RENDER_GL_API );
	vAssert( result == EGL_TRUE );
  
	printf( "EGL Creating Context.\n" );
    context = eglCreateContext(display, config, NULL, context_attribs );

    result = eglMakeCurrent(display, surface, surface, context);
	vAssert( result == EGL_TRUE );

	// Store our EGL params with out render window
	w->display = display;
	w->surface = surface;
	w->context = context;

    eglQuerySurface(display, surface, EGL_WIDTH, &w->width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &w->height);
}


