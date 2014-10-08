// ringqueue.c
#include "common.h"
#include "ringqueue.h"
//---------------------
#include "mem/allocator.h"

ringQueue* ringQueue_create( int size ) {
	ringQueue* r = (ringQueue*)mem_alloc( sizeof( ringQueue ));
	r->size = size;
	r->first = 0;
	r->last = 0;
	r->items = (void**)mem_alloc( sizeof( void* ) * size );
	return r;
}

int ringQueue_count( ringQueue* r ) {
	int count = r->last - r->first + ( r->last < r->first ? r->size : 0 );
	return count;
}

void* ringQueue_pop( ringQueue* r ) {
	vAssert( ringQueue_count( r ) > 0 );
	void* item = r->items[r->first];
	r->first = ( r->first + 1 ) % r->size;

	return item;
}

void ringQueue_push( ringQueue* r, void* item ) {
	vAssert( ringQueue_count( r ) < r->size );
	r->items[r->last] = item;
	r->last = ( r->last + 1 ) % r->size;
}
