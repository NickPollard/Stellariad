//render_linux.h
#pragma once
// External
#include <X11/Xlib.h>
#include "EGL/egl.h"

#define RENDER_OPENGL
#define RENDER_GL_API EGL_OPENGL_API

// Create an OS-native window (in this case, Linux Xwindows)
EGLNativeWindowType os_createWindow();
