// hash.c
#include "common.h"
#include "hash.h"
//-----------------------
#include "mem/allocator.h"

unsigned int mhash( const char* src ) {
	unsigned int seed = 0x0;
	int len = strlen( src );
	return MurmurHash2( src, len, seed );
}
/*
void* hashtable_findOrAdd( hashtable, key ) {

}

void* hashtable_find( hashtable, key ) {
	// NYI
	assert( 0 );
	// find the key in the hashtable
	// If it's not there, return null
	return NULL;
}
*/

map* map_create( int max, int stride ) {
	map* m = mem_alloc( sizeof( map ));
	m->max = max;
	m->count = 0;
	m->stride = stride;
	m->keys = mem_alloc( sizeof( int ) * max );
	m->values = mem_alloc( stride * max );
	memset( m->keys, 0, sizeof( int ) * max );
	memset( m->values, 0, stride * max );
	return m;
}

void* map_find( map* m, int key ) {
	for ( int i = 0; i < m->count; i++ ) {
		if ( m->keys[i] == key )
			return m->values + (i * m->stride);
	}
	return NULL;
}

void map_add( map* m, int key, void* value ) {
	assert( map_find( m, key) == NULL );
	assert( m->count < m->max );
	int i = m->count;
	m->keys[i] = key;
	if ( value ) // Allow NULL in which case don't copy
		memcpy( m->values + (i * m->stride), value, m->stride );
	m->count++;
}

void* map_findOrAdd( map* m, int key ) {
	void* data = map_find( m, key );
	if ( !data ) {
		data = m->values + (m->count * m->stride);
		map_add( m, key, NULL );
	}
	return data;
}

void map_addOverride( map* m, int key, void* value ) {
	void* data = map_findOrAdd( m, key );
	memcpy( data, value, m->stride );
}

void map_delete( map* m ) {
	mem_free( m->keys );
	mem_free( m->values );
	mem_free( m );
}

void test_map() {
	map* test_map = map_create( 16, sizeof( unsigned int ));
	int key = mhash( "modelview" );
	unsigned int value = 0x3;
	map_add( test_map, key, &value );
	unsigned int* modelview = map_find( test_map, key );
	assert( *modelview = value );
}

// *** Test

void test_murmurHash( const char* source ) {
	printf( "Hash of: \"%s\" is 0x%x.\n", source, mhash( source ));
}

void test_hash() {

	test_map();

	/*
	test_murmurHash( "test" );
	test_murmurHash( "blarg" );
	test_murmurHash( "scrund" );
	test_murmurHash( "people" );
	test_murmurHash( "simples" );
	*/
}
