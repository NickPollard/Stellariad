// Standard C libraries
#pragma once

#ifndef ANDROID
#define LINUX_X // At the moment we only support linux apart from Android
#define IF_LINUX(src) src
#define IF_ANDROID(src)
#endif

#ifdef ANDROID
#define IF_LINUX(src)
#define IF_ANDROID(src) src
#endif

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "lua_debug.h" // For Lua Debugging

#include "common.fwd.h"

#include "debug/vdebug.h"
#include "profile.h"

// For sleep
#include <unistd.h>

// For openGL compatibility
#define GL_GLEXT_PROTOTYPES

// Boolean defines
#ifndef __cplusplus
#define true 1
#define false 0
#endif

#ifdef __cplusplus
#define restrict __restrict__
#endif

// Nullptr
#define nullptr NULL

// *** Unit Testing
#ifdef ANDROID
#define UNIT_TEST false
#else
#define UNIT_TEST true
#endif
#define TERM_RED "[1;31;40m"
#define TERM_GREEN "[1;32;40m"
#define TERM_WHITE "[0;37;40m"

// *** Architecture
//#define ARCH_64BIT
#ifdef ARCH_64BIT
#define dPTRf "%ld"
#define xPTRf "%lx"
#else
#define dPTRf "%d"
#define xPTRf "%x"
#endif

//#define then ?
//#define otherwise :

// *** Units
#define KILOBYTES 1024
#define MEGABYTES 1024*KILOBYTES

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
							printf( "%sAssert Failed: %s", TERM_RED, TERM_WHITE ); \
							printf( #a " (%s:%d)\n", __FILE__, __LINE__ ); \
							lua_activeStateStack( ); \
							assert( (a) ); \
						}
#define vAssertMsg( a, msg )	if ( !(a) ) { \
							printf( "%sAssert Failed: %s: ", TERM_RED, TERM_WHITE ); \
							printf( #a " (%s:%d)\n", __FILE__, __LINE__ ); \
							printf( "Assert Msg: %s\n", msg ); \
							assert( (a) ); \
						}

#define NYI vAssert(0);

// Higher Order macros

//#define for_each( array, size, func, ... )
	//for ( int i = 0; i < size; ++i ) {
		//func( array[i], __VA_ARGS__ );
	//}



#define stackArray( type, size )	alloca( sizeof( type ) * size );

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
											printf( " (%s:%d)\n", __FILE__, __LINE__ ); \
										}

// Maximum path length in characters
#define kMaxPath 256
