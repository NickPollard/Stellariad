// debug.c
#include "common.h"
#include "debug.h"

#include "mem/allocator.h"

heapAllocator* debug_string_pool;

//
// Initialise the static debug module
void debug_init() {
	debug_string_pool = heap_create( DEBUG_STRING_POOL_SIZE );
}

// Allocate a debug string in the debug_string_pool
// Copy the contents of *string* into this string
const char* debug_string( const char* string ) {
	assert( debug_string_pool );
	int len = strlen( string );
	assert( len < maxDebugStringLength );
	char* d_string = heap_allocate( debug_string_pool, len+1 ); 
	strncpy( d_string, string, len+1 );
	return d_string;
}
