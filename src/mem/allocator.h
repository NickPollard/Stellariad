#pragma once
// Allocator.h
#include "mem/bitpool.h"

//#define MEM_DEBUG_VERBOSE
#define MEM_GUARD_BLOCK
#define MEM_FORCE_ALIGNED
//#define MEM_STACK_TRACE

#ifdef MEM_STACK_TRACE
#define mem_pushStack( string ) mem_pushStackString( string )
#define mem_popStack( ) mem_popStackString( )
#else // MEM_STACK_TRACE
#define mem_pushStack( string )
#define mem_popStack( )
#endif // MEM_STACK_TRACE

#define TRACK_ALLOCATIONS

#ifdef TRACK_ALLOCATIONS
#define mem_alloc( size ) mem_alloc_( size, __func__, __FILE__, __LINE__ ) 
#define MemSourceLength 48
#endif

#define kMaxBitpools 8

typedef struct block_s block;

// A heap allocator struct
// Allocates memory from a defined size heap
// Uses a doubly-linked list of block headers to keep track of used space
// Insertion time is O(n)
// Deallocation is O(n)
struct heapAllocator_s {
	size_t total_size;		// in bytes, size of the heap
	size_t total_allocated;	// in bytes, currently allocated
	size_t total_free;		// in bytes, currently free
	size_t allocations;
	block* first;					// doubly-linked list of blocks
	block* free;					// doubly-linked list of free blocks
	// Bitpools
	int			bitpool_count;
	bitpool		bitpools[kMaxBitpools];
};

// A memory block header for the heapAllocator
// Each heapAllocator has a doubly-linked list of these
// Each heap_allocate call will return one of these
// TODO - can optimise the space here?
struct block_s {
	void*	data;			// the memory location of the actual block
	size_t	size;	// in bytes, the block size
	block*	next;			// doubly-linked list pointer
	block*	prev;			// doubly-linked list pointer
	block* padding;
	block* nextFree;
	block* paddingee;
	block* prevFree;
	size_t	free;	// true (1) if free, false (0) if used
#ifdef MEM_STACK_TRACE
	const char* stack;
#endif
#ifdef TRACK_ALLOCATIONS
	char source[MemSourceLength];
#endif
#ifdef MEM_GUARD_BLOCK
	unsigned int	guard;	// Guard block for Canary purposes
#endif
}__attribute__ ((aligned (8)));

typedef struct empty_s {
	block* nextFree;
	block* prevFree;
} empty;

// Default allocate from the static heap
// Passes straight through to heap_allocate()
#ifdef TRACK_ALLOCATIONS
void* mem_alloc_(size_t bytes, const char* func, const char* file, int line );
#else
void* mem_alloc(size_t bytes);
#endif

// Default deallocate from the static heap
// Passes straight through to heap_deallocate()
void mem_free( void* ptr );

// Initialise the memory subsystem
void mem_init(int argc, char** argv);

// Allocates *size* bytes from the given heapAllocator *heap*
// Will crash if out of memory
void* heap_allocate( heapAllocator* heap, int size, const char* source );
void* heap_allocate_aligned( heapAllocator* heap, size_t size, size_t alignment, const char* source );

// Release a block from the heapAllocator
void heap_deallocate( heapAllocator* heap, void* data );

// Create a heapAllocator of *size* bytes
// Initialised with one block pointing to the whole memory
heapAllocator* heap_create( int heap_size );

// Add a bitpool of COUNT blocks of SIZE bytes to the given heapAllocator
// The bitpool storage is taken from the heaps storage, so there must be enough space
// for the bitpool arena
void heap_addBitpool( heapAllocator* h, size_t size, size_t count );

void heap_dumpBlocks( heapAllocator* heap );
void heap_dumpUsedBlocks( heapAllocator* heap );

void mem_pushStackString( const char* string );
void mem_popStackString( );
//
// Tests
//

void test_allocator();
