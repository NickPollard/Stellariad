// Allocator.c
#include "common.h"
#include "allocator.h"
//---------------------
#include "test.h"
#include "system/thread.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define kGuardValue 0xdeadbeef

#define static_heap_size (128 * 1024 * 1024) // In MegaBytes
heapAllocator* static_heap = NULL;
vmutex allocator_mutex = kMutexInitialiser;

static const char* mem_stack_string = NULL;

void block_recordAlloc( block* b, const char* stack );

// Default allocate from the static heap
// Passes straight through to heap_allocate()
void* mem_alloc( size_t bytes ) {
	return heap_allocate( static_heap, bytes );
}

// Default deallocate from the static heap
// Passes straight through to heap_deallocate()
void mem_free( void* ptr ) {
	heap_deallocate( static_heap, ptr );
}

// Initialise the memory subsystem
void mem_init(int argc, char** argv) {
	(void)argc; (void)argv;
	static_heap = heap_create( static_heap_size );
}

// Allocates *size* bytes from the given heapAllocator *heap*
// Will crash if out of memory
void* heap_allocate( heapAllocator* heap, int size ) {
#ifdef MEM_FORCE_ALIGNED
	return heap_allocate_aligned( heap, size, 4 );
#else
	return heap_allocate_aligned( heap, size, 0 );
#endif
}

// Allocates *size* bytes from the given heapAllocator *heap*
// Will crash if out of memory
// NEEDS TO BE THREADSAFE
void* heap_allocate_aligned( heapAllocator* heap, size_t size, size_t alignment ) {
	vmutex_lock( &allocator_mutex );
#ifdef MEM_DEBUG_VERBOSE
	printf( "HeapAllocator request for " dPTRf " bytes, " dPTRf " byte aligned.\n", size, alignment );
#endif
	size_t size_original = size;
	size += alignment;	// Make sure we have enough space to align
	block* b = heap_findEmptyBlock( heap, size );

	if ( !b ) {
		//heap_dumpBlocks( heap );
		printError( "HeapAllocator out of memory on request for " dPTRf " bytes. Total size: " dPTRf " bytes, Used size: " dPTRf " bytes\n", size, heap->total_size, heap->total_allocated );
		assert( 0 );
	}
	
	vAssert( !b->next || b->next == b->data + b->size );

	if ( b->size > ( size + sizeof( block ) ) ) {
//		heap_dumpBlocks( heap );
		void* new_ptr = ((uint8_t*)b->data) + size;
		block* remaining = block_create( new_ptr, b->size - size );
		block_insertAfter( b, remaining );
		b->size = size;
		heap->total_allocated += sizeof( block );
		heap->total_free -= sizeof( block );
		// Try to merge blocks
		if ( remaining->next && remaining->next->free )
			block_merge( heap, remaining, remaining->next );
	}
	b->free = false;

	vAssert( b->next == b->data + b->size );

	// Move the data pointer on enough to force alignment
	uintptr_t offset = alignment - ((uintptr_t)b->data % alignment);
	b->data = ((uint8_t*)b->data) + offset;
	b->size -= offset;
	// Now move the block on and copy the header, so that it's contiguous with the block
	block* new_block_position = (block*)(((uint8_t*)b) + offset);
	// Force alignment for this so we can memcpy (on Android, even memcpy requires alignments when dealing with structs)
	//static block block_temp __attribute__ ((aligned (8)));
	block block_temp;
	//block block_temp = *b; // This doesn't work on android, alignment issues again?
	memcpy( &block_temp, b, sizeof( block ));
	b = new_block_position;
	*b = block_temp;
	// Fix up pointers to this block, for the new location
	if ( b->prev ) {
		b->prev->next = b;
		// Increment previous block size by what we've moved the block
		b->prev->size += offset;
	}
	else {
		heap->first = b;
	}
	if ( b->next ) {
		b->next->prev = b;
	}


	vAssert( b->next == b->data + b->size );

	heap->total_allocated += size;
	heap->total_free -= size;

	// Ensure we have met our requirements
	uintptr_t align_offset = ((uintptr_t)b->data) % alignment;
	vAssert( align_offset == 0 );	// Correctly Aligned
	vAssert( b->size >= size_original );	// Large enough

#ifdef MEM_DEBUG_VERBOSE
	printf("Allocator returned address: " xPTRf ".\n", (uintptr_t)b->data );
#endif

	vmutex_unlock( &allocator_mutex );
	++heap->allocations;

#ifdef MEM_STACK_TRACE
	block_recordAlloc( b, mem_stack_string );
#endif // MEM_STACK_TRACE

	return b->data;
}

