//render_linux.h
#pragma once
// External
#include <X11/Xlib.h>
#include "EGL/egl.h"

//#define RENDER_OPENGL
//#define kGlApi EGL_OPENGL_API
#define RENDER_OPENGL_ES
#define kGlApi EGL_OPENGL_ES_API

// Create an OS-native window (in this case, Linux Xwindows)
EGLNativeWindowType os_createWindow( window* w, const char* title );
