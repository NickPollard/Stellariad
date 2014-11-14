// vdebug.h
#pragma once

// ***
// Debug Settings
// ***

#ifdef DEBUG
#define DEBUG_STRINGS 1
#else
#define DEBUG_STRINGS 0
#endif

#define maxDebugStringLength 64


#define DEBUG_STRING_POOL_SIZE 4096

extern heapAllocator* debug_string_pool;

//
// Initialise the static debug module
void debug_init();

// Allocate a debug string in the debug_string_pool
// Copy the contents of *string* into this string
const char* debug_string( const char* string );
