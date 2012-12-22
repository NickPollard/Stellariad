// bitpool.h

typedef struct bitpool_s { 
	size_t		block_size;		// How big each block is (in BYTES)
	size_t		block_count;	// How many blocks we have
	uint8_t*	arena;			// Pointer to a memory arena of size equal to ( BLOCK_SIZE * BLOCK_COUNT )
	void*		first_free;
} bitpool;

void*	bitpool_allocate( bitpool* b, size_t size );
void	bitpool_free( bitpool* b, void* data );
bitpool	bitpool_create( size_t size, size_t count, void* arena );
bool	bitpool_contains( bitpool* b, void* data );
