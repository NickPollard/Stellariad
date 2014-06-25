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

void validateFreeList( heapAllocator* heap );
int countFree( heapAllocator* heap );
int printFree( heapAllocator* heap );

#define static_heap_size (480*MEGABYTES)
heapAllocator* static_heap = NULL;
vmutex allocator_mutex = kMutexInitialiser;
#define kMaxAlignmentSpace 8

static const char* mem_stack_string = NULL;

void block_recordAlloc( block* b, const char* stack );

// Default allocate from the static heap
// Passes straight through to heap_allocate()
#ifdef TRACK_ALLOCATIONS
void* mem_alloc_( size_t bytes, const char* func, const char* file, int line ) {
	char buffer[MemSourceLength];
	snprintf( buffer, MemSourceLength, "%s(%s:%d)", func, file, line );
	buffer[MemSourceLength - 1] = '\0';
	return heap_allocate( static_heap, bytes, buffer );
}
#else
void* mem_alloc( size_t bytes ) {
	return heap_allocate( static_heap, bytes, NULL );
}
#endif

// Default deallocate from the static heap
// Passes straight through to heap_deallocate()
void mem_free( void* ptr ) {
	heap_deallocate( static_heap, ptr );
}

// Initialise the memory subsystem
void mem_init(int argc, char** argv) {
	(void)argc; (void)argv;
	static_heap = heap_create( static_heap_size );
	// Start with two simple bitpools
	heap_addBitpool( static_heap, 16, 16384 );
	heap_addBitpool( static_heap, 64, 16384 );
}

void heap_addBitpool( heapAllocator* h, size_t size, size_t count ) {
	size_t arena_size = size * count;
	void* arena = heap_allocate( h, arena_size, NULL );
	h->bitpools[h->bitpool_count++] = bitpool_create( size, count, arena );
}

// Find the smallest bitpool big enough to hold SIZE
bitpool* heap_findBitpool( heapAllocator* h, size_t size ) {
	bitpool* b = NULL;
	for ( int i = 0; i < h->bitpool_count; ++i ) {
		size_t block_size = h->bitpools[i].block_size;
		if ( block_size > size &&
				( !b || block_size < b->block_size )) {
			b = &h->bitpools[i];
		}
	}
	return b;
}

bitpool* heap_findBitpoolForData( heapAllocator* h, void* data ) {
	bitpool* b = NULL;
	for ( int i = 0; i < h->bitpool_count && !b; ++i ) {
		if ( bitpool_contains( &h->bitpools[i], data )) {
			b = &h->bitpools[i];
		}
	}
	return b;
}

// Allocates *size* bytes from the given heapAllocator *heap*
// Will crash if out of memory
void* heap_allocate( heapAllocator* heap, int size, const char* source ) {
#ifdef MEM_FORCE_ALIGNED
	return heap_allocate_aligned( heap, size, 4, source );
#else
	return heap_allocate_aligned( heap, size, 0, source );
#endif
}

block* nextFree( block* b ) { vAssert( b->free ); return b->nextFree; }
block* prevFree( block* b ) { vAssert( b->free ); return b->prevFree; }
void setPrevFree( block* b, block* prev ) { if (b) b->prevFree = prev; }
void setNextFree( block* b, block* next ) { if (b) b->nextFree = next; }

void addToFreeList( heapAllocator* heap, block* b ) {
	b->free = true;

	block* oldFree = heap->free;
	vAssert( oldFree != b );
	setNextFree( b, oldFree );
	vAssert( oldFree != b );
	vAssert( oldFree == NULL || prevFree( oldFree ) == NULL ); // It should have been first in the list
	setPrevFree( oldFree, b );

	heap->free = b;
	setPrevFree( b, NULL );
}

void removeFromFreeList( heapAllocator* heap, block* b ) {
	//validateFreeList( heap );
	block* const pFree = prevFree( b );
	block* const nFree = nextFree( b );
	//printf( "prev: " xPTRf ", next: " xPTRf "\n", (uintptr_t)pFree, (uintptr_t)nFree );
	//vAssert( nFree || pFree );
	if ( pFree ) { 
		vAssert( pFree->free );
		//vAssert( nFree || pFree->prevFree );
		setNextFree( pFree, nFree );
	}
	else {
		vAssert( heap->free == b );
		if (!nFree ) {
			block* bl = heap->first;
			while (bl) {
				vAssert( bl == b || !bl->free );
				bl = bl->next;
			}
		}
		heap->free = nFree;
	}
	if ( nFree ) {
		vAssert( nFree->free );
		setPrevFree( nFree, pFree );
	}
	setNextFree( b, NULL );
	setPrevFree( b, NULL );
	b->free = false;
	//validateFreeList( heap );
	b->free = true;
}

