// Allocator.c
#include "allocator.h"
#include "src/common.h"
//---------------------
#include <assert.h>
#include <stdio.h>

#define static_heap_size (64 << 20) // In MegaBytes
heapAllocator* static_heap = NULL;

// Default allocate from the static heap
// Passes straight through to heap_allocate()
void* mem_alloc(size_t bytes) {
	return heap_allocate(static_heap, bytes);
}

// Initialise the memory subsystem
void mem_init(int argc, char** argv) {
	static_heap = heap_create(static_heap_size);
}

// Allocates *size* bytes from the given heapAllocator *heap*
// Will crash if out of memory
void* heap_allocate( heapAllocator* heap, int size ) {
#ifdef MEM_DEBUG_VERBOSE
	printf( "HeapAllocator request for %d bytes.\n", size );
#endif
	block* b = heap_findEmptyBlock( heap, size );
	if ( !b )
		printError( "HeapAllocator out of memory on request for %d bytes.\n", size );
	if ( b->size > ( size + sizeof( block ) ) ) {
		block* remaining = block_create( b->data + size, b->size - size );
		block_insertAfter( b, remaining );
		b->size = size;
		// Try to merge blocks
		if ( remaining->next && remaining->next->free )
			block_merge( remaining, remaining->next );
	}
	b->free = false;
	return b->data;
}

// Find a block of at least *min_size* bytes
// First version will naively use first found block meeting the criteria
block* heap_findEmptyBlock( heapAllocator* heap, int min_size ) {
	block* b = heap->first;
	while ( ( ( b->size < min_size ) || !b->free ) && b->next )
		b = b->next;
	// Re-check in case we ran out without finding one
	if ( !b->free || ( b->size < min_size ) )
		b = NULL;
	return b;
}

// Find a block with a given data pointer to *mem_addr*
// Returns NULL if no such block is found
block* heap_findBlock( heapAllocator* heap, void* mem_addr ) {
	block* b = heap->first;
	while ( (b->data != mem_addr) && b->next ) b = b->next;
	if ( b->data != mem_addr ) b = NULL;
	return b;
}

// Release a block from the heapAllocator
void heap_deallocate( heapAllocator* heap, void* data ) {
	block* b = heap_findBlock( heap, data );
	assert( b );
	b->free = true;
	// Try to merge blocks
	if ( b->next->free )
		block_merge( b, b->next );
	if ( b->prev )
		block_merge( b->prev, b );
}

// Merge two continous blocks, *first* and *second*
// Both *first* and *second* must be valid
// Afterwards, only *first* will remain valid
// but will have size equal to both plus sizeof( block )
void block_merge( block* first, block* second ) {
	assert( first );
	assert( second );
	assert( second->free );								// Second must be empty (not necessarily first!)
	assert( second == (first->data + first->size) );	// Contiguous

	first->size += second->size + sizeof( block );
	first->next = second->next;
}

// Create a heapAllocator of *size* bytes
// Initialised with one block pointing to the whole memory
heapAllocator* heap_create( int heap_size ) {
	void* data = malloc( sizeof( heapAllocator ) + heap_size );

	heapAllocator* allocator = (heapAllocator*)data;
	data += sizeof( heapAllocator );
	
	// Should not be possible to fail creating the first block header
	block* first = block_create( data, heap_size );
	assert( first ); 
	allocator->first = first;

	return allocator;
}

// Insert a block *after* into a linked-list after the block *before*
// Both *before* and *after* must be valid
void block_insertAfter( block* before, block* after ) {
	assert( before );
	assert( after );
	after->next = before->next;
	after->prev = before;
	before->next = after;
}

// Create and initialise a block in a given piece of memory of *size* bytes
block* block_create( void* data, int size ) {
	block* b = (block*)data;
	b->size = size - sizeof( block );
	b->data = data + sizeof( block );
	b->free = true;
	b->prev = b->next = NULL;
	return b;
}

//
// Tests
//

void test_allocator() {
	printf( "%s--- Beginning Unit Test: Heap Allocator ---\n", TERM_WHITE );
	heapAllocator* heap = heap_create( 4096 );

	if (!heap) {
		printf("Test: Failed: Allocator not created.\n");
	} else {
		printf("Test: %sPassed%s: Allocator created succesfully.\n", TERM_GREEN, TERM_WHITE );
	}
//	if (heap->total_size == 4096) {
//		printf("Test: Passed: Create Allocator of 4096 byte heap.\n");
//	} else {
//		printf("Test: Failed: Allocator has incorrect size (should be 4096).\n");
//	}

	void* a = heap_allocate( heap, 2048 );
	memset( a, 0, 2048 );
	printf( "Test: %sPassed%s: Allocated 2048 bytes succesfully.\n", TERM_GREEN, TERM_WHITE );

	void* b = heap_allocate( heap, 1024 );
	memset( b, 0, 1024 );
	printf( "Test: %sPassed%s: Allocated 2048 + 1024 bytes succesfully.\n", TERM_GREEN, TERM_WHITE );

	heap_deallocate( heap, a );
	printf( "Test: %sPassed%s: Deallocated 2048 bytes succesfully.\n", TERM_GREEN, TERM_WHITE );

	heap_deallocate( heap, b );
	printf( "Test: %sPassed%s: Deallocated 1024 bytes succesfully.\n", TERM_GREEN, TERM_WHITE );

	a = heap_allocate( heap, 3072 );
	memset( a, 0, 3072 );
	printf( "Test: %sPassed%s: Allocated 3072 bytes succesfully.\n", TERM_GREEN, TERM_WHITE );

	b = heap_allocate( heap, 512 );
	memset( b, 0, 512 );
	printf( "Test: %sPassed%s: Allocated 512 bytes succesfully.\n", TERM_GREEN, TERM_WHITE );

//	printf("Test: Passed: Allocate 2048 bytes from 4096 heap.\n");
//	if (heap->total_allocated == 2048) && (heap->total_free) == 
}