// Array.c
#include "common.h"
#include "array.h"
//-------------------------
#include "test.h"


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
