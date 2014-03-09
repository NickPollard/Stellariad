// buildCacheTask.c
#include "src/common.h"
#include "src/terrain/buildCacheTask.h"
//---------------------
#include "canyon.h"
#include "canyon_terrain.h"
#include "worker.h"
#include "base/pair.h"
#include "mem/allocator.h"
#include "system/thread.h"

IMPLEMENT_LIST(cacheBlock);

vmutex terrainMutex = kMutexInitialiser;

// Find the element in the list and return it, orElse none
cacheBlock* cached( terrainCache* cache, int uMin, int vMin ) {
	cacheBlock* block = NULL;
	vmutex_lock( &terrainMutex ); {
		cacheBlocklist* b = cache->blocks;
		while ( b ) {
			if ( b->head->uMin == uMin && b->head->vMin == vMin ) {
				block = b->head;
				break;
			}
			b = b->tail;
		}
	} vmutex_unlock( &terrainMutex );
	return block;
}

void cacheAdd( terrainCache* t, cacheBlock* b ) {
	(void)t;(void)b;
	vmutex_lock( &terrainMutex ); {
	} vmutex_unlock( &terrainMutex );
}

cacheBlock* generateCacheBlock( canyon* c, int uMin, int vMin ) {
	cacheBlock* b = mem_alloc( sizeof( cacheBlock ));
	b->uMin = uMin;
	b->vMin = vMin;
	for ( int v = 0; v < CacheBlockSize; ++v ) {
		for ( int u = 0; u < CacheBlockSize; ++u ) {
			float x, z;
			terrain_worldSpaceFromCanyon( c, u + uMin, v + vMin, &x, &z );
			b->positions[u][v] = Vector( x, canyonTerrain_sampleUV( u + uMin, v + vMin ), z, 1.f );
		}
	}
	return b;
}

// Generate a Cache block, if it's not already built from terrain params
void* buildCache( void* args ) {
	canyon* c = _1(args);
	terrainCache* t = c->terrainCache;
	int u = (intptr_t)_1(_2(args));
	int v = (intptr_t)_2(_2(args));
	if (cached( t, u, v ) == NULL)
		cacheAdd( t, generateCacheBlock( c, u, v ));

	mem_free( _2(args) ); // Free nested Pair
	mem_free( args );
	return NULL;
}

void buildCache_queueWorkerTask( canyon* c, int u, int v ) {
	// make the task and queue it
	worker_task build;
	build.func = buildCache;
	build.args = Pair( c, Pair( (void*)(intptr_t)u, (void*)(intptr_t)v ));
	worker_addTask( build );
}
