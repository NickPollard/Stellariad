#pragma once

typedef struct passthroughAllocator_s {
	heapAllocator* heap;
	size_t total_allocated;	// in bytes, currently allocated
	size_t allocations;
} passthroughAllocator;

// Create a new passthrough allocator using the given HEAP
passthroughAllocator* passthrough_create( heapAllocator* heap );

// Allocate SIZE memory through the passthroughAllocator P
// (The allocation is actually in p->heap)
void* passthrough_allocate( passthroughAllocator* p, size_t size, const char* source );

// Deallocate allocation MEM from passthroughAllocator P
// (The allocation is actually in p->heap)
void passthrough_deallocate( passthroughAllocator* p, void* mem );


