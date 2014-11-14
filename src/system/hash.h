// hash.h
#pragma once

unsigned int mhash( const char* src );
unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed );

void test_murmurHash( const char* source );
void test_hash();

// Simple (non-hashed) map
struct map_s {
	int max;
	int count;
	int stride;
	int*	keys;
	uint8_t*	values;
};

map*	map_create( int max, int stride );
void	map_delete( map* m );
void*	map_find( map* m, int key );
void	map_add( map* m, int key, void* value );
void	map_addOverride( map* m, int key, void* value );
void*	map_findOrAdd( map* m, int key );
