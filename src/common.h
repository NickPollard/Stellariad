// Standard C libraries
#pragma once

#ifndef ANDROID
#define LINUX_X // At the moment we only support linux apart from Android
#endif

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "common.fwd.h"

#include "debug/debug.h"
#include "profile.h"

// For sleep
#include <unistd.h>

// For openGL compatibility
#define GL_GLEXT_PROTOTYPES

// Boolean defines
#define true 1
#define false 0

// *** Unit Testing
#define UNIT_TEST true
#define TERM_RED "[1;31;40m"
#define TERM_GREEN "[1;32;40m"
#define TERM_WHITE "[0;37;40m"

// *** Architecture
#define ARCH_64BIT
#ifdef ARCH_64BIT
#define dPTRf "%ld"
#define xPTRf "%lx"
#else
#define dPTRf "%d"
#define xPTRf "%x"
#endif

#define then ?
#define otherwise :

// *** Android specific
#ifdef ANDROID
// *** Logging
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "vitae", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "vitae", __VA_ARGS__))

//#define ANDROID_PROFILING

#define printf( ... )  LOGI( __VA_ARGS__ )

#endif // ANDROID

// *** Asserts

#define vAssert( a )	if ( !(a) ) { \
							printf( "Assert Failed: " #a " (%s:%d)\n", __FILE__, __LINE__ ); \
							assert( (a) ); \
						}
#define vAssertMsg( a, msg )	if ( !(a) ) { \
							printf( "Assert Failed: " #a " (%s:%d)\n", __FILE__, __LINE__ ); \
							printf( "Assert Msg: %s\n", msg ); \
							assert( (a) ); \
						}

// Higher Order macros

#define for_each( array, size, func, ... )  \
	for ( int i = 0; i < size; ++i ) { \
		func( array[i], __VA_ARGS__ ); \
	}



// types
//typedef unsigned int uint;
typedef unsigned int u32;
typedef unsigned char uchar;
typedef unsigned char ubyte;
typedef unsigned char u8;
typedef const char* String;

// *** Asset & Resource Loading

// Printing
#define printError( format, args... ) 	{ \
											printf( "%sError%s: ", TERM_RED, TERM_WHITE ); \
											printf( format, args ); \
											printf( " [File: %s, Line: %d]\n", __FILE__, __LINE__ ); \
										}

// Maximum path length in characters
#define kMaxPath 256
