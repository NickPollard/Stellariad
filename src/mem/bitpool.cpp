// bitpool.c
#include "common.h"
#include "bitpool.h"
//---------------------

#define kInvalidNextFree ((void*)-1)

// Initialize the free list used to keep track of what space we can use in the bitpool
// Each bitblock points to the next one as free; Last has kInvalidNextFree
void bitpool_initFree( bitpool* b ) {
	for ( size_t i = 0; i < b->block_count; ++i ) {
		size_t address = b->block_size * i;
		void** data = (void**)&b->arena[address];
		void* next_address = (void*)&b->arena[address+b->block_size];
		(*data) = next_address;
	}
	// Init the last to invalid
	size_t address = b->block_size * (b->block_count - 1);
	void** data = (void**)&b->arena[address];
	(*data) = kInvalidNextFree;
}

// Create a new bitpool
bitpool bitpool_create( size_t size, size_t count, void* arena ) {
	bitpool b;
	vAssert( size > sizeof( void* )) // Cannot have bitpools that cannot store a pointer (used for free list)
	b.block_size	= size;
	b.block_count	= count;
	b.arena			= (uint8_t*)arena;
	b.first_free	= b.arena;

	bitpool_initFree( &b );

	return b;
}

void* bitpool_allocate( bitpool* b, size_t size ) {
	//printf( "Allocating from %d-byte bitpool.\n", (int)b->block_size );
	vAssert( size <= b->block_size );
	void* data = b->first_free;
	if ( data != kInvalidNextFree ) {
		b->first_free = *(void**)b->first_free;
		return data;
	}
	else {
		return NULL;
	}
}

size_t bitpool_index( bitpool* b, void* data ) {
	size_t offset = (uintptr_t)data - (uintptr_t)b->arena;
	size_t index = offset / b->block_size;
	size_t alias = offset % b->block_size;
	(void)alias;
	vAssert( alias == 0 );	// Must be an exact multiple
	vAssert( index > 0 );
	vAssert( index < b->block_count );
	return index;
}

void bitpool_free( bitpool* b, void* data ) {
	size_t index = bitpool_index( b, data ); (void)index; // we are just checking that it's valid
	// What is b->first_free here? Could be kInvalidNextFree - does that matter?
	// So we make data point to b->first_free
	// Then make b->first_free point to data
	// ie. prepend
	vAssert( b->first_free != 0x0 );
	vAssert( data != 0x0 );
	*(void**)data = b->first_free;
	b->first_free = data;
}

bool bitpool_contains( bitpool* b, void* data ) {
	uintptr_t address = (uintptr_t)data;
	uintptr_t last = (uintptr_t)b->arena + (uintptr_t)(b->block_size * b->block_count);
	return ( address >= (uintptr_t)b->arena && address < last ); 
}
