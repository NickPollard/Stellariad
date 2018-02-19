// Array.c
#include "common.h"
#include "array.h"
//-------------------------
#include "test.h"

// TODO - remove these

int array_find( void** array, int count, void* ptr ) {
	for ( int i = 0; i < count; ++i ) {
		if ( array[i] == ptr )
			return i;
	}
	return -1;
}

void array_add( void** array, int* count, void* ptr ) {
	array[(*count)++] = ptr;
}

void array_remove( void** array, int* count, void* ptr ) {
	int i = array_find( array, *count, ptr );
	if ( i != -1 ) {
		--(*count);
		array[i] = array[*count];
		array[*count] = NULL;
	}
}

bool array_contains( void** array, int count, void* ptr ) {
	int foundAt = array_find( array, count, ptr );
  return foundAt != -1;
}
