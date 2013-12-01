//render_android.h
#pragma once
// External
#include "EGL/egl.h"

#define RENDER_OPENGL_ES
#define kGlApi EGL_OPENGL_ES_API
typedef struct android_app AndroidApp;

EGLNativeWindowType os_createwindow( EGLDisplay* display, EGLConfig* config, AndroidApp* app );