#ifdef MEM_STACK_TRACE
void block_recordAlloc( block* b, const char* stack ) {
	b->stack = stack;
	}
#endif // MEM_STACK_TRACE

void heap_dumpBlocks( heapAllocator* heap ) {
	block* b = heap->first;
	while ( b ) {
#ifdef MEM_STACK_TRACE
		if ( b->stack && !b->free )
			printf( "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\t\tStack: %s\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free, b->stack );
		else
			printf( "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free );
#else // MEM_STACK_TRACE
		printf( "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free );
#endif // MEM_STACK_TRACE
		b = b->next;
	}
}

void heap_dumpUsedBlocks( heapAllocator* heap ) {
	block* b = heap->first;
	while ( b ) {
#ifdef MEM_STACK_TRACE
		if ( b->stack && !b->free )
			printf( "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\t\tStack: %s\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free, b->stack );
#else // MEM_STACK_TRACE
		if ( !b->free )
			printf( "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free );
#endif // MEM_STACK_TRACE
		b = b->next;
	}
}

// Find a block of at least *min_size* bytes
// First version will naively use first found block meeting the criteria
block* heap_findEmptyBlock( heapAllocator* heap, size_t min_size ) {
//	heap_dumpBlocks( heap );
	block* b = heap->first;
	while ((( b->size < min_size ) || !b->free ) && b->next ) {
#ifdef MEM_GUARD_BLOCK
		assert( b->guard == kGuardValue );
#endif
		b = b->next;
	}
	// Re-check in case we ran out without finding one
	if ( !b->free || ( b->size < min_size ) )
		b = NULL;
	return b;
}

// Find a block with a given data pointer to *mem_addr*
// Returns NULL if no such block is found
block* heap_findBlock( heapAllocator* heap, void* mem_addr ) {
	block* b = heap->first;
	while ( (b->data != mem_addr) && b->next ) {
#ifdef MEM_GUARD_BLOCK
		assert( b->guard == kGuardValue );
#endif
	   	b = b->next;
	}
	if ( b->data != mem_addr )
		b = NULL;
	return b;
}

// Release a block from the heapAllocator
void heap_deallocate( heapAllocator* heap, void* data ) {
	vmutex_lock( &allocator_mutex );
	//block* b = heap_findBlock( heap, data );
	block* b = (block*)((uint8_t*)data - sizeof( block ));
#ifdef MEM_GUARD_BLOCK
	vAssert( b->guard == kGuardValue );
#endif // MEM_GUARD_BLOCK
	vAssert( b );
#ifdef MEM_DEBUG_VERBOSE
	printf("Allocator freed address: " xPTRf ".\n", (uintptr_t)b->data );
#endif
	b->free = true;

	heap->total_free += b->size;
	heap->total_allocated -= b->size;

	// Try to merge blocks
	if ( b->next && b->next->free )
		block_merge( heap, b, b->next );
	if ( b->prev && b->prev->free )
		block_merge( heap, b->prev, b );
	vmutex_unlock( &allocator_mutex );

	--heap->allocations;
}

// Merge two continous blocks, *first* and *second*
// Both *first* and *second* must be valid
// Afterwards, only *first* will remain valid
// but will have size equal to both plus sizeof( block )
void block_merge( heapAllocator* heap, block* first, block* second ) {
//	printf( "Allocator: Merging Blocks\n" );

	vAssert( first );
	vAssert( second );
	vAssert( first->free );								// Both must be empty
	vAssert( second == (first->data + first->size) );	// Contiguous
	vAssert( first->next == second );
	if ( second->prev != first ) {
		printf( "Second: 0x" xPTRf ", first: 0x" xPTRf ", second->prev: 0x" xPTRf "\n", (uintptr_t)second, (uintptr_t)first, (uintptr_t)second->prev );
	}
	vAssert( second->prev == first );

	vAssert( !first->next || first->next == first->data + first->size );
	vAssert( !second->next || second->next == second->data + second->size );

	vAssert( second > first );
	vAssert( second->next > second || second->next == NULL );

	heap->total_free += sizeof( block );
	heap->total_allocated -= sizeof( block );

//	int total_size = first->size + second->size;
	// We can't just add sizes, as there may be alignment padding.
//	first->size += second->size + sizeof( block );

//	printf( "first: 0x" xPTRf ", Second: 0x" xPTRf ", Second->next 0x" xPTRf ", Second->data 0x" xPTRf ", Second->size 0x" xPTRf "",		first, second, second->next, second->data, second->size );

//	size_t true_size = (uint8_t*)second->next - (uint8_t*)second;
	size_t true_size = second->size + ( (size_t)second->data - (size_t)second );
//	printf( "first->size: " dPTRf " true_size: " dPTRf "\n", first->size, true_size );
	first->size = first->size + true_size;
	first->next = second->next;
	if ( second->next )
		second->next->prev = first;
	first->free = true;

	vAssert( !first->next || first->next == first->data + first->size );

//	vAssert( first->size < total_size + 128 );	
}

// Create a heapAllocator of *size* bytes
// Initialised with one block pointing to the whole memory
heapAllocator* heap_create( int heap_size ) {
	// We add space for the first block header, so we do get the correct total size
	// ie. this means that heap_create (size), followed by heap_Allocate( size ) should work
	void* data = malloc( sizeof( heapAllocator ) + sizeof( block ) + heap_size );

	heapAllocator* allocator = (heapAllocator*)data;
	data += sizeof( heapAllocator );
	allocator->total_size = heap_size;
	allocator->total_free = heap_size;
	allocator->total_allocated = 0;
	
	// Should not be possible to fail creating the first block header
	block* first = block_create( data, heap_size );
	assert( first ); 
	allocator->first = first;

	// Test write the data to check we have a valid block of mem
#if 0
	for ( uintptr_t i = 0; i < allocator->total_size; i+=1024 ) {
		printf( "ptr: 0x" xPTRf "\n", ((uintptr_t)first->data + i) );
		*(uint8_t*)((uintptr_t)first->data + i) = 0xde;
	}
#endif

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
	if ( after->next )
		after->next->prev = after;
}

// Create and initialise a block in a given piece of memory of *size* bytes
block* block_create( void* data, int size ) {
	block* b = (block*)data;
	b->size = size - sizeof( block );
	b->data = data + sizeof( block );
	b->free = true;
	b->prev = b->next = NULL;
#ifdef MEM_GUARD_BLOCK
	b->guard = kGuardValue;
#endif
	return b;
}

//
// Tests
//

#if UNIT_TEST
void test_allocator() {
	printf( "%s--- Beginning Unit Test: Heap Allocator ---\n", TERM_WHITE );
	heapAllocator* heap = heap_create( 4096 );

	test( heap, "Allocator created successfully.", "Allocator not created." );
	test( heap->total_size == 4096, "Created Allocator of 4096 byte heap.", "Allocator has incorrect size (should be 4096)." );

	void* a = heap_allocate( heap, 2048 );
	memset( a, 0, 2048 );
	test( true, "Allocated 2048 bytes succesfully.", NULL );

	void* b = heap_allocate( heap, 1024 );
	memset( b, 0, 1024 );
	test( true, "Allocated 2048 + 1024 bytes succesfully.", NULL );

	heap_deallocate( heap, a );
	test( true, "Deallocated 2048 bytes succesfully.", NULL );

	heap_deallocate( heap, b );
	test( true, "Deallocated 1024 bytes succesfully.", NULL );

	a = heap_allocate( heap, 3072 );
	memset( a, 0, 3072 );
	test( true, "Allocated 3072 bytes succesfully.", NULL );

	b = heap_allocate( heap, 512 );
	memset( b, 0, 512 );
	test( true, "Allocated 512 bytes succesfully.", NULL );
}
#endif // UNIT_TEST

passthroughAllocator* passthrough_create( heapAllocator* heap ) {
	passthroughAllocator* p = mem_alloc( sizeof( passthroughAllocator ));
	p->heap = heap;
	p->total_allocated = 0;
	p->allocations = 0;
	return p;
	}

void* passthrough_allocate( passthroughAllocator* p, size_t size ) {
	int before = p->heap->total_allocated;
	void* mem = heap_allocate( p->heap, size );
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
	
void mem_pushStackString( const char* string ) {
	vAssert( mem_stack_string == NULL );
	mem_stack_string = string;
	}

void mem_popStackString() {
	vAssert( mem_stack_string != NULL );
	mem_stack_string = NULL;
	}
