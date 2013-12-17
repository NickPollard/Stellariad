//render_android.c
#include "common.h"
#ifdef ANDROID
#include "render_android.h"
//-----------------------
#include "render/render.h"
#include "EGL/egl.h"

EGLNativeWindowType os_createWindow( EGLDisplay* display, EGLConfig* config, AndroidApp* app ) { 
    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    EGLint eglVisualId;
    eglGetConfigAttrib( display, config, EGL_NATIVE_VISUAL_ID, &eglVisualId );
    ANativeWindow_setBuffersGeometry( app->window, 0, 0, eglVisualId );
	return app->window;
}
#endif // ANDROID