// Allocates *size* bytes from the given heapAllocator *heap*
// Will crash if out of memory
// NEEDS TO BE THREADSAFE
void* heap_allocate_aligned( heapAllocator* heap, size_t size, size_t alignment, const char* source ) {
	(void)source;
	vmutex_lock( &allocator_mutex );
	//validateFreeList( heap );
#ifdef MEM_DEBUG_VERBOSE
	printf( "HeapAllocator request for " dPTRf " bytes, " dPTRf " byte aligned.\n", size, alignment );
#endif
	bitpool* bit_pool = heap_findBitpool( heap, size );
	if ( bit_pool ) {
		void* data = bitpool_allocate( bit_pool, size );
		if ( data ) {
			vmutex_unlock( &allocator_mutex );
			return data;
		}
	}

	size_t size_original = size;
	size += alignment;	// Make sure we have enough space to align
	block* b = heap_findEmptyBlock( heap, size );

	if ( !b ) {
		heap_dumpBlocks( heap );
		printError( "HeapAllocator out of memory on request for " dPTRf " bytes. Total size: " dPTRf " bytes, Used size: " dPTRf " bytes\n", size, heap->total_size, heap->total_allocated );
		vAssert( 0 );
	}

	vAssert( b->free );
	vAssert( !b->next || b->next == b->data + b->size );

	removeFromFreeList( heap, b );
	b->free = false;

	if ( b->size > ( size + sizeof( block ) + sizeof( block* ) * 2) ) {
		void* new_ptr = ((uint8_t*)b->data) + size;
		block* remaining = block_create( heap, new_ptr, b->size - size );
		block_insertAfter( b, remaining );
		b->size = size;
		heap->total_allocated += sizeof( block );
		heap->total_free -= sizeof( block );
		vAssert( b->next == b->data + b->size );
	}

	vAssert( !b->next || b->next == b->data + b->size );

	// Move the data pointer on enough to force alignment
	uintptr_t offset = alignment - (((uintptr_t)b->data - 1) % alignment + 1);
	b->data = ((uint8_t*)b->data) + offset;
	b->size -= offset;
	// Now move the block on and copy the header, so that it's contiguous with the block
	block* new_block_position = (block*)(((uint8_t*)b) + offset);
	// Force alignment for this so we can memcpy (on Android, even memcpy requires alignments when dealing with structs)
	block block_temp;
	memcpy( &block_temp, b, sizeof( block ));
	//validateFreeList( heap );
	//printf( "heap " xPTRf " before : %d free blocks\n", (uintptr_t)heap, countFree( heap ));
	//printf( "heap->free " xPTRf "\n", (uintptr_t)heap->free );
	//printFree( heap );
	//////////////////////////////////////////////////////
	vAssert( b->prevFree == NULL );
	vAssert( b->nextFree == NULL );
	b = new_block_position;
	memcpy( b, &block_temp, sizeof( block ));
	// Fix up pointers to this block, for the new location
	if ( b->prev ) {
		b->prev->next = b;
		// Increment previous block size by what we've moved the block
		b->prev->size += offset;
	}
	else
		heap->first = b;
	if ( b->next )
		b->next->prev = b;
	//////////////////////////////////////////////////////
	//printf( "heap " xPTRf " after : %d free blocks\n", (uintptr_t)heap, countFree( heap ));
	//printf( "heap->free " xPTRf "\n", (uintptr_t)heap->free );
	//printFree( heap );
	//validateFreeList( heap );
	vAssert( b->next == NULL || b->next == b->data + b->size );

	heap->total_allocated += size;
	heap->total_free -= size;

	// Ensure we have met our requirements
	uintptr_t align_offset = ((uintptr_t)b->data) % alignment;
	vAssert( align_offset == 0 );	// Correctly Aligned
	vAssert( b->size >= size_original );	// Large enough

#ifdef MEM_DEBUG_VERBOSE
	printf("Allocator returned address: " xPTRf ".\n", (uintptr_t)b->data );
#endif

	++heap->allocations;

#ifdef MEM_STACK_TRACE
	block_recordAlloc( b, mem_stack_string );
#endif // MEM_STACK_TRACE

	vAssert( !b->next || b->next->prev == b );
	vAssert( !b->prev || b->prev->next == b );

#ifdef TRACK_ALLOCATIONS
	if (source)
		strncpy( b->source, source, MemSourceLength);
	else
		b->source[0] = '\0';
#endif
	//validateFreeList( heap );
	vmutex_unlock( &allocator_mutex );

	return b->data;
}

