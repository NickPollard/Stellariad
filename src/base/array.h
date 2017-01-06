// Array.h
#pragma once

/*
	Fixed Array

	An array of Fixed size, containing objects rather than pointers-to-objects.
	i.e. object array[size];

	Objects are Fixed and are never moved once created (though can be destroyed)

	Use a linked list of empty pointers in unused spaces to define which points are valid and which are not

	Initializing is O(n)
	Adding an element is O(1)
	Removing an element is proportional to the fragmentation of the array
   */

#define DECLARE_FIXED_ARRAY( A ) \
typedef struct A##FixedArray_s { \
	A*	array; 					\
	int	size;					\
	A*	first_free;				\
	A*	end;					\
} A##FixedArray;					\
								\
A* A##FixedArray_add( A##FixedArray* array, A object );			\
void A##FixedArray_remove( A##FixedArray* array, A* object ); 	\
A##FixedArray* A##FixedArray_create( int size );					\

#define IMPLEMENT_FIXED_ARRAY( A ) \
A* A##FixedArray_add( A##FixedArray* array, A object ) { \
	/* Assert there is a space; */						\
	vAssert( a->first_free < a->end );					\
														\
	A* next_free = *a->first_free;						\
	*(a->first_free) = object;							\
	A* inserted_position = a->first_free;				\
	a->first_free = next_free;							\
														\
	return inserted_position;							\
}														\
														\
void A##FixedArray_remove( A##FixedArray* array, A* object ) {	\
	vAssert( object >= array->array && object < array->end );	\
																\
	void* previous_free = &array->first_free;					\
	void* next_free = array->first_free;						\
																\
	while ( next_free < object ) {								\
		previous_free = next_free;								\
		next_free = *next_free;									\
	}															\
																\
	/* Find previous and next free, and update them */			\
	*object = next_free;										\
	*previous_free = object;									\
}																\
																\
A##FixedArray* A##FixedArray_create( int size ) {					\
	A##FixedArray* array = mem_alloc( sizeof( A##FixedArray ));	\
	*array = {};												\
	array->size = size;											\
	array->array = mem_alloc( sizeof( A ) * array->size ));		\
	array->end = &array->array[size];							\
	array->first_free = &array->array[0]; /* When created the array is empty, so the first element must be free*/ \
	/* init all spare spaces, they all point to the next (and the last points past the end) */ \
	for ( int i = 0; i < array->size; ++i ) { 					\
		array->array[i] = array->array[i + 1];					\
	}															\
}

#ifdef UNIT_TEST
void test_array();
#endif // UNIT_TEST

// *** Array funcs
#define arrayContains( a, b, c ) array_contains( (void**)(a), (b), (void*)(c) )
bool array_contains( void** array, int count, void* ptr );
#define arrayAdd( a, b, c ) array_add( (void**)(a), (b), (c) )
void array_add( void** array, int* count, void* ptr );
#define arrayRemove( a, b, c ) array_remove( (void**)(a), (b), (c) )
void array_remove( void** array, int* count, void* ptr );
int array_find( void** array, int count, void* ptr );
