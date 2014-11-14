// Passthrough.c
#include "common.h"
#include "passthrough.h"
//---------------------
#include "mem/allocator.h"

passthroughAllocator* passthrough_create( heapAllocator* heap ) {
	passthroughAllocator* p = (passthroughAllocator*)mem_alloc( sizeof( passthroughAllocator ));
	p->heap = heap;
	p->total_allocated = 0;
	p->allocations = 0;
	return p;
	}

void* passthrough_allocate( passthroughAllocator* p, size_t size, const char* source ) {
	int before = p->heap->total_allocated;
	void* mem = heap_allocate( p->heap, size, source );
	int after = p->heap->total_allocated;
	int delta = after - before;
	p->total_allocated = (size_t)((int)p->total_allocated + delta);	
	++p->allocations;
	return mem;
	}
	
void passthrough_deallocate( passthroughAllocator* p, void* mem ) {
	int before = p->heap->total_allocated;
	heap_deallocate( p->heap, mem );
	int after = p->heap->total_allocated;
	int delta = after - before;
	p->total_allocated = (size_t)((int)p->total_allocated + delta);	
	--p->allocations;
	}
	