#ifdef MEM_STACK_TRACE
void block_recordAlloc( block* b, const char* stack ) {
	b->stack = stack;
	}
#endif // MEM_STACK_TRACE

void heap_dumpBlocks( heapAllocator* heap ) {
	block* b = heap->first;
	FILE* memlog = fopen( "memlog", "w" );
	while ( b ) {
#ifdef TRACK_ALLOCATIONS
		if ( !b->free )
			fprintf( memlog, "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\t\tSource: %s\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free, b->source );
		else
			fprintf( memlog, "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free );
#else
#ifdef MEM_STACK_TRACE
		if ( b->stack && !b->free )
			printf( "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\t\tStack: %s\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free, b->stack );
		else
			printf( "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free );
#else // MEM_STACK_TRACE
		printf( "Block: ptr 0x" xPTRf ", data: 0x" xPTRf ", size " dPTRf ", free " dPTRf "\n", (uintptr_t)b, (uintptr_t)b->data, b->size, b->free );
#endif // MEM_STACK_TRACE
#endif
		vAssert( b->next == 0 || (uintptr_t)b->next > 0x1ff );
		b = b->next;
	}
	fclose( memlog );
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
// ---First version will naively use first found block meeting the criteria
// +++Now use empty blocks to maintain a free-list
block* heap_findEmptyBlock( heapAllocator* heap, size_t min_size ) {
	block* b = heap->free;
	block* found = NULL;
	//block* b = heap->first;
	block* prev = NULL;
	while ( !found && b ) {
#ifdef MEM_GUARD_BLOCK
		assert( b->guard == kGuardValue );
#endif
		//printf( "Trying block " xPTRf "\n", (uintptr_t)b );
		if ( b->size >= min_size )
			found = b;
		block* tmp = b;
		b = nextFree( b );
		prev = tmp;
		(void)prev;
	}
	// Re-check in case we ran out without finding one
	return ( !found || !found->free || ( found->size < min_size )) ? NULL : found;
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

int countFree( heapAllocator* heap ) {
	int i = 0;
	block* b = heap->free;
	while ( b ) {
		vAssert( b->free );
		++i;
		b = b->nextFree;
	}
	return i;
}

int printFree( heapAllocator* heap ) {
	int i = 0;
	block* b = heap->free;
	while ( b ) {
		vAssert( b->free );
		printf( "Free : " xPTRf "\n", (uintptr_t)b );
		++i;
		b = b->nextFree;
	}
	return i;
}



void checkFree( heapAllocator* heap, block* bl );

void heapContains( heapAllocator* heap, block* bl ) {
	block* b = heap->first;
	bool found = false;
	while ( b && !found ) {
		if ( b == bl ) found = true;
		b = b->next;
	}
	vAssert( found );
}

// Release a block from the heapAllocator
void heap_deallocate( heapAllocator* heap, void* data ) {
	if ( data == NULL )
		return;
	vmutex_lock( &allocator_mutex );

	//validateFreeList( heap );
	// Check if it's in a bitpool
	bitpool* bit_pool = heap_findBitpoolForData( heap, data );
	if ( bit_pool ) {
		bitpool_free( bit_pool, data );
		vmutex_unlock( &allocator_mutex );
		return;
	}

	block* b = (block*)((uint8_t*)data - sizeof( block ));
	// Check it's in the right heap!
	//heapContains( heap, b );
	vAssert( !b->free );

#ifdef MEM_GUARD_BLOCK
	vAssert( b->guard == kGuardValue );
#endif // MEM_GUARD_BLOCK
	vAssert( b );
	vAssert( !b->next || b->next->prev == b );
	vAssert( !b->prev || b->prev->next == b );
#ifdef MEM_DEBUG_VERBOSE
	printf("Allocator freed address: " xPTRf ".\n", (uintptr_t)b->data );
#endif
	//validateFreeList( heap );
	b->free = true;
	addToFreeList( heap, b );
	heap->total_free += b->size;
	heap->total_allocated -= b->size;

	checkFree( heap, b );
	// Try to merge blocks
	if ( b->next && b->next->free ) {
		checkFree( heap, b->next );
		block_merge( heap, b, b->next );
	}

	if ( b->prev && b->prev->free ) {
		checkFree( heap, b->prev );
		block_merge( heap, b->prev, b );
	}

	//validateFreeList( heap );
	--heap->allocations;
	vmutex_unlock( &allocator_mutex );
}

void printFreeList( heapAllocator* heap ) {
	block* b = heap->free;
	while ( b ) {
		printf( "Free: " xPTRf "\n", (uintptr_t)b );
		b = nextFree( b );
	}
}

void checkFree( heapAllocator* heap, block* bl ) {
	block* b = heap->free;
	bool found = false;
	while ( b && !found ) {
		if ( b == bl ) found = true;
		b = nextFree( b );
	}
	if ( !found )
		printError( "Block 0x" xPTRf " is free but not in the free list\n", (uintptr_t)bl );
	vAssert( found );
}

void validateFreeList( heapAllocator* heap ) {
	block* b = heap->first;
	while ( b ) {
		if ( b->free ) {
			checkFree( heap, b );
		}
		b = b->next;
	}
}

// Merge two continous blocks, *first* and *second*
// Both *first* and *second* must be valid
// Afterwards, only *first* will remain valid
// but will have size equal to both plus sizeof( block )
void block_merge( heapAllocator* heap, block* first, block* second ) {
	//printf( "Allocator: Merging Blocks 0x" xPTRf " and 0x " xPTRf "\n", (uintptr_t)first, (uintptr_t)second );

	vAssert( first );
	vAssert( second );
	vAssert( first->free && second->free );								// Both must be empty
	vAssert( ((char*)second - ((char*)first->data + first->size)) < kMaxAlignmentSpace );	// Contiguous
	vAssert( first->next == second );
	//if ( second->prev != first ) {
		//printf( "Second: 0x" xPTRf ", first: 0x" xPTRf ", second->prev: 0x" xPTRf "\n", (uintptr_t)second, (uintptr_t)first, (uintptr_t)second->prev );
	//}
	vAssert( second->prev == first );

	vAssert( !first->next || first->next == first->data + first->size );
	vAssert( !second->next || second->next == second->data + second->size );

	vAssert( second > first );
	vAssert( second->next > second || second->next == NULL );

	heap->total_free += sizeof( block );
	heap->total_allocated -= sizeof( block );
	
	removeFromFreeList( heap, second );

	// We can't just add sizes, as there may be alignment padding.
	size_t true_size = second->size + ( (size_t)second->data - (size_t)second );
	first->size = first->size + true_size;
	first->next = second->next;
	if ( second->next )
		second->next->prev = first;

	memset( second, 0xED, sizeof( block ));
	vAssert( !first->next || first->next == first->data + first->size );
	vAssert( !first->next || first->next->prev == first );
	vAssert( !first->prev || first->prev->next == first );
}

// Create a heapAllocator of *size* bytes
// Initialised with one block pointing to the whole memory
heapAllocator* heap_create( int heap_size ) {
	// We add space for the first block header, so we do get the correct total size
	// ie. this means that heap_create (size), followed by heap_Allocate( size ) should work
	void* data = malloc( sizeof( heapAllocator ) + sizeof( block ) + heap_size );
	memset( data, 0, sizeof( heapAllocator ) + sizeof( block ) + heap_size );

	heapAllocator* allocator = (heapAllocator*)data;
	data += sizeof( heapAllocator );
	allocator->total_size = heap_size;
	allocator->total_free = heap_size;
	allocator->total_allocated = 0;
	allocator->bitpool_count = 0;
	
	// Should not be possible to fail creating the first block header
	allocator->free = NULL;
	allocator->first = block_create( allocator, data, heap_size );
	vAssert( allocator->first ); 
	vAssert( allocator->free == allocator->first ); 
	vAssert( allocator->free == allocator->first ); 
	vAssert( nextFree( allocator->free ) == NULL ); 

	return allocator;
}

// Insert a block *after* into a linked-list after the block *before*
// Both *before* and *after* must be valid
void block_insertAfter( block* before, block* after ) {
	vAssert( before );
	//vAssert( before->free );
	vAssert( after );
	//vAssert( after->free );

	after->next = before->next;
	after->prev = before;
	before->next = after;
	if ( after->next )
		after->next->prev = after;
}

// Create and initialise a block in a given piece of memory of *size* bytes
block* block_create( heapAllocator* heap, void* data, size_t size ) {
	block* b = (block*)data;
	memset( b, 0, sizeof( block ));
	b->size = size - sizeof( block );
	b->data = ((u8*)data) + sizeof( block );
	b->free = true;
	b->prev = b->next = NULL;
	vAssert( size > sizeof( void* ) * 2 );
	vAssert( b->size > sizeof( block* ) * 2 );
	addToFreeList( heap, b );
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
	/*
	printf( "%s--- Beginning Unit Test: Heap Allocator ---\n", TERM_WHITE );
	heapAllocator* heap = heap_create( 4096 );

	test( heap, "Allocator created successfully.", "Allocator not created." );
	test( heap->total_size == 4096, "Created Allocator of 4096 byte heap.", "Allocator has incorrect size (should be 4096)." );
	
	test( heap->free != NULL, "Heap has a free block", NULL );
	test( nextFree(heap->free) == NULL, "Heap has only one free block", NULL );
	test( prevFree(heap->free) == NULL, "Heap has only one free block", NULL );

	void* a = heap_allocate( heap, 2048, NULL );
	memset( a, 0, 2048 );
	test( true, "Allocated 2048 bytes succesfully.", NULL );

	void* b = heap_allocate( heap, 1024, NULL );
	memset( b, 0, 1024 );
	test( true, "Allocated 2048 + 1024 bytes succesfully.", NULL );

	heap_deallocate( heap, a );
	test( true, "Deallocated 2048 bytes succesfully.", NULL );

	heap_deallocate( heap, b );
	test( true, "Deallocated 1024 bytes succesfully.", NULL );

	a = heap_allocate( heap, 3072, NULL );
	memset( a, 0, 3072 );
	test( true, "Allocated 3072 bytes succesfully.", NULL );

	b = heap_allocate( heap, 512, NULL );
	memset( b, 0, 512 );
	test( true, "Allocated 512 bytes succesfully.", NULL );

	free( heap );
	heap = heap_create( 4096 );
	test( heap->free != NULL, "Heap has a free block", NULL );
	test( nextFree(heap->free) == NULL, "Heap has only one free block", NULL );

	//// freelist tests

	heapAllocator* hp = heap_create( 4096 );
	void* aa = heap_allocate( hp, 768, NULL );
	void* bb = heap_allocate( hp, 768, NULL );
	void* cc = heap_allocate( hp, 768, NULL );
	heap_deallocate( hp, aa );
	heap_deallocate( hp, bb );
	heap_deallocate( hp, cc );
	aa = heap_allocate( hp, 768, NULL );
	bb = heap_allocate( hp, 768, NULL );
	cc = heap_allocate( hp, 384, NULL );
	void* dd = heap_allocate( hp, 384, NULL );
	heap_deallocate( hp, bb );
	heap_deallocate( hp, cc );
	cc = heap_allocate( hp, 1024, NULL );
	heap_deallocate( hp, cc );
	heap_deallocate( hp, aa );
	heap_deallocate( hp, dd );

	//vAssert( false );
	*/
}
#endif // UNIT_TEST

passthroughAllocator* passthrough_create( heapAllocator* heap ) {
	passthroughAllocator* p = mem_alloc( sizeof( passthroughAllocator ));
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
	
void mem_pushStackString( const char* string ) {
	vAssert( mem_stack_string == NULL );
	mem_stack_string = string;
	}

void mem_popStackString() {
	vAssert( mem_stack_string != NULL );
	mem_stack_string = NULL;
	}
