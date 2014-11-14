// pool.h

// An object pool, for fixed size objects

#pragma once

#include "mem/allocator.h"

// Implementation Macro
// ( Place in a .h file )

#define DECLARE_POOL( type )				\
typedef struct pool_##type##_s {			\
	int				size;					\
	bool*			free;					\
	type*	pool;							\
} pool_##type;								\
											\
pool_##type* pool_##type##_create( int size );			\
type* pool_##type##_allocate( pool_##type* pool );		\
void pool_##type##_free( pool_##type* pool, type* m );

// Implementation Macro
// ( Place in a .c file )

// TODO - maintain a free-list for O(1) allocations over O(n)

#define IMPLEMENT_POOL( type )											\
																		\
pool_##type* pool_##type##_create( int size ) {							\
	void* data = mem_alloc( sizeof( pool_##type ) + 					\
							sizeof( bool ) * size +						\
							sizeof( type ) * size );					\
	pool_##type* p = (pool_##type*)data;												\
	p->size = size;														\
	p->free = (bool*)((uint8_t*)data + sizeof( pool_##type ));					\
	p->pool = (type*)((uint8_t*)data + sizeof( pool_##type ) + sizeof( bool ) * size);		\
	memset( p->free, 1, sizeof( bool ) * size );						\
	return p;															\
}																		\
type* pool_##type##_allocate( pool_##type* pool ) {						\
	for ( int i = 0; i < pool->size; i++) {								\
		if ( pool->free[i] ) {											\
			pool->free[i] = false;										\
			return &pool->pool[i];										\
		}																\
	}																	\
	printf( "Pool is full; cannot allocate new object.\n" );			\
	assert( 0 );														\
	return NULL;														\
}																		\
void pool_##type##_free( pool_##type* pool, type* m ) {					\
	int index = m - pool->pool;											\
	assert( index >= 0 );												\
	assert( index < pool->size );										\
	pool->free[index] = true;											\
}
