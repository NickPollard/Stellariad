// ringqueue.h
#pragma once
#define def auto
#include "mem/allocator.h"

typedef struct ringQueue_s {
  int first;
  int last;
  int size;
  void** items;
} ringQueue;

ringQueue* ringQueue_create( int size );
int ringQueue_count( ringQueue* r );
void* ringQueue_pop( ringQueue* r );
void ringQueue_push( ringQueue* r, void* item );

#ifdef __cplusplus
template<typename T> struct RingQ {
	int first;
	int last;
	int size;
	T** items;

	int count() { return last - first + ( last < first ? size : 0 ); }

	T* pop() {
		vAssert( count() > 0 );
		T* item = items[first];
		first = ( first + 1 ) % size;
		return item;
	}

	void push( T* item ) {
		vAssert( count() < size );
		items[last] = item;
		last = ( last + 1 ) % size;
	}

	void operator << ( T* item ) {
		push( item );
	}
};

template<typename T>
RingQ<T>* newRingQ(int size) {
	RingQ<T>* r = (RingQ<T>*)mem_alloc(sizeof(RingQ<body>));
	r->first = 0;
	r->last = 0;
	r->size = size;
	r->items = (T**)mem_alloc( sizeof( void* ) * size );
	return r;
};

#endif
