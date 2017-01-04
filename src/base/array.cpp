// Array.c
#include "common.h"
#include "array.h"
//-------------------------
#include "test.h"

/*
#ifdef UNIT_TEST
typedef struct arrayTestStruct_s {
	int a;
	int b;
	float c;
} arrayTestStruct;

DECLARE_FIXED_ARRAY( arrayTestStruct );
IMPLEMENT_FIXED_ARRAY( arrayTestStruct );

void test_array() {
	int size = 64;
	arrayTestStructFixedArray* array = arrayTestStructFixedArray_create( size );
	test( array->size == size, "Created fixedArray of correct size.", "Created fixedArray of incorrect size." );
	for ( int i = 0; i < 64; ++i ) {
		arrayTestStruct test_object = { 1, 2, 3.f };
		arrayTestStructFixedArray_add( array, test_object );
	}
	test( array->first_free == array->end );
}
#endif // UNIT_TEST
*/

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
